/*
===============================================================================
 File:          GlobalPauseManager.cpp
 Author:        Padilla Carl Jameson
 Date:          2025-11-22
 Contribution:  100%
 ------------------------------------------------------------------------------
  Global Pause Manager Implementation
===============================================================================
*/

#include "GlobalPauseManager.h"

namespace GlobalPause
{
    // Private global state
    static bool g_isPaused = false;

    bool IsPaused()
    {
        return g_isPaused;
    }

    void SetPaused(bool paused)
    {
        g_isPaused = paused;
    }

    void Toggle()
    {
        g_isPaused = !g_isPaused;
    }

} // namespace GlobalPause