/*
===============================================================================
 File:          StateDispatchTable.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 StateDispatchTable Implementation

 Overview:
    The StateDispatchTable module provides runtime functionality for the game engine.

===============================================================================
*/
#include "Precompiled.h"
#include "StateDispatchTable.h"

#include "GameStateManager.h"
#include "Level2StateHooks.h"
#include "Level3StateHooks.h"
#include "SpecialStateHooks.h"
#include "StandardScriptedStateHooks.h"
#include "StateProfiles.h"

namespace {
    /**
     * @brief Binds Level 2 specific lifecycle callbacks into the GSM dispatch pointers.
     */
    void ConfigureLevel2State()
    {
        fpLoad = Framework::Level2StateHooks::OnLoad;
        fpInitialize = Framework::Level2StateHooks::OnInitialize;
        fpUpdate = Framework::Level2StateHooks::OnUpdate;
        fpDraw = Framework::Level2StateHooks::OnDraw;
        fpFree = Framework::Level2StateHooks::OnFree;
        fpUnload = Framework::Level2StateHooks::OnUnload;
        LOG_INFO("GSM", "State changed to: LEVEL_2 (Lua)");
    }
}

namespace Framework::StateDispatchTable {

/**
 * @brief Configures callback function pointers for the given state id.
 * @param state State identifier that determines callback profile.
 */
void ConfigureState(int state)
{
    switch (Framework::StateProfiles::GetKind(state)) {
    case Framework::StateProfiles::Kind::StandardScripted:
        Framework::StandardScriptedStateHooks::Configure(state);
        return;

    case Framework::StateProfiles::Kind::Level2Lua:
        ConfigureLevel2State();
        return;

    case Framework::StateProfiles::Kind::Level3Hybrid:
        Framework::StandardScriptedStateHooks::Configure(state);
        fpInitialize = Framework::Level3StateHooks::OnInitialize;
        fpUpdate = Framework::Level3StateHooks::OnUpdate;
        fpFree = Framework::Level3StateHooks::OnFree;
        return;

    case Framework::StateProfiles::Kind::Restart:
        Framework::SpecialStateHooks::ConfigureRestartState();
        return;

    case Framework::StateProfiles::Kind::Quit:
        Framework::SpecialStateHooks::ConfigureQuitState();
        return;

    case Framework::StateProfiles::Kind::Unknown:
    default:
        Framework::SpecialStateHooks::ConfigureUnknownState(state);
        return;
    }
}

} // namespace Framework::StateDispatchTable
