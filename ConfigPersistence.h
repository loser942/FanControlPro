#pragma once

#include <windows.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

constexpr DWORD CONFIG_FILE_MAGIC = 0x46504346;
constexpr DWORD CONFIG_FILE_VERSION_V3 = 3;
constexpr DWORD CONFIG_FILE_VERSION_V4 = 4;
constexpr size_t CONFIG_FAN_COUNT = 2;
constexpr size_t CONFIG_TEMP_LEVEL_COUNT = 10;

struct ConfigV4Disk
{
    int DutyList[CONFIG_FAN_COUNT][CONFIG_TEMP_LEVEL_COUNT];
    int TransitionTemp;
    int UpdateInterval;
    BOOL Linear;
    BOOL TakeOver;
    int ForceTemp;
    int MaxDutyLimit;
    BOOL LockGPUFrequency;
    int GPUFrequency;
    int ControlMode;
    int ManualDuty[CONFIG_FAN_COUNT];
    int WarningTemp;
    BOOL DesktopNotifications;
    int NotificationCooldownMinutes;
};

struct ConfigV4FileHeader
{
    DWORD magic;
    DWORD version;
    DWORD payloadSize;
};

namespace config_persistence_detail
{
struct ConfigV3Disk
{
    int DutyList[CONFIG_FAN_COUNT][CONFIG_TEMP_LEVEL_COUNT];
    int TransitionTemp;
    int UpdateInterval;
    BOOL Linear;
    BOOL TakeOver;
    int ForceTemp;
    int MaxDutyLimit;
    BOOL LockGPUFrequency;
    int GPUFrequency;
    int ControlMode;
    int ManualDuty[CONFIG_FAN_COUNT];
    int WarningTemp;
    BOOL DesktopNotifications;
    int NotificationCooldownMinutes;
};

inline DWORD NextChunkSize(size_t byteCount)
{
    const size_t maximum = static_cast<size_t>((std::numeric_limits<DWORD>::max)());
    return static_cast<DWORD>(byteCount < maximum ? byteCount : maximum);
}

inline bool ReadExact(HANDLE file, void* bytes, size_t byteCount)
{
    BYTE* destination = static_cast<BYTE*>(bytes);
    while (byteCount != 0)
    {
        const DWORD chunkSize = NextChunkSize(byteCount);
        DWORD bytesRead = 0;
        if (!ReadFile(file, destination, chunkSize, &bytesRead, nullptr) || bytesRead != chunkSize)
            return false;

        destination += bytesRead;
        byteCount -= bytesRead;
    }
    return true;
}

inline bool WriteExact(HANDLE file, const void* bytes, size_t byteCount)
{
    const BYTE* source = static_cast<const BYTE*>(bytes);
    while (byteCount != 0)
    {
        const DWORD chunkSize = NextChunkSize(byteCount);
        DWORD bytesWritten = 0;
        if (!WriteFile(file, source, chunkSize, &bytesWritten, nullptr) || bytesWritten != chunkSize)
            return false;

        source += bytesWritten;
        byteCount -= bytesWritten;
    }
    return true;
}

inline void Normalize(ConfigV4Disk& config)
{
    config.TransitionTemp = std::clamp(config.TransitionTemp, 0, 10);
    config.UpdateInterval = std::clamp(config.UpdateInterval, 1, 5);
    config.ForceTemp = std::clamp(config.ForceTemp, 60, 95);
    config.MaxDutyLimit = std::clamp(config.MaxDutyLimit, 0, 100);
    config.ControlMode = std::clamp(config.ControlMode, 0, 2);
    config.WarningTemp = std::clamp(config.WarningTemp, 60, 100);
    config.NotificationCooldownMinutes = std::clamp(config.NotificationCooldownMinutes, 1, 60);
    config.Linear = !!config.Linear;
    config.TakeOver = !!config.TakeOver;
    config.LockGPUFrequency = !!config.LockGPUFrequency;
    config.DesktopNotifications = !!config.DesktopNotifications;
    config.GPUFrequency = config.GPUFrequency < 0 ? 0 : config.GPUFrequency;

    for (size_t fan = 0; fan < CONFIG_FAN_COUNT; ++fan)
    {
        config.ManualDuty[fan] = std::clamp(config.ManualDuty[fan], 0, 100);
        for (size_t level = 0; level < CONFIG_TEMP_LEVEL_COUNT; ++level)
            config.DutyList[fan][level] = std::clamp(config.DutyList[fan][level], 0, 100);
    }
}

inline void MigrateV3(const ConfigV3Disk& source, ConfigV4Disk& destination)
{
    std::memcpy(destination.DutyList, source.DutyList, sizeof(destination.DutyList));
    destination.TransitionTemp = source.TransitionTemp;
    destination.UpdateInterval = source.UpdateInterval;
    destination.Linear = source.Linear;
    destination.TakeOver = source.TakeOver;
    destination.ForceTemp = source.ForceTemp;
    destination.MaxDutyLimit = source.MaxDutyLimit;
    destination.LockGPUFrequency = source.LockGPUFrequency;
    destination.GPUFrequency = source.GPUFrequency;
    destination.ControlMode = source.ControlMode;
    std::memcpy(destination.ManualDuty, source.ManualDuty, sizeof(destination.ManualDuty));
    destination.WarningTemp = source.WarningTemp;
    destination.DesktopNotifications = source.DesktopNotifications;
    destination.NotificationCooldownMinutes = source.NotificationCooldownMinutes;
    Normalize(destination);
}

inline bool ReadConfigFile(PCWSTR path, void* bytes, size_t byteCount)
{
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER fileSize{};
    const bool hasSize = GetFileSizeEx(file, &fileSize) != FALSE;
    const bool validSize = hasSize && fileSize.QuadPart >= 0;
    DWORD prefix[2]{};
    const bool hasPrefix = validSize && ReadExact(file, prefix, sizeof(prefix));
    if (!hasPrefix || prefix[0] != CONFIG_FILE_MAGIC)
    {
        CloseHandle(file);
        return false;
    }

    bool read = false;
    if (prefix[1] == CONFIG_FILE_VERSION_V4)
    {
        DWORD payloadSize = 0;
        const ULONGLONG expectedSize = static_cast<ULONGLONG>(sizeof(ConfigV4FileHeader)) + byteCount;
        if (fileSize.QuadPart == static_cast<LONGLONG>(expectedSize) &&
            ReadExact(file, &payloadSize, sizeof(payloadSize)) && payloadSize == byteCount)
        {
            std::vector<BYTE> payload(byteCount);
            read = ReadExact(file, payload.data(), byteCount);
            if (read && byteCount == sizeof(ConfigV4Disk))
            {
                ConfigV4Disk config{};
                std::memcpy(&config, payload.data(), sizeof(config));
                Normalize(config);
                std::memcpy(bytes, &config, sizeof(config));
            }
            else if (read)
            {
                std::memcpy(bytes, payload.data(), byteCount);
            }
        }
    }
    else if (prefix[1] == CONFIG_FILE_VERSION_V3 && byteCount == sizeof(ConfigV4Disk))
    {
        const ULONGLONG expectedSize = static_cast<ULONGLONG>(sizeof(prefix)) + sizeof(ConfigV3Disk);
        if (fileSize.QuadPart == static_cast<LONGLONG>(expectedSize))
        {
            ConfigV3Disk legacy{};
            read = ReadExact(file, &legacy, sizeof(legacy));
            if (read)
            {
                ConfigV4Disk migrated{};
                MigrateV3(legacy, migrated);
                std::memcpy(bytes, &migrated, sizeof(migrated));
            }
        }
    }

    CloseHandle(file);
    return read;
}
}

