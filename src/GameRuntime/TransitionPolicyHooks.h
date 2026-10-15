/*
===============================================================================
 File:          TransitionPolicyHooks.h
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2026-04-05
 Contribution:  100%
 ------------------------------------------------------------------------------
 TransitionPolicyHooks Header

 Overview:
    The TransitionPolicyHooks module provides runtime functionality for the game engine.

===============================================================================
*/
#pragma once

namespace Framework::TransitionPolicyHooks {

/**
 * @brief Sets the next state while honoring editor/play transition policy.
 * @param nextState State identifier to transition into.
 */
void SetNextStateWithPolicy(int nextState);

} // namespace Framework::TransitionPolicyHooks
