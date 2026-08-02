#pragma once

#include <string>
#include <utility>
#include <vector>

struct StartupCheckResult final
{
    const std::vector<std::wstring> blockingFaults;
    const std::vector<std::wstring> warnings;
    const unsigned int consecutiveValidTemperatureSamples;
    const bool takeoverAllowed;
    const std::wstring statusMessage;

    StartupCheckResult(std::vector<std::wstring> faults, std::vector<std::wstring> reportedWarnings,
        unsigned int validSampleCount, bool allowTakeover, std::wstring message)
        : blockingFaults(std::move(faults)),
          warnings(std::move(reportedWarnings)),
          consecutiveValidTemperatureSamples(validSampleCount),
          takeoverAllowed(allowTakeover),
          statusMessage(std::move(message))
    {
    }
};

class StartupSelfCheckPolicy
{
public:
    StartupCheckResult Evaluate(bool coreInitialized, int cpuTemperature, int gpuTemperature,
        int cpuRpm, int gpuRpm, bool gpuAvailable)
    {
        std::vector<std::wstring> warnings;
        if (cpuRpm <= 0)
        {
            warnings.push_back(L"CPU 风扇转速不可用");
        }
        if (!gpuAvailable)
        {
            warnings.push_back(L"GPU 不可用");
        }
        else if (gpuRpm <= 0)
        {
            warnings.push_back(L"GPU 风扇转速不可用");
        }

        if (!coreInitialized)
        {
            locked_ = true;
            consecutiveValidTemperatureSamples_ = 0;
            return MonitoringOnly({ L"核心初始化失败" }, std::move(warnings), L"核心初始化失败，仅监控模式");
        }

        if (!IsReasonableTemperature(cpuTemperature) || !IsReasonableTemperature(gpuTemperature))
        {
            locked_ = true;
            consecutiveValidTemperatureSamples_ = 0;
            return MonitoringOnly({ L"温度读数异常" }, std::move(warnings), L"温度读数异常，仅监控模式");
        }

        if (locked_)
        {
            return MonitoringOnly({ L"启动自检已锁定" }, std::move(warnings), L"启动自检失败，仅监控模式");
        }

        ++consecutiveValidTemperatureSamples_;
        const bool takeoverAllowed = consecutiveValidTemperatureSamples_ >= RequiredValidTemperatureSamples;
        const std::wstring status = takeoverAllowed
            ? L"启动自检完成，可启用接管"
            : L"正在进行启动自检，仅监控模式";
        return StartupCheckResult({}, std::move(warnings), consecutiveValidTemperatureSamples_, takeoverAllowed, status);
    }

private:
    static constexpr unsigned int RequiredValidTemperatureSamples = 3;
    static constexpr int MaximumReasonableTemperature = 120;

    static bool IsReasonableTemperature(int temperature)
    {
        return temperature > 0 && temperature <= MaximumReasonableTemperature;
    }

    StartupCheckResult MonitoringOnly(std::vector<std::wstring> faults, std::vector<std::wstring> warnings,
        std::wstring status) const
    {
        return StartupCheckResult(std::move(faults), std::move(warnings),
            consecutiveValidTemperatureSamples_, false, std::move(status));
    }

    bool locked_ = false;
    unsigned int consecutiveValidTemperatureSamples_ = 0;
};
