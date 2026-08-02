#include "../ControlVerificationState.h"
#include <cassert>

int main()
{
    assert(EvaluateControlVerification(StartupState::CoreReady, false, false, false) == ControlVerification::BiosControl);
    assert(EvaluateControlVerification(StartupState::CoreReady, true, false, false) == ControlVerification::RequestingTakeover);
    assert(EvaluateControlVerification(StartupState::CoreReady, true, true, true) == ControlVerification::Active);
    assert(EvaluateControlVerification(StartupState::CoreReady, true, true, false) == ControlVerification::Fault);
    assert(EvaluateControlVerification(StartupState::CoreFailed, true, true, true) == ControlVerification::Fault);
    return 0;
}
