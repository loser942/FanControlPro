#pragma once

#include "StartupState.h"

enum class ControlVerification { BiosControl, RequestingTakeover, Active, Fault };

inline ControlVerification EvaluateControlVerification(
    StartupState startupState, bool authorized, bool wrote, bool readbackMatches)
{
    if (startupState != StartupState::CoreReady)
        return ControlVerification::Fault;
    if (!authorized)
        return ControlVerification::BiosControl;
    if (!wrote)
        return ControlVerification::RequestingTakeover;
    return readbackMatches ? ControlVerification::Active : ControlVerification::Fault;
}
