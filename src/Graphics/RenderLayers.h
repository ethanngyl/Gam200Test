#pragma once

namespace Framework {
    namespace RenderLayers {
        constexpr int Background = -1000;
        constexpr int Ground = 0;
        constexpr int Props = 1;
        constexpr int Characters = 2;
        constexpr int Projectiles = 3;
        constexpr int Effects = 4;
        constexpr int UI = 5;
        constexpr int Overlay = 1000;

        //// Helper to get layer name for ImGui
        //inline const char* GetLayerName(int layer) {
        //    switch (layer) {
        //    case Background:    return "Background";
        //    case Ground:        return "Ground";
        //    case Props:         return "Props";
        //    case Characters:    return "Characters";
        //    case Projectiles:   return "Projectiles";
        //    case Effects:       return "Effects";
        //    case UI:            return "UI";
        //    case Overlay:       return "Overlay";
        //    default:            return "Custom";
        //    }
        //}
    }
}