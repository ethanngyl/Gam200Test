/*
===============================================================================
 File:          GlobalPauseManager.h
 Author:        Padilla Carl Jameson
 Date:          2025-11-22
 Contribution:  100%
 ------------------------------------------------------------------------------
  Global Pause Manager

  Simple global pause state that can be accessed from anywhere
  Used to pause the entire engine update loop

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
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