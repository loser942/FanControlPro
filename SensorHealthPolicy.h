#pragma once

class SensorHealthPolicy
{
public:
    explicit SensorHealthPolicy(unsigned int failureLimit = 3)
        : failureLimit(failureLimit == 0 ? 1 : failureLimit)
        , consecutiveFailures(0)
        , faulted(false)
    {
    }

    bool RecordCycle(bool cpuValid, bool gpuValid)
    {
        if (faulted)
            return false;

        if (cpuValid && gpuValid)
        {
            consecutiveFailures = 0;
            return true;
        }

        ++consecutiveFailures;
        if (consecutiveFailures >= failureLimit)
            faulted = true;

        return !faulted;
    }

    bool IsFaulted() const
    {
        return faulted;
    }

    void Reset()
    {
        consecutiveFailures = 0;
        faulted = false;
    }

private:
    unsigned int failureLimit;
    unsigned int consecutiveFailures;
    bool faulted;
};
