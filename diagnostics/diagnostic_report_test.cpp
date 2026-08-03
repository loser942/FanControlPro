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
    input.logLines.push_back(L"额外日志 C:\\Users\\Alice Example\\secret.txt");
    input.logLines.push_back(L"Power resume self-check started");
    input.logLines.push_back(L"Automatic takeover requested after self-check");

    const std::string report = BuildDiagnosticReportUtf8(input);
    assert(report.find("FanControl Pro v8") != std::string::npos);
    assert(report.find("Windows 11 23H2") != std::string::npos);
    assert(report.find("\xE5\x90\xAF\xE5\x8A\xA8\xE8\x87\xAA\xE6\xA3\x80") != std::string::npos);
    assert(report.find("\xE6\x97\xA5\xE5\xBF\x97[004]") == std::string::npos);
    assert(report.find("\xE6\x97\xA5\xE5\xBF\x97[008]") != std::string::npos);
    assert(report.find("C:\\Users\\alice") == std::string::npos);
    assert(report.find("Alice Example") == std::string::npos);
    assert(report.find("secret.txt") == std::string::npos);
    assert(report.find("<\xE5\xB7\xB2\xE9\x9A\x90\xE8\x97\x8F\xE8\xB7\xAF\xE5\xBE\x84>") != std::string::npos);
    assert(report.find("Power resume self-check started") != std::string::npos);
    assert(report.find("Automatic takeover requested after self-check") != std::string::npos);
    return 0;
}
