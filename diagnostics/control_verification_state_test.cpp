#include "../ControlVerificationState.h"
#include "../TakeoverVerificationPolicy.h"
#include <cassert>

int main()
{
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

    // A user can explicitly end a faulted takeover session and start a new one.
    policy.Reset();
    assert(!policy.IsFaulted());
    policy.RecordWrite(80, 80, 800);
    policy.RecordReadback(80, 80, 900);
    assert(policy.State() == ControlVerification::RequestingTakeover);
    policy.RecordReadback(80, 80, 1000);
    assert(policy.State() == ControlVerification::Active);

    TakeoverVerificationPolicy reclaimWindowPolicy(15, 3000, 2, 5000, 3);
    assert(reclaimWindowPolicy.CanReclaim(100));
    assert(reclaimWindowPolicy.CanReclaim(200));
    assert(reclaimWindowPolicy.CanReclaim(5200));
    assert(reclaimWindowPolicy.CanReclaim(5300));
    assert(!reclaimWindowPolicy.IsFaulted());

    TakeoverVerificationPolicy targetChangePolicy(15, 3000, 2, 5000, 3);
    targetChangePolicy.RecordWrite(50, 50, 100);
    targetChangePolicy.RecordReadback(50, 50, 200);
    assert(targetChangePolicy.State() == ControlVerification::RequestingTakeover);
    targetChangePolicy.RecordReadback(50, 50, 300);
    assert(targetChangePolicy.State() == ControlVerification::Active);
    targetChangePolicy.RecordWrite(70, 70, 300);
    assert(targetChangePolicy.State() == ControlVerification::RequestingTakeover);
    targetChangePolicy.RecordReadback(70, 70, 400);
    assert(targetChangePolicy.State() == ControlVerification::RequestingTakeover);
    targetChangePolicy.RecordReadback(70, 70, 500);
    assert(targetChangePolicy.State() == ControlVerification::Active);

    TakeoverVerificationPolicy timeoutPolicy(15, 3000, 2, 5000, 3);
    timeoutPolicy.RecordWrite(50, 50, 100);
    timeoutPolicy.RecordReadback(50, 50, 200);
    assert(timeoutPolicy.State() == ControlVerification::RequestingTakeover);
    timeoutPolicy.RecordReadback(50, 50, 300);
    assert(timeoutPolicy.State() == ControlVerification::Active);
    timeoutPolicy.RecordReadback(50, 50, 3201);
    assert(timeoutPolicy.State() == ControlVerification::RequestingTakeover);
    timeoutPolicy.RecordWrite(50, 50, 3300);
    timeoutPolicy.RecordReadback(50, 50, 3400);
    assert(timeoutPolicy.State() == ControlVerification::RequestingTakeover);
    timeoutPolicy.RecordReadback(50, 50, 3500);
    assert(timeoutPolicy.State() == ControlVerification::Active);

    TakeoverVerificationPolicy deviationPolicy(15, 3000, 2, 5000, 3);
    deviationPolicy.RecordWrite(50, 50, 100);
    deviationPolicy.RecordReadback(50, 50, 200);
    assert(deviationPolicy.State() == ControlVerification::RequestingTakeover);
    deviationPolicy.RecordReadback(50, 50, 300);
    assert(deviationPolicy.State() == ControlVerification::Active);
    deviationPolicy.RecordReadback(70, 50, 300);
    assert(deviationPolicy.State() == ControlVerification::RequestingTakeover);
    deviationPolicy.RecordReadback(50, 50, 400);
    assert(deviationPolicy.State() == ControlVerification::RequestingTakeover);
    deviationPolicy.RecordReadback(50, 50, 500);
    assert(deviationPolicy.State() == ControlVerification::Active);
    return 0;
}
