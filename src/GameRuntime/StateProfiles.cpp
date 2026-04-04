/*
===============================================================================
 File:          StateProfiles.cpp
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 StateProfiles Implementation

 Overview:
    The StateProfiles module provides runtime functionality for the game engine.

===============================================================================
*/
#include "Precompiled.h"
#include "StateProfiles.h"

#include "GameStateList.h"

namespace {
    struct StateProfileEntry {
        int state;
        Framework::StateProfiles::Kind kind;
    };

    constexpr StateProfileEntry kStateProfiles[] = {
        { mainMenu, Framework::StateProfiles::Kind::StandardScripted },
        { settingsMenu, Framework::StateProfiles::Kind::StandardScripted },
        { Level_select, Framework::StateProfiles::Kind::StandardScripted },
        { LEVEL_2, Framework::StateProfiles::Kind::Level2Lua },
        { LEVEL_3, Framework::StateProfiles::Kind::Level3Hybrid },
        { LEVEL_END, Framework::StateProfiles::Kind::StandardScripted },
        { TUTORIAL, Framework::StateProfiles::Kind::StandardScripted },
        { CONTROL, Framework::StateProfiles::Kind::StandardScripted },
        { CONTROL2, Framework::StateProfiles::Kind::StandardScripted },
        { SKILL_SETS, Framework::StateProfiles::Kind::StandardScripted },
        { WIN_SCREEN, Framework::StateProfiles::Kind::StandardScripted },
        { LOSE_SCREEN, Framework::StateProfiles::Kind::StandardScripted },
        { DEMO_BRIDGE, Framework::StateProfiles::Kind::StandardScripted },
        { Copyright, Framework::StateProfiles::Kind::StandardScripted },
        { GS_QUIT, Framework::StateProfiles::Kind::Quit },
        { GS_RESTART, Framework::StateProfiles::Kind::Restart }
    };
}

namespace Framework::StateProfiles {

/**
 * @brief Returns the profile kind associated with a state id.
 * @param state State identifier to classify.
 * @return Matching state profile kind, or Unknown when unmapped.
 */
Kind GetKind(int state)
{
    for (const auto& entry : kStateProfiles) {
        if (entry.state == state) {
            return entry.kind;
        }
    }
    return Kind::Unknown;
}

} // namespace Framework::StateProfiles
