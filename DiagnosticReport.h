#pragma once

#include "ControlVerificationState.h"
#include "StartupSelfCheckPolicy.h"

#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

struct DiagnosticReportInput final
{
    std::wstring applicationVersion;
    std::wstring windowsVersion;
    bool runningAsAdministrator;
    bool ecDllAvailable;
    bool driverInitialized;
    StartupCheckResult startupCheck;
    int cpuTemperature;
    int gpuTemperature;
    int cpuRpm;
    int gpuRpm;
    ControlVerification controlVerification;
    unsigned long initError;
    std::vector<std::wstring> logLines;
};

inline std::wstring SanitizeDiagnosticLine(const std::wstring& line)
{
    std::wstring sanitized;
    for (size_t index = 0; index < line.size();)
    {
        const bool startsAbsolutePath = index + 2 < line.size() && iswalpha(line[index]) &&
            line[index + 1] == L':' && (line[index + 2] == L'\\' || line[index + 2] == L'/');
        if (!startsAbsolutePath)
        {
            sanitized.push_back(line[index++]);
            continue;
        }

        size_t pathEnd = index + 3;
        while (pathEnd < line.size() && !iswspace(line[pathEnd]) && line[pathEnd] != L'"' && line[pathEnd] != L'\'')
            ++pathEnd;
        sanitized += L"<已隐藏路径>";
        index = pathEnd;
    }
    return sanitized;
}

inline void AppendUtf8CodePoint(std::string& output, unsigned int codePoint)
{
    if (codePoint <= 0x7F)
        output.push_back(static_cast<char>(codePoint));
    else if (codePoint <= 0x7FF)
    {
        output.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else if (codePoint <= 0xFFFF)
    {
        output.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else
    {
        output.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

inline std::string ToUtf8(const std::wstring& text)
{
    std::string output;
    output.reserve(text.size() * 3);
    for (size_t index = 0; index < text.size(); ++index)
    {
        unsigned int codePoint = static_cast<unsigned int>(text[index]);
        if (codePoint >= 0xD800 && codePoint <= 0xDBFF && index + 1 < text.size())
        {
            const unsigned int lowSurrogate = static_cast<unsigned int>(text[index + 1]);
            if (lowSurrogate >= 0xDC00 && lowSurrogate <= 0xDFFF)
            {
                codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (lowSurrogate - 0xDC00);
                ++index;
            }
        }
        AppendUtf8CodePoint(output, codePoint);
    }
    return output;
}

inline const wchar_t* ToDiagnosticControlStatus(ControlVerification status)
{
    switch (status)
    {
    case ControlVerification::BiosControl: return L"BIOS 控制中";
    case ControlVerification::RequestingTakeover: return L"正在请求接管";
    case ControlVerification::Active: return L"接管已生效";
    case ControlVerification::Fault: return L"接管状态异常";
    }
    return L"未知";
}

inline std::string BuildDiagnosticReportUtf8(const DiagnosticReportInput& input)
{
    std::wstring report;
    report += L"FanControl Pro 本地诊断报告\r\n";
    report += L"程序版本: " + input.applicationVersion + L"\r\n";
    report += L"Windows: " + input.windowsVersion + L"\r\n";
    report += L"管理员权限: " + std::wstring(input.runningAsAdministrator ? L"是" : L"否") + L"\r\n";
    report += L"EC DLL: " + std::wstring(input.ecDllAvailable ? L"可用" : L"不可用") + L"\r\n";
    report += L"驱动初始化: " + std::wstring(input.driverInitialized ? L"成功" : L"失败") + L"\r\n";
    report += L"启动自检: " + input.startupCheck.statusMessage + L"\r\n";
    report += L"CPU/GPU 温度: " + std::to_wstring(input.cpuTemperature) + L" / " + std::to_wstring(input.gpuTemperature) + L" °C\r\n";
    report += L"CPU/GPU RPM: " + std::to_wstring(input.cpuRpm) + L" / " + std::to_wstring(input.gpuRpm) + L"\r\n";
    report += L"接管状态: " + std::wstring(ToDiagnosticControlStatus(input.controlVerification)) + L"\r\n";
    report += L"初始化错误码: " + std::to_wstring(input.initError) + L"\r\n";

    for (const std::wstring& fault : input.startupCheck.blockingFaults)
        report += L"阻断故障: " + fault + L"\r\n";
    for (const std::wstring& warning : input.startupCheck.warnings)
        report += L"警告: " + warning + L"\r\n";

    report += L"最近日志:\r\n";
    const size_t firstLine = input.logLines.size() > 200 ? input.logLines.size() - 200 : 0;
    for (size_t index = firstLine; index < input.logLines.size(); ++index)
        report += SanitizeDiagnosticLine(input.logLines[index]) + L"\r\n";
    return ToUtf8(report);
}
