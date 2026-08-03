#include "../PowerResumePolicy.h"

#include <cassert>

int main()
{
    PowerResumePolicy automaticRecovery;
    automaticRecovery.BeginRecovery(true);
    assert(automaticRecovery.IsSelfChecking());
    assert(!automaticRecovery.CanWriteFans());
    assert(!automaticRecovery.ConsumeAutoTakeoverRequest());

    automaticRecovery.CompleteSelfCheck(true);
    assert(!automaticRecovery.IsSelfChecking());
    assert(automaticRecovery.CanWriteFans());
    assert(automaticRecovery.ConsumeAutoTakeoverRequest());
    assert(!automaticRecovery.ConsumeAutoTakeoverRequest());

    PowerResumePolicy manualRecovery;
    manualRecovery.BeginRecovery(false);
    manualRecovery.CompleteSelfCheck(true);
    assert(manualRecovery.CanWriteFans());
    assert(!manualRecovery.ConsumeAutoTakeoverRequest());

    PowerResumePolicy failedRecovery;
    failedRecovery.BeginRecovery(true);
    failedRecovery.CompleteSelfCheck(false);
    assert(!failedRecovery.CanWriteFans());
    assert(!failedRecovery.ConsumeAutoTakeoverRequest());

    // Simulate long-running use with repeated sleep/resume and mixed outcomes.
    PowerResumePolicy soakPolicy;
    for (int cycle = 0; cycle < 10000; ++cycle)
    {
        const bool automatic = (cycle % 2) == 0;
        const bool selfCheckPassed = (cycle % 7) != 0;
        soakPolicy.BeginRecovery(automatic);
        assert(soakPolicy.IsSelfChecking());
        assert(!soakPolicy.CanWriteFans());
        assert(!soakPolicy.ConsumeAutoTakeoverRequest());

        soakPolicy.CompleteSelfCheck(selfCheckPassed);
        assert(soakPolicy.CanWriteFans() == selfCheckPassed);
        assert(soakPolicy.ConsumeAutoTakeoverRequest() == (automatic && selfCheckPassed));
        assert(!soakPolicy.ConsumeAutoTakeoverRequest());
    }

    return 0;
}
