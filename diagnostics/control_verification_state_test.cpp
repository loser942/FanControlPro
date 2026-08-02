#include "../ControlVerificationState.h"
#include "../TakeoverVerificationPolicy.h"
#include <cassert>

int main()
{
    assert(EvaluateControlVerification(StartupState::CoreReady, false, false, false) == ControlVerification::BiosControl);
    assert(EvaluateControlVerification(StartupState::CoreReady, true, false, false) == ControlVerification::RequestingTakeover);
    assert(EvaluateControlVerification(StartupState::CoreReady, true, true, true) == ControlVerification::Active);
    assert(EvaluateControlVerification(StartupState::CoreReady, true, true, false) == ControlVerification::Fault);
    assert(EvaluateControlVerification(StartupState::CoreFailed, true, true, true) == ControlVerification::Fault);

    TakeoverVerificationPolicy policy(15, 3000, 2, 5000, 3);
    policy.RecordWrite(60, 65, 100);
    assert(policy.State() == ControlVerification::RequestingTakeover);
    policy.RecordReadback(62, 63, 200);
    assert(policy.State() == ControlVerification::RequestingTakeover);
    policy.RecordReadback(61, 64, 300);
    assert(policy.State() == ControlVerification::Active);

    policy.RecordWrite(80, 80, 400);
    assert(policy.State() == ControlVerification::RequestingTakeover);
    policy.RecordReadback(20, 20, 500);
    assert(policy.State() == ControlVerification::RequestingTakeover);
    assert(policy.CanReclaim(500));
    assert(policy.CanReclaim(600));
    assert(!policy.CanReclaim(700));
    assert(policy.IsFaulted());

    TakeoverVerificationPolicy targetChangePolicy(15, 3000, 2, 5000, 3);
    targetChangePolicy.RecordWrite(50, 50, 100);
    targetChangePolicy.RecordReadback(50, 50, 200);
    targetChangePolicy.RecordWrite(70, 70, 300);
    targetChangePolicy.RecordReadback(50, 50, 400);
    assert(targetChangePolicy.State() == ControlVerification::RequestingTakeover);

    TakeoverVerificationPolicy timeoutPolicy(15, 3000, 2, 5000, 3);
    timeoutPolicy.RecordWrite(50, 50, 100);
    timeoutPolicy.RecordReadback(50, 50, 3201);
    assert(timeoutPolicy.State() == ControlVerification::RequestingTakeover);

    TakeoverVerificationPolicy deviationPolicy(15, 3000, 2, 5000, 3);
    deviationPolicy.RecordWrite(50, 50, 100);
    deviationPolicy.RecordReadback(50, 50, 200);
    deviationPolicy.RecordReadback(70, 50, 300);
    assert(deviationPolicy.State() == ControlVerification::RequestingTakeover);
    return 0;
}