inline bool WriteConfigAtomically(PCWSTR path, const void* bytes, size_t byteCount)
{
    if (path == nullptr || (bytes == nullptr && byteCount != 0) ||
        byteCount > (std::numeric_limits<DWORD>::max)())
    {
        return false;
    }

    const std::wstring configPath(path);
    const std::wstring temporaryPath = configPath + L".tmp";
    const std::wstring backupPath = configPath + L".bak";
    const ConfigV4FileHeader header{ CONFIG_FILE_MAGIC, CONFIG_FILE_VERSION_V4,
        static_cast<DWORD>(byteCount) };

    HANDLE temporaryFile = CreateFileW(temporaryPath.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (temporaryFile == INVALID_HANDLE_VALUE)
        return false;

    const bool written = config_persistence_detail::WriteExact(temporaryFile, &header, sizeof(header)) &&
        config_persistence_detail::WriteExact(temporaryFile, bytes, byteCount) &&
        FlushFileBuffers(temporaryFile) != FALSE;
    CloseHandle(temporaryFile);
    if (!written)
    {
        DeleteFileW(temporaryPath.c_str());
        return false;
    }

    const DWORD attributes = GetFileAttributesW(configPath.c_str());
    bool replaced = false;
    if (attributes != INVALID_FILE_ATTRIBUTES)
    {
        replaced = ReplaceFileW(configPath.c_str(), temporaryPath.c_str(), backupPath.c_str(),
            REPLACEFILE_WRITE_THROUGH, nullptr, nullptr) != FALSE;
    }
    else if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND)
    {
        replaced = MoveFileExW(temporaryPath.c_str(), configPath.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    }

    if (!replaced)
        DeleteFileW(temporaryPath.c_str());
    return replaced;
}

inline bool ReadConfigWithBackup(PCWSTR path, void* bytes, size_t byteCount)
{
    if (path == nullptr || (bytes == nullptr && byteCount != 0))
        return false;

    if (config_persistence_detail::ReadConfigFile(path, bytes, byteCount))
        return true;

    const std::wstring backupPath = std::wstring(path) + L".bak";
    return config_persistence_detail::ReadConfigFile(backupPath.c_str(), bytes, byteCount);
}
