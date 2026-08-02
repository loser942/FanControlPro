#include "../DiagnosticReport.h"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    StartupSelfCheckPolicy policy;
    const StartupCheckResult first = policy.Evaluate(true, 45, 50, 1800, 1600, true);
    const StartupCheckResult second = policy.Evaluate(true, 46, 51, 1800, 1600, true);
    const StartupCheckResult check = policy.Evaluate(true, 47, 52, 1800, 1600, true);
    assert(!first.takeoverAllowed);
    assert(!second.takeoverAllowed);
    assert(check.takeoverAllowed);

    DiagnosticReportInput input{
        L"FanControl Pro v8",
        L"Windows 11 23H2",
        true,
        true,
        true,
        check,
        47,
        53,
        1800,
        1600,
        ControlVerification::Active,
        0,
        {}
    };
    for (int line = 0; line < 205; ++line)
    {
        const std::wstring marker = line < 10 ? L"00" + std::to_wstring(line) :
            (line < 100 ? L"0" + std::to_wstring(line) : std::to_wstring(line));
        input.logLines.push_back(L"日志[" + marker + L"] C:\\Users\\alice\\FanControlPro.debug.log");
    }

    const std::string report = BuildDiagnosticReportUtf8(input);
    assert(report.find("FanControl Pro v8") != std::string::npos);
    assert(report.find("Windows 11 23H2") != std::string::npos);
    assert(report.find("\xE5\x90\xAF\xE5\x8A\xA8\xE8\x87\xAA\xE6\xA3\x80") != std::string::npos);
    assert(report.find("日志[004]") == std::string::npos);
    assert(report.find("日志[005]") != std::string::npos);
    assert(report.find("C:\\Users\\alice") == std::string::npos);
    assert(report.find("<已隐藏路径>") != std::string::npos);
    return 0;
}
