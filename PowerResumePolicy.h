#pragma once

class PowerResumePolicy
{
public:
    void BeginRecovery(bool autoTakeoverEnabled)
    {
        selfChecking_ = true;
        writeAllowed_ = false;
        autoTakeoverEnabled_ = autoTakeoverEnabled;
        autoTakeoverRequestPending_ = false;
    }

    void CompleteSelfCheck(bool passed)
    {
        selfChecking_ = false;
        writeAllowed_ = passed;
        autoTakeoverRequestPending_ = passed && autoTakeoverEnabled_;
    }

    bool IsSelfChecking() const
    {
        return selfChecking_;
    }

    bool CanWriteFans() const
    {
        return writeAllowed_;
    }

    bool ConsumeAutoTakeoverRequest()
    {
        const bool requested = autoTakeoverRequestPending_;
        autoTakeoverRequestPending_ = false;
        return requested;
    }

private:
    bool selfChecking_ = false;
    bool writeAllowed_ = true;
    bool autoTakeoverEnabled_ = false;
    bool autoTakeoverRequestPending_ = false;
};
