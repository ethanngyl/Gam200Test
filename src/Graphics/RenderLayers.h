#pragma once

namespace Framework {
    /**
     * @namespace RenderLayers
     * @brief Standard rendering layer constants for Z-ordering
     *
     * Lower layer values render first (behind), higher values render last (in front).
     * Each layer gets depth offset of (layer * 0.001) to prevent Z-fighting.
     */
    namespace RenderLayers {
        constexpr int Background = -1000;    // Skybox, parallax backgrounds
        constexpr int Ground = 0;            // Grid tiles, floor, terrain
        constexpr int Props = 1;             // Static objects, decorations
        constexpr int Items = 1;             // Collectibles, pickups (alias for Props)
        constexpr int Enemies = 2;           // Enemy entities
        constexpr int Characters = 2;        // Characters (alias, same as Enemies)
        constexpr int RangeIndicators = 3;   // Movement/attack range visualizations
        constexpr int Player = 4;            // Player character (on top of enemies)
        constexpr int Projectiles = 5;       // Bullets, projectiles
        constexpr int Effects = 10;          // Particles, VFX
        constexpr int UI = 100;              // Buttons, HUD elements
        constexpr int Overlay = 1000;        // Debug overlays, tooltips

        // Helper to get layer name for debugging/ImGui
        inline const char* GetLayerName(int layer) {
            switch (layer) {
            case Background:        return "Background";
            case Ground:            return "Ground";
            case Props:             return "Props/Items";
            case Enemies:           return "Enemies/Characters";
            case RangeIndicators:   return "Range Indicators";
            case Player:            return "Player";
            case Projectiles:       return "Projectiles";
            case Effects:           return "Effects";
            case UI:                return "UI";
            case Overlay:           return "Overlay";
            default:                return "Custom";
            }
        }
    }
}