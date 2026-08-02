#include "../SensorHealthPolicy.h"

#include <cassert>

int main()
{
    SensorHealthPolicy policy(3);
    assert(policy.RecordCycle(true, true));
    assert(policy.RecordCycle(false, true));
    assert(policy.RecordCycle(false, true));
    assert(!policy.RecordCycle(false, true));
    assert(policy.IsFaulted());
    policy.Reset();
    assert(!policy.IsFaulted());
    assert(policy.RecordCycle(true, true));
    return 0;
}
