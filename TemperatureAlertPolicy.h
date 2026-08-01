#pragma once

struct TemperatureAlertDecision
{
    bool alertActive;
    bool shouldNotify;
};

class TemperatureAlertPolicy
{
public:
    TemperatureAlertDecision Evaluate(int cpuTemp, int gpuTemp, int threshold,
        bool notificationsEnabled, unsigned long long nowMs, unsigned long long cooldownMs)
    {
        const bool overThreshold = cpuTemp >= threshold || gpuTemp >= threshold;
        const bool belowResetTemperature = cpuTemp < threshold - 5 && gpuTemp < threshold - 5;

        if (alertActive_ && belowResetTemperature)
        {
            alertActive_ = false;
        }

        bool shouldNotify = false;
        if (!alertActive_ && overThreshold)
        {
            alertActive_ = true;
            shouldNotify = notificationsEnabled &&
                (!hasNotified_ || nowMs - lastNotificationMs_ >= cooldownMs);
            if (shouldNotify)
            {
                lastNotificationMs_ = nowMs;
                hasNotified_ = true;
            }
        }

        return { alertActive_, shouldNotify };
    }

private:
    bool alertActive_ = false;
    bool hasNotified_ = false;
    unsigned long long lastNotificationMs_ = 0;
};
