/*
===============================================================================
 File:          TimeConstants.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Purpose:       Centralized time constants for the engine

 Usage:
    #include "TimeConstants.h"

    // In any system:
    position += velocity * FIXED_DT;
===============================================================================
*/

#pragma once

namespace Framework
{
    namespace Time
    {
        // Target framerate: 60 FPS
        constexpr double FIXED_DT = 1.0 / 60.0;         // 0.016666... seconds
        constexpr float  FIXED_DT_F = 1.0f / 60.0f;     // Float version
        constexpr int    TARGET_FPS = 60;

        // Maximum allowed delta time (to prevent "spiral of death")
        constexpr double MAX_DELTA_TIME = 0.25;         // 250ms

        // Maximum physics steps per frame
        constexpr int MAX_PHYSICS_STEPS = 5;
    }
}