/*
===============================================================================
 File:          Level2StateHooks.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 Level2StateHooks Header

 Overview:
    The Level2StateHooks module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

namespace Framework::Level2StateHooks {

/**
 * @brief Loads resources and script content for Level 2.
 */
void OnLoad();

/**
 * @brief Performs Level 2 initialization after load.
 */
void OnInitialize();

/**
 * @brief Updates Level 2 gameplay logic each fixed step.
 */
void OnUpdate();

/**
 * @brief Draws Level 2 frame content.
 */
void OnDraw();

/**
 * @brief Frees Level 2 runtime resources prior to unload.
 */
void OnFree();

/**
 * @brief Handles final Level 2 unload cleanup.
 */
void OnUnload();

} // namespace Framework::Level2StateHooks
