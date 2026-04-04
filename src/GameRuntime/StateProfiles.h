/*
===============================================================================
 File:          StateProfiles.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 StateProfiles Header

 Overview:
    The StateProfiles module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

namespace Framework::StateProfiles {

enum class Kind {
    StandardScripted,
    Level2Lua,
    Level3Hybrid,
    Restart,
    Quit,
    Unknown
};

/**
 * @brief Retrieves the state profile classification for a state id.
 * @param state State identifier to classify.
 * @return Profile kind for the state, or Unknown when not listed.
 */
Kind GetKind(int state);

} // namespace Framework::StateProfiles
