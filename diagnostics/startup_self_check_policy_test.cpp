#include "../StartupSelfCheckPolicy.h"

#include <cassert>

int main()
{
    StartupSelfCheckPolicy policy;

    const StartupCheckResult firstResult = policy.Evaluate(true, 45, 52, 1800, 1600, true);
    assert(firstResult.consecutiveValidTemperatureSamples == 1);
    assert(!firstResult.takeoverAllowed);
    assert(firstResult.blockingFaults.empty());

    const StartupCheckResult secondResult = policy.Evaluate(true, 46, 53, 1800, 1600, true);
    assert(secondResult.consecutiveValidTemperatureSamples == 2);
    assert(!secondResult.takeoverAllowed);

    const StartupCheckResult thirdResult = policy.Evaluate(true, 47, 54, 1800, 1600, true);
    assert(thirdResult.consecutiveValidTemperatureSamples == 3);
    assert(thirdResult.takeoverAllowed);
    assert(thirdResult.blockingFaults.empty());

    StartupSelfCheckPolicy invalidTemperaturePolicy;
    const StartupCheckResult invalidTemperatureResult = invalidTemperaturePolicy.Evaluate(true, 0, 54, 1800, 1600, true);
    assert(!invalidTemperatureResult.takeoverAllowed);
    assert(!invalidTemperatureResult.blockingFaults.empty());
    assert(invalidTemperatureResult.statusMessage == L"温度读数异常，仅监控模式");
    const StartupCheckResult lockedResult = invalidTemperaturePolicy.Evaluate(true, 47, 54, 1800, 1600, true);
    assert(!lockedResult.takeoverAllowed);
    assert(!lockedResult.blockingFaults.empty());
    assert(lockedResult.consecutiveValidTemperatureSamples == 0);

    StartupSelfCheckPolicy warningPolicy;
    const StartupCheckResult warningResult = warningPolicy.Evaluate(true, 47, 54, 0, 0, false);
    assert(warningResult.blockingFaults.empty());
    assert(!warningResult.warnings.empty());
    assert(!warningResult.takeoverAllowed);

    StartupSelfCheckPolicy coreFailurePolicy;
    const StartupCheckResult coreFailureResult = coreFailurePolicy.Evaluate(false, 47, 54, 1800, 1600, true);
    assert(!coreFailureResult.takeoverAllowed);
    assert(!coreFailureResult.blockingFaults.empty());
    assert(coreFailureResult.statusMessage == L"核心初始化失败，仅监控模式");

    return 0;
}
