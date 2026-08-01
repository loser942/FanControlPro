#include "../TemperatureAlertPolicy.h"
#include <cassert>

int main()
{
    TemperatureAlertPolicy policy;

    TemperatureAlertDecision decision = policy.Evaluate(89, 88, 90, true, 0, 600000);
    assert(!decision.alertActive);
    assert(!decision.shouldNotify);

    decision = policy.Evaluate(90, 70, 90, true, 1, 600000);
    assert(decision.alertActive);
    assert(decision.shouldNotify);

    decision = policy.Evaluate(91, 70, 90, true, 2, 600000);
    assert(decision.alertActive);
    assert(!decision.shouldNotify);

    decision = policy.Evaluate(84, 84, 90, true, 3, 600000);
    assert(!decision.alertActive);
    assert(!decision.shouldNotify);

    decision = policy.Evaluate(90, 70, 90, true, 600001, 600000);
    assert(decision.alertActive);
    assert(decision.shouldNotify);

    policy.Evaluate(84, 84, 90, true, 600002, 600000);
    decision = policy.Evaluate(90, 70, 90, false, 1200001, 600000);
    assert(decision.alertActive);
    assert(!decision.shouldNotify);

    return 0;
}
