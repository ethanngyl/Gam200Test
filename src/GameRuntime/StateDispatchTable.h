/*
===============================================================================
 File:          StateDispatchTable.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 StateDispatchTable Header

 Overview:
    The StateDispatchTable module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

namespace Framework::StateDispatchTable {

/**
 * @brief Configures dispatch callbacks for a state id.
 * @param state State identifier to configure.
 */
void ConfigureState(int state);

} // namespace Framework::StateDispatchTable
