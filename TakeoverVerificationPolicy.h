#pragma once

#include "ControlVerificationState.h"

class TakeoverVerificationPolicy
{
public:
    TakeoverVerificationPolicy(int threshold, unsigned long long readbackTimeoutMs,
        int requiredMatches, unsigned long long reclaimWindowMs, int faultOnReclaim)
        : threshold_(threshold), readbackTimeoutMs_(readbackTimeoutMs),
          requiredMatches_(requiredMatches), reclaimWindowMs_(reclaimWindowMs),
          faultOnReclaim_(faultOnReclaim), state_(ControlVerification::RequestingTakeover)
    {
    }

    void RecordWrite(int cpuTarget, int gpuTarget, unsigned long long nowMs)
    {
        if (!hasWrite_ || cpuTarget != cpuTarget_ || gpuTarget != gpuTarget_)
        {
            cpuTarget_ = cpuTarget;
            gpuTarget_ = gpuTarget;
            matchingReadbacks_ = 0;
            SetRequesting();
        }

        hasWrite_ = true;
        lastWriteMs_ = nowMs;
    }

    void RecordReadback(int cpuDuty, int gpuDuty, unsigned long long nowMs)
    {
        if (state_ == ControlVerification::Fault)
            return;

        if (!hasWrite_ || nowMs - lastWriteMs_ > readbackTimeoutMs_ ||
            !Matches(cpuDuty, cpuTarget_) || !Matches(gpuDuty, gpuTarget_))
        {
            matchingReadbacks_ = 0;
            SetRequesting();
            return;
        }

        ++matchingReadbacks_;
        state_ = matchingReadbacks_ >= requiredMatches_
            ? ControlVerification::Active
            : ControlVerification::RequestingTakeover;
    }

    bool CanReclaim(unsigned long long nowMs)
    {
        if (state_ == ControlVerification::Fault)
            return false;

        if (reclaimCount_ == 0 || nowMs - reclaimWindowStartMs_ >= reclaimWindowMs_)
        {
            reclaimWindowStartMs_ = nowMs;
            reclaimCount_ = 0;
        }

        ++reclaimCount_;
        if (reclaimCount_ >= faultOnReclaim_)
        {
            state_ = ControlVerification::Fault;
            return false;
        }

        return true;
    }

    bool IsFaulted() const
    {
        return state_ == ControlVerification::Fault;
    }

    void Reset()
    {
        cpuTarget_ = 0;
        gpuTarget_ = 0;
        matchingReadbacks_ = 0;
        lastWriteMs_ = 0;
        reclaimWindowStartMs_ = 0;
        reclaimCount_ = 0;
        hasWrite_ = false;
        state_ = ControlVerification::RequestingTakeover;
    }

    ControlVerification State() const
    {
        return state_;
    }

private:
    bool Matches(int actualDuty, int targetDuty) const
    {
        const long long difference = static_cast<long long>(actualDuty) - targetDuty;
        return difference >= -threshold_ && difference <= threshold_;
    }

    void SetRequesting()
    {
        if (state_ != ControlVerification::Fault)
            state_ = ControlVerification::RequestingTakeover;
    }

    int threshold_;
    unsigned long long readbackTimeoutMs_;
    int requiredMatches_;
    unsigned long long reclaimWindowMs_;
    int faultOnReclaim_;
    int cpuTarget_ = 0;
    int gpuTarget_ = 0;
    int matchingReadbacks_ = 0;
    unsigned long long lastWriteMs_ = 0;
    unsigned long long reclaimWindowStartMs_ = 0;
    int reclaimCount_ = 0;
    bool hasWrite_ = false;
    ControlVerification state_;
};
