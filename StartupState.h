#pragma once

enum class StartupState { UiReady, CoreStarting, SelfChecking, CoreReady, CoreFailed, Exiting };

inline bool CanEnableTakeover(StartupState state)
{
    return state == StartupState::CoreReady;
}

inline bool CanWriteFans(StartupState state, bool userAuthorized)
{
    return state == StartupState::CoreReady && userAuthorized;
}

inline bool CanEnableForcedCooling(StartupState state, bool userAuthorized)
{
    return CanWriteFans(state, userAuthorized);
}
