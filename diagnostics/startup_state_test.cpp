#include "../StartupState.h"
#include <cassert>

int main()
{
    assert(!CanEnableTakeover(StartupState::UiReady));
    assert(!CanEnableTakeover(StartupState::CoreStarting));
    assert(CanEnableTakeover(StartupState::CoreReady));
    assert(!CanEnableTakeover(StartupState::CoreFailed));
    assert(!CanWriteFans(StartupState::CoreReady, false));
    assert(CanWriteFans(StartupState::CoreReady, true));
    return 0;
}
