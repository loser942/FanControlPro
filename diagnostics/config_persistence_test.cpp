#include "../ConfigPersistence.h"

#include <assert.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
std::wstring MakeTestPath()
{
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / L"FanControlProConfigPersistenceTest";
    std::filesystem::create_directories(directory);
    return (directory / L"config.bin").wstring();
}

void RemoveTestFiles(const std::wstring& configPath)
{
    std::filesystem::remove(configPath);
    std::filesystem::remove(configPath + L".bak");
    std::filesystem::remove(configPath + L".tmp");
}

void WriteIncompleteFile(const std::wstring& path)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write("bad", 3);
}

void WriteStaleTemporaryFile(const std::wstring& path)
{
    ConfigV4Disk stale{};
    stale.UpdateInterval = 5;
    std::ofstream file(path + L".tmp", std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(&stale), sizeof(stale));
}

void WriteLegacyV3File(const std::wstring& path, const ConfigV4Disk& config)
{
    struct Header
    {
        std::uint32_t magic;
        std::uint32_t version;
    } header{ CONFIG_FILE_MAGIC, CONFIG_FILE_VERSION_V3 };

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(&config), sizeof(config));
}

void AssertV4Header(const std::wstring& path)
{
    struct Header
    {
        std::uint32_t magic;
        std::uint32_t version;
        std::uint32_t payloadSize;
    } header{};

    std::ifstream file(path, std::ios::binary);
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    assert(file.gcount() == sizeof(header));
    assert(header.magic == CONFIG_FILE_MAGIC);
    assert(header.version == CONFIG_FILE_VERSION_V4);
    assert(header.payloadSize == sizeof(ConfigV4Disk));
}
}

int main()
{
    const std::wstring configPath = MakeTestPath();
    RemoveTestFiles(configPath);

    ConfigV4Disk first{};
    first.UpdateInterval = 2;
    assert(WriteConfigAtomically(configPath.c_str(), &first, sizeof(first)));

    ConfigV4Disk second{};
    second.UpdateInterval = 4;
    assert(WriteConfigAtomically(configPath.c_str(), &second, sizeof(second)));
    AssertV4Header(configPath);

    // A successful replacement must preserve the previously valid snapshot.
    ConfigV4Disk loaded{};
    assert(ReadConfigWithBackup(configPath.c_str(), &loaded, sizeof(loaded)));
    assert(loaded.UpdateInterval == 4);

    // A corrupt primary and a stale temporary file must fall back to the backup.
    WriteIncompleteFile(configPath);
    WriteStaleTemporaryFile(configPath);
    assert(ReadConfigWithBackup(configPath.c_str(), &loaded, sizeof(loaded)));
    assert(loaded.UpdateInterval == 2);

    // Existing v3 files use the same field sequence and must be normalized during migration.
    ConfigV4Disk legacy{};
    legacy.UpdateInterval = 9;
    legacy.ManualDuty[0] = 125;
    WriteLegacyV3File(configPath, legacy);
    assert(ReadConfigWithBackup(configPath.c_str(), &loaded, sizeof(loaded)));
    assert(loaded.UpdateInterval == 5);
    assert(loaded.ManualDuty[0] == 100);

    RemoveTestFiles(configPath);
    return 0;
}
