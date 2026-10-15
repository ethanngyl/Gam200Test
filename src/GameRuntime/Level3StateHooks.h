/*
===============================================================================
 File:          Level3StateHooks.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 Level3StateHooks Header

 Overview:
    The Level3StateHooks module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

namespace Framework::Level3StateHooks {

/**
 * @brief Initializes Level 3 hybrid runtime state.
 */
void OnInitialize();

/**
 * @brief Updates Level 3 runtime logic each frame.
 */
void OnUpdate();

/**
 * @brief Frees Level 3 resources and transient entities.
 */
void OnFree();

} // namespace Framework::Level3StateHooks
