#include "../StartupState.h"
#include <cassert>

int main()
{
    // States before, after, or failing core startup cannot write fans even when authorized.
    assert(!CanEnableTakeover(StartupState::UiReady));
    assert(!CanEnableTakeover(StartupState::CoreStarting));
    assert(CanEnableTakeover(StartupState::CoreReady));
    assert(!CanEnableTakeover(StartupState::CoreFailed));
    assert(!CanEnableTakeover(StartupState::Exiting));
    assert(!CanWriteFans(StartupState::UiReady, true));
    assert(!CanWriteFans(StartupState::CoreStarting, true));
    assert(!CanWriteFans(StartupState::CoreFailed, true));
    assert(!CanWriteFans(StartupState::Exiting, true));
    assert(!CanWriteFans(StartupState::CoreReady, false));
    assert(CanWriteFans(StartupState::CoreReady, true));
    assert(!CanEnableForcedCooling(StartupState::CoreReady, false));
    assert(CanEnableForcedCooling(StartupState::CoreReady, true));
    return 0;
}
