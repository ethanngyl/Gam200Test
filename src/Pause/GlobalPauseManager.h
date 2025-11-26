/*
===============================================================================
 File:          GlobalPauseManager.h
 Author:        GE YONGQI
 Date:          2025-11-22
 ------------------------------------------------------------------------------
  Global Pause Manager

  Simple global pause state that can be accessed from anywhere
  Used to pause the entire engine update loop
===============================================================================
*/

#pragma once

namespace GlobalPause
{
    // ========================================================================
    // GLOBAL PAUSE STATE
    // ========================================================================

    /**
     * @brief Check if game is paused
     */
    bool IsPaused();

    /**
     * @brief Set pause state
     */
    void SetPaused(bool paused);

    /**
     * @brief Toggle pause state
     */
    void Toggle();

} // namespace GlobalPause