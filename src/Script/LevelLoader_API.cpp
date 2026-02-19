/*
===============================================================================
File:        LevelLoader_API.cpp
Author:      ETHAN NG, Sim Kah Yan
Email:       n.ethanyongle@digipen.edu; kahyan.sim@digipen.edu
Date:        2026-02-04 (yyyy-mm-dd)
Contribution: ETHAN NG 98%(2758 lines of 2840 total); Sim Kah Yan 2% (82 lines of 2840 total)
-------------------------------------------------------------------------------
Brief:
Level Loader Lua API implementation: C++ bridge to Lua. Static functions
registered to Lua for Audio, UI, Input, Entities, TileMap, Player/Enemy,
Animation, Party, Grid movement, Save/Load, Editor mode, and more.

Overview (ETHAN NG):
  Bridge between C++ engine core and Lua scripting layer. Suite of static
  functions registered to Lua so scripts control Audio, UI, Input, Entities,
  and grid-based gameplay. Uses standard Lua C API (lua_State*); validates
  engine system pointers (Audio, UI, Graphics) before execution; handles
  coordinate conversions (World to Screen, World to Grid); integrates with
  ECS (AP, Transform, Health, etc.).

Details:
  Implements LevelLoader::Lua_* declared in LevelLoader.h. Categories: Audio
  (PlaySound, StopSound, StopAllSounds, UpdateAudio, volume); Camera; Engine;
  ImGui; Pause; UI/Buttons/Text; Input; JSON; TileMap; SpawnSprite,
  SpawnAnimatedSprite, SetSprite*, DestroyEntity, ClearAllEntities; Player/Enemy
  (FindPlayer, GetAllEnemies, SetEnemyTarget, GetCurrentTurn, GetChestProgress,
  enemy turn manager); Animation (config, player load, frame control); Party
  (GetEntityAP, ConsumeEntityAP, SetActiveCharacter, etc.); Script component;
  Grid movement; Save/Load; Procedural map; Entity spawning; Editor mode.
  g_activeCharacterID shared for party system. All callbacks get LevelLoader
  via GetLevelLoader(L) and null-check subsystems.

Notes:
  Compatible with InputSystem. Windows min/max undefined before algorithm.
  Many functions push booleans/integers or return 0/1/2 Lua return values.

Safety:
  Loader and subsystem pointers null-checked before use. Lua args validated
  with luaL_checkstring/luaL_checknumber/luaL_checkinteger where required.
  Errors logged; stack cleaned on failure.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
===============================================================================
*/

#include "Precompiled.h"
#include "LevelLoader.h"
#include "UISystem.h"
#include "Audio/AudioSystem.h"
#include "Audio/AudioLoader.h"
#include "GraphicsSystemV2.h"
#include "Input.h"
#include "LevelLoader_JSON.h"
#include "ImguiSystem.h"
#include "TileMapLoader.h"
#include "Component.h"    // Movement, CircleCollider, AP components
#include "Graphics/RenderComponents.h"  // Renderable component
#include "Pathfinding.h"  // EnemyAI component
#include "Turn.h"         // Turn system
#include "Pause/GlobalPauseManager.h"  // GlobalPause namespace
#include "Grid/GridECS.h" // Grid system functions
#include "PlayerManager.h"
#include "SaveLoadSystem.h"  // JSON Save/Load system
#include "MapGenerator/ProceduralMapLoader.h"
#include <Windows.h>      // For GetTickCount64()    

// Fix for Windows min/max macro conflicts
#include <algorithm>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace Framework {

    // ========================================================================
    // PARTY SYSTEM STATE - Shared between Lua and C++
    // ========================================================================

    // Global variable to track the currently active character for party system
    // Set by Lua's PartyTurnManager, queried by entity scripts
    static uint32_t g_activeCharacterID = 0;

    // ========================================================================
    // TILE TINTING SYSTEM - For PulseTile visual feedback
    // ========================================================================

    struct TileTintState {
        Entity tileEntity;
        glm::vec4 originalTint;
        ULONGLONG expiryTimeMs;
    };

    static std::vector<TileTintState> s_activeTileTints;

    /**
     * @brief Update tile tints and restore expired ones
     * Called every frame from UpdateCurrentLevel
     */
    void UpdateTileTints() {
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return;

        ULONGLONG now = GetTickCount64();

        // Iterate backwards so we can safely erase
        for (int i = static_cast<int>(s_activeTileTints.size()) - 1; i >= 0; --i) {
            auto& state = s_activeTileTints[i];

            // Check if tint has expired
            if (now >= state.expiryTimeMs) {
                // Restore original tint if entity still exists
                if (state.tileEntity.GetID() != INVALID_ENTITY &&
                    em->HasComponent<Renderable>(state.tileEntity)) {
                    auto& renderable = em->GetComponent<Renderable>(state.tileEntity);
                    renderable.tint = state.originalTint;
                }

                // Remove from active list
                s_activeTileTints.erase(s_activeTileTints.begin() + i);
            }
        }
    }

    // ========================================================================
    // AUDIO API
    // ========================================================================

    /**
     * @brief Plays a sound via the AudioSystem from Lua
     * @params soundName (string), loop (boolean)
     * @return boolean (true if successful)
     *
     * Implementation details:
     * - Retrieves the AudioSystem from the LevelLoader instance
     * - Validates arguments using luaL_checkstring and lua_toboolean
     * - Delegates the actual playback to audioSystem->PlaySound
     */
    int LevelLoader::Lua_PlaySound(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->audioSystem) {
            lua_pushboolean(L, false);
            return 1;
        }

        const char* soundName = luaL_checkstring(L, 1);
        bool loop = lua_toboolean(L, 2);

        // Play sound with loop flag
        loader->audioSystem->PlaySound(soundName, loop);

        lua_pushboolean(L, true);
        return 1;
    }

    /**
     * @brief Stops a specific sound (Currently not supported)
     * @params soundName (string)
     *
     * Implementation details:
     * - This function logs a warning because the underlying AudioSystem
     * currently only supports stopping ALL sounds, not individual ones.
     */
    int LevelLoader::Lua_StopSound(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->audioSystem) return 0;

        const char* soundName = luaL_checkstring(L, 1);
        LOG_WARN("LevelLoader", "StopSound('%s') not supported - AudioSystem only has StopAllSounds()", soundName);

        return 0;
    }

    /**
     * @brief Stops all currently playing sounds
     *
     * Implementation details:
     * - Calls audioSystem->StopAllSounds() to halt the master channel group
     */
    int LevelLoader::Lua_StopAllSounds(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->audioSystem) return 0;

        loader->audioSystem->StopAllSounds();
        return 0;
    }

    /**
     * @brief Updates the AudioSystem (usually called once per frame)
     * @params dt (number) - Delta time
     */
    int LevelLoader::Lua_UpdateAudio(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->audioSystem) return 0;

        float dt = luaL_optnumber(L, 1, 0.0f);
        loader->audioSystem->Update(dt);
        return 0;
    }

    /**
     * @brief Sets the master volume for the game
     * @params volume (number) - 0.0 to 1.0
     */
    int LevelLoader::Lua_SetMasterVolume(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->audioSystem) return 0;

        float volume = luaL_checknumber(L, 1);
        loader->audioSystem->SetMasterVolume(volume);
        return 0;
    }

    /**
     * @brief Gets the saved master volume from audio config
     * @return number - The saved master volume (0.0 to 1.0)
     */
    int LevelLoader::Lua_GetMasterVolume(lua_State* L) {
        float volume = AudioLoader::GetSettings().masterVolume;
        lua_pushnumber(L, volume);
        return 1;
    }

    /**
     * @brief Saves the master volume to audio config JSON file
     * @params volume (number) - 0.0 to 1.0
     * @return boolean - True if save succeeded
     */
    int LevelLoader::Lua_SaveMasterVolume(lua_State* L) {
        float volume = luaL_checknumber(L, 1);
        bool success = AudioLoader::SetMasterVolume(volume);
        lua_pushboolean(L, success);
        return 1;
    }

    // ========================================================================
    // UI BUTTON API
    // ========================================================================

    /**
     * @brief Creates a clickable UI button with a callback function
     * @params texture, posX, posY, scaleX, scaleY, callbackName, layer(optional)
     * @return integer (Pointer address of the button, or 0 on failure)
     *
     * Implementation details:
     * - Parses position and scale parameters from Lua
     * - Wraps the Lua callback function string in a C++ lambda
     * - The lambda captures the Lua State pointer to execute the global Lua function when clicked
     * - Calls UISystem::CreateButton, passing the layer explicitly
     * - Returns the button memory address as an ID for future reference
     */
    int LevelLoader::Lua_CreateButton(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->uiSystem) {
            lua_pushinteger(L, 0);
            return 1;
        }

        // Parse parameters (layer is optional, defaults to 10)
        const char* texture = luaL_checkstring(L, 1);
        float posX = (float)luaL_checknumber(L, 2);
        float posY = (float)luaL_checknumber(L, 3);
        float scaleX = (float)luaL_checknumber(L, 4);
        float scaleY = (float)luaL_checknumber(L, 5);
        const char* callbackName = luaL_checkstring(L, 6);

        // Check for optional 7th argument (Layer)
        // If Lua sends 7 args, use the 7th. If Lua sends 6, default to 10.
        int layer = (int)luaL_optinteger(L, 7, 10);

        // Store callback name
        std::string cbName = callbackName;

        // Create button callback wrapper
        auto callback = [loader, cbName]() {
            if (loader && loader->L) {
                lua_getglobal(loader->L, cbName.c_str());
                if (lua_isfunction(loader->L, -1)) {
                    if (lua_pcall(loader->L, 0, 0, 0) != LUA_OK) {
                        const char* error = lua_tostring(loader->L, -1);
                        LOG_ERROR("LevelLoader", "Button callback error: %s", error);
                        lua_pop(loader->L, 1);
                    }
                }
                else {
                    LOG_WARN("LevelLoader", "Button callback not found: %s", cbName.c_str());
                    lua_pop(loader->L, 1);
                }
            }
            };

        // Create button through UISystem, PASSING THE LAYER DIRECTLY
        UIButton* button = loader->uiSystem->CreateButton(
            texture,
            Vector2D(posX, posY),
            Vector2D(scaleX, scaleY),
            callback,
            layer // <--- PASSED AS ARGUMENT
        );

        if (button) {
            // Note: We don't need 'button->layer = layer' here anymore 
            // because the CreateButton function inside UISystem handles it.
            // Button created
            lua_pushinteger(L, static_cast<lua_Integer>(reinterpret_cast<intptr_t>(button)));
        }
        else {
            lua_pushinteger(L, 0);
        }

        return 1;
    }

    int LevelLoader::Lua_ClearAllButtons(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->uiSystem) return 0;

        loader->uiSystem->ClearAllButtons();
        return 0;
    }

    /**
     * @brief Renders text centered on a specific button
     * @params buttonID, font, text, offX, offY, scale, r, g, b
     *
     * Implementation details:
     * - Validates that the button entity still exists in the ECS to prevent crashes
     * - Determines render target size (Window size vs ImGui Viewport)
     * - Performs World-to-Screen coordinate conversion using the active camera's ViewProjection matrix
     * - Calculates responsive scaling logic (Reference resolution: 1920x1080)
     * - Centers the text on the projected screen coordinates
     * - Temporarily binds the correct Framebuffer Object (FBO) if rendering inside the Editor
     */
    int LevelLoader::Lua_DrawButtonText(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->graphicsSystem || !loader->coreEngine) return 0;

        // Parse parameters
        lua_Integer buttonID = luaL_checkinteger(L, 1);
        const char* font = luaL_checkstring(L, 2);
        const char* text = luaL_checkstring(L, 3);
        float offsetX = luaL_checknumber(L, 4);
        float offsetY = luaL_checknumber(L, 5);
        float scale = luaL_checknumber(L, 6);
        float colorR = luaL_checknumber(L, 7);
        float colorG = luaL_checknumber(L, 8);
        float colorB = luaL_checknumber(L, 9);

        // Debug tracking removed for performance

        UIButton* button = reinterpret_cast<UIButton*>(static_cast<intptr_t>(buttonID));
        if (!button) {
            LOG_WARN("LevelLoader", "Invalid button ID for DrawButtonText");
            return 0;
        }

        // CRITICAL: Validate button still exists in UISystem (prevents cross-level contamination)
        auto* uiSystem = loader->coreEngine->GetUISystem();
        if (!uiSystem) {
            return 0; // No UI system, can't render
        }

        // Check if button entity is still valid (not destroyed when level changed)
        auto* em = loader->coreEngine->GetEntityManager();
        if (!em || !button->entity.IsValid() || !em->HasComponent<Transform>(button->entity)) {
            // Button was destroyed (level changed), don't render
            static int warnThrottle = 0;
            if (warnThrottle++ % 60 == 0) {
                LOG_WARN("LevelLoader", "Skipping text for destroyed button (level changed?) - throttled");
            }
            return 0;
        }

        // Get correct render target size
        int fbWidth, fbHeight;

        auto* imgui = loader->coreEngine->GetImGuiSystem();
        if (imgui && imgui->IsEnabled() && imgui->IsRenderingToViewport() &&
            imgui->GetViewportFBO() != 0) {
            // Editor mode: Use viewport dimensions
            fbWidth = imgui->GetViewportWidth();
            fbHeight = imgui->GetViewportHeight();
        }
        else {
            // Non-editor mode: Use WINDOW size (not framebuffer - avoids DPI scaling issues)
            auto windowSystem = loader->coreEngine->GetWindowSystem();
            if (!windowSystem) return 0;
            GLFWwindow* window = windowSystem->GetWindow();
            if (!window) return 0;
            glfwGetWindowSize(window, &fbWidth, &fbHeight);  // Changed from glfwGetFramebufferSize
        }

        // ========================================
        // GET BUTTON'S ACTUAL SCREEN POSITION
        // ========================================
        // Use the button entity's Transform component for accurate positioning
        auto& transform = em->GetComponent<Transform>(button->entity);
        float worldX = transform.position.x;
        float worldY = transform.position.y;

        //  FIX: Use the active camera (editorCamera or mainCamera) based on play state
        // This fixes the issue where text moves with editorCamera but buttons don't
        Camera& activeCamera = loader->coreEngine->IsPlaying()
            ? loader->graphicsSystem->GetCamera()           // Playing: use mainCamera
            : loader->graphicsSystem->GetEditorCamera();    // Editor: use editorCamera

        // Convert button center from world space to screen space
        glm::mat4 viewProj = activeCamera.GetViewProjectionMatrix();
        glm::vec4 clipSpace = viewProj * glm::vec4(worldX, worldY, 0.0f, 1.0f);

        float ndcX = clipSpace.x;
        float ndcY = clipSpace.y;

        float screenX = (ndcX + 1.0f) * 0.5f * fbWidth;
        float screenY = (ndcY + 1.0f) * 0.5f * fbHeight;

        // ========================================
        // RESPONSIVE TEXT SCALING
        // ========================================
        // Scale text proportionally to viewport size
        // Reference resolution: 1920x1080 (common HD resolution)
        const float REFERENCE_WIDTH = 1920.0f;
        const float REFERENCE_HEIGHT = 1080.0f;

        // Calculate scale factors for width and height
        float scaleX = static_cast<float>(fbWidth) / REFERENCE_WIDTH;
        float scaleY = static_cast<float>(fbHeight) / REFERENCE_HEIGHT;

        // Use the smaller scale factor to maintain aspect ratio
        // This prevents text from stretching on non-16:9 aspect ratios
        float viewportScale = std::min(scaleX, scaleY);

        // Apply viewport scaling to the base text scale from Lua
        float finalScale = scale * viewportScale;

        // Clamp scale to prevent text from becoming too small or too large
        const float MIN_SCALE = 0.3f;
        const float MAX_SCALE = 3.0f;
        finalScale = std::max(MIN_SCALE, std::min(MAX_SCALE, finalScale));

        // ========================================
        // TEXT CENTERING - SNAP TO BUTTON CENTER
        // ========================================
        // Scale offsets proportionally to viewport size (like text scale)
        // This ensures text stays properly aligned when window is resized
        float scaledOffsetX = offsetX * viewportScale;
        float scaledOffsetY = offsetY * viewportScale;

        // Center text at button's screen position with scaled offsets
        float centeredX = screenX + scaledOffsetX;
        float centeredY = screenY + scaledOffsetY;

        // ========================================
        // DEBUG OUTPUT (Enhanced)
        // ========================================
        bool editorEnabled = imgui && imgui->IsEnabled() && imgui->IsRenderingToViewport();

        // Debug output removed for performance
        // ========================================

        glm::vec3 textColor(colorR, colorG, colorB);

        // CRITICAL FIX: Bind viewport FBO if editor is enabled
        // This ensures text renders to the game viewport, not the main window
        GLuint previousFBO = 0;
        bool needsRestore = false;

        if (imgui && imgui->IsEnabled() && imgui->IsRenderingToViewport()) {
            GLuint viewportFBO = imgui->GetViewportFBO();
            if (viewportFBO != 0) {
                // Save current FBO and bind viewport FBO
                glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&previousFBO));
                glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO);
                needsRestore = true;

                // Rendering text to viewport FBO
            }
        }

        // Draw text to the currently bound framebuffer (viewport or main window)
        // Use centered coordinates and finalScale for responsive, centered text
        loader->graphicsSystem->DrawText4(font, text, centeredX, centeredY, finalScale, textColor);

        // Restore previous framebuffer
        if (needsRestore) {
            glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
        }

        return 0;
    }

    int LevelLoader::Lua_DrawText(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->graphicsSystem) return 0;

        // Parse parameters: DrawText(font, text, x, y, scale, r, g, b)
        const char* font = luaL_checkstring(L, 1);
        const char* text = luaL_checkstring(L, 2);
        float x = luaL_checknumber(L, 3);
        float y = luaL_checknumber(L, 4);
        float scale = luaL_checknumber(L, 5);
        float r = luaL_checknumber(L, 6);
        float g = luaL_checknumber(L, 7);
        float b = luaL_checknumber(L, 8);

        glm::vec3 color(r, g, b);
        loader->graphicsSystem->DrawText4(font, text, x, y, scale, color);

        return 0;
    }

    // ========================================================================
    // INPUT API (FIXED for InputSystem)
    // ========================================================================

    /**
     * @brief Checks if a specific key is currently held down
     * @params keyName (string) - "W", "Space", "Enter", etc.
     * @return boolean
     *
     * Implementation details:
     * - Maps Lua string arguments to C++ KeyCode enums
     * - Queries the core InputSystem for key state
     * - Supports alphabets, arrows, numbers, function keys, and special keys
     */
    int LevelLoader::Lua_IsKeyDown(lua_State* L) {
        const char* keyName = luaL_checkstring(L, 1);

        // Only block gameplay hotkeys when an ImGui popup/modal is open OR when typing in a text input.
        // If ImGui is merely visible (editor overlay), allow gameplay hotkeys to work.
        if (ImGui::GetCurrentContext())
        {
            ImGuiIO& io = ImGui::GetIO();

            // AnyPopupLevel catches nested popups/modals (e.g., Save/Load modal + child popup)
            const bool anyPopupOpen =
                ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);

            const bool typingInTextBox = io.WantTextInput;

            if (anyPopupOpen || typingInTextBox)
            {
                lua_pushboolean(L, false);
                return 1;
            }
        }


        auto* input = CORE ? CORE->GetInputSystem() : nullptr;
        if (!input) {
            lua_pushboolean(L, false);
            return 1;
        }

        // Map to Framework::KeyCode (from Input.h)
        KeyCode keyCode = KEY_UNKNOWN;

        // Letters
        if (strcmp(keyName, "W") == 0) keyCode = KEY_W;
        else if (strcmp(keyName, "A") == 0) keyCode = KEY_A;
        else if (strcmp(keyName, "S") == 0) keyCode = KEY_S;
        else if (strcmp(keyName, "D") == 0) keyCode = KEY_D;
        else if (strcmp(keyName, "E") == 0) keyCode = KEY_E;
        else if (strcmp(keyName, "Q") == 0) keyCode = KEY_Q;
        else if (strcmp(keyName, "R") == 0) keyCode = KEY_R;
        else if (strcmp(keyName, "P") == 0) keyCode = KEY_P;

        // Special keys
        else if (strcmp(keyName, "Space") == 0) keyCode = KEY_SPACE;
        else if (strcmp(keyName, "Escape") == 0) keyCode = KEY_ESCAPE;
        else if (strcmp(keyName, "Enter") == 0) keyCode = KEY_ENTER;
        else if (strcmp(keyName, "Shift") == 0) keyCode = KEY_SHIFT;

        // Arrow keys
        else if (strcmp(keyName, "Up") == 0) keyCode = KEY_UP;
        else if (strcmp(keyName, "Down") == 0) keyCode = KEY_DOWN;
        else if (strcmp(keyName, "Left") == 0) keyCode = KEY_LEFT;
        else if (strcmp(keyName, "Right") == 0) keyCode = KEY_RIGHT;

        // Number keys
        else if (strcmp(keyName, "1") == 0) keyCode = KEY_1;
        else if (strcmp(keyName, "2") == 0) keyCode = KEY_2;
        else if (strcmp(keyName, "3") == 0) keyCode = KEY_3;
        else if (strcmp(keyName, "4") == 0) keyCode = KEY_4;
        else if (strcmp(keyName, "5") == 0) keyCode = KEY_5;
        else if (strcmp(keyName, "6") == 0) keyCode = KEY_6;
        else if (strcmp(keyName, "7") == 0) keyCode = KEY_7;
        else if (strcmp(keyName, "8") == 0) keyCode = KEY_8;
        else if (strcmp(keyName, "9") == 0) keyCode = KEY_9;
        else if (strcmp(keyName, "0") == 0) keyCode = KEY_0;
        else if (strcmp(keyName, "F1") == 0) keyCode = KEY_F1;
        else if (strcmp(keyName, "F2") == 0) keyCode = KEY_F2;
        else if (strcmp(keyName, "F3") == 0) keyCode = KEY_F3;
        // Check key state - use IsKeyDown for continuous input
        bool pressed = (keyCode != KEY_UNKNOWN) && input->IsKeyDown(keyCode);
        lua_pushboolean(L, pressed);
        return 1;
    }

    /**
     * @brief Checks if a mouse button is currently held down
     * @params button (string) - "Left" or "Right"
     * @return boolean
     */
    int LevelLoader::Lua_IsMouseButtonDown(lua_State* L) {
        const char* buttonName = luaL_checkstring(L, 1);
        auto* input = CORE ? CORE->GetInputSystem() : nullptr;
        if (!input) {
            lua_pushboolean(L, false);
            return 1;
        }

        KeyCode buttonCode = KEY_UNKNOWN;
        if (strcmp(buttonName, "Left") == 0) buttonCode = MOUSE_LEFT;
        else if (strcmp(buttonName, "Right") == 0) buttonCode = MOUSE_RIGHT;

        bool pressed = (buttonCode != KEY_UNKNOWN) && input->IsKeyDown(buttonCode);
        lua_pushboolean(L, pressed);
        return 1;
    }

    /**
     * @brief Checks if a mouse button was just pressed this frame (edge detection)
     * @params button (string) - "Left" or "Right"
     * @return boolean
     */
    int LevelLoader::Lua_IsMouseButtonPressed(lua_State* L) {
        const char* buttonName = luaL_checkstring(L, 1);
        auto* input = CORE ? CORE->GetInputSystem() : nullptr;
        if (!input) {
            lua_pushboolean(L, false);
            return 1;
        }

        KeyCode buttonCode = KEY_UNKNOWN;
        if (strcmp(buttonName, "Left") == 0) buttonCode = MOUSE_LEFT;
        else if (strcmp(buttonName, "Right") == 0) buttonCode = MOUSE_RIGHT;

        bool pressed = (buttonCode != KEY_UNKNOWN) && input->IsKeyPressed(buttonCode);
        lua_pushboolean(L, pressed);
        return 1;
    }

    /**
     * @brief Gets the current mouse position in screen coordinates
     * @return x, y (two numbers)
     */
    int LevelLoader::Lua_GetMousePosition(lua_State* L) {
        auto* input = CORE ? CORE->GetInputSystem() : nullptr;
        if (!input) {
            lua_pushnumber(L, 0);
            lua_pushnumber(L, 0);
            return 2;
        }

        float x = 0, y = 0;
        input->GetMousePosition(x, y);
        lua_pushnumber(L, x);
        lua_pushnumber(L, y);
        return 2;
    }

    int LevelLoader::Lua_LoadJSON(lua_State* L) {
        const char* filepath = luaL_checkstring(L, 1);

        // Loading JSON file

        // Load JSON and convert to Lua table
        bool success = LevelLoaderJSON::LoadJSONToLua(L, filepath);

        if (!success) {
            LOG_ERROR("LevelLoader", "Failed to load JSON: %s", filepath);
            lua_pushnil(L);
            return 1;
        }

        LOG_INFO("LevelLoader", "JSON successfully loaded as Lua table");
        return 1;
    }

    /**
     * @brief Toggles the ImGui editor overlay
     * @return boolean (Will the editor be enabled after this call?)
     *
     * Implementation details:
     * - Pauses the game automatically when editor is opened
     * - Calls ImguiSystem::RequestToggle()
     */
    int LevelLoader::Lua_ToggleEditor(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, false);
            return 1;
        }

        auto* imgui = loader->coreEngine->GetImGuiSystem();
        if (imgui) {
            bool willBeEnabled = !imgui->IsEnabled();
            imgui->RequestToggle();

            // ========================================
            // AUTO-PAUSE WHEN EDITOR OPENS
            // ========================================
            if (willBeEnabled) {
                loader->coreEngine->SetPlaying(false);
                // Game paused (editor mode)
            }
            // ========================================

            lua_pushboolean(L, willBeEnabled);
        }
        return 1;
    }

    int LevelLoader::Lua_IsEditorEnabled(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, false);
            return 1;
        }

        auto* imgui = loader->coreEngine->GetImGuiSystem();
        bool enabled = imgui && imgui->IsEnabled();
        lua_pushboolean(L, enabled);

        return 1;
    }

    int LevelLoader::Lua_SetEditorMode(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        bool enable = lua_toboolean(L, 1);

        auto* imgui = loader->coreEngine->GetImGuiSystem();
        if (imgui) {
            if (enable) {
                imgui->Enable();
                // Editor enabled
            }
            else {
                imgui->Disable();
                // Editor disabled
            }
        }

        return 0;
    }

    // ========================================================================
    // PAUSE CONTROL API
    // ========================================================================

    int LevelLoader::Lua_TogglePause(lua_State* L) {
        (void)L;
        GlobalPause::Toggle();
        // Pause toggled
        return 0;
    }

    int LevelLoader::Lua_IsPaused(lua_State* L) {
        bool paused = GlobalPause::IsPaused();
        lua_pushboolean(L, paused);
        return 1;
    }

    // ========================================================================
    // TILEMAP LOADING API
    // ========================================================================

    /**
     * @brief Loads a tilemap level from JSON data
     * @params jsonPath, startX, startY, spacingX, spacingY
     * @return boolean
     *
     * Implementation details:
     * - Uses TileMapLevelLoader::LoadLevel to instantiate entities
     * - Configures grid spacing and start position for the level
     */
    int LevelLoader::Lua_LoadTileMap(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, false);
            return 1;
        }

        // Parse parameters: LoadTileMap(jsonPath, startX, startY, spacingX, spacingY)
        const char* jsonPath = luaL_checkstring(L, 1);
        float startX = luaL_checknumber(L, 2);
        float startY = luaL_checknumber(L, 3);
        float spacingX = luaL_checknumber(L, 4);
        float spacingY = luaL_checknumber(L, 5);

        auto* spawner = loader->coreEngine->GetSpawner();
        auto* em = loader->coreEngine->GetEntityManager();

        if (!spawner || !em) {
            LOG_ERROR("LevelLoader", "LoadTileMap failed: spawner or entity manager not available");
            lua_pushboolean(L, false);
            return 1;
        }

        Vector2D startPos(startX, startY);
        Vector2D spacing(spacingX, spacingY);
        Vector2D tileSize(spacingX, spacingY); // Assuming tile size equals spacing

        bool success = TileMapLevelLoader::LoadLevel(jsonPath, spawner, em, startPos, spacing, tileSize);

        if (success) {
            if (loader->coreEngine) {
                auto* em = loader->coreEngine->GetEntityManager();
                auto* pc = loader->coreEngine->GetPlayerController();
                auto* spawner = loader->coreEngine->GetSpawner();
                auto* input = loader->coreEngine->GetInputSystem();
                auto* audio = loader->coreEngine->GetAudioSystem();

                if (em && pc && spawner && input) {
                    Framework::Entity player{ Framework::INVALID_ENTITY };
                    for (Framework::Entity e : em->GetAllEntities())
                    {
                        if (em->HasComponent<Framework::CircleCollider>(e) &&
                            !em->HasComponent<Framework::EnemyAI>(e))
                        {
                            player = e;
                            break;
                        }
                    }

                    if (player.GetID() != Framework::INVALID_ENTITY) {
                        pc->SetPlayerEntity(player);
                        pc->SetEntitySpawner(spawner);
                        pc->SetEntityManager(em);
                        pc->SetInputSystem(input);
                        // DISABLED: Don't force grid movement ON - let Lua scripts control it
                        // pc->SetGridMovementEnabled(true);

                        // Optional but recommended: enables walk SFX calls from PlayerManager
                        pc->SetAudioSystem(audio);

                        // PlayerController configured

                        // Optional: ensure turn starts in player phase when editor-loading
                        auto& turn = Framework::Turn();
                        turn.phase = Framework::TurnPhase::Player;
                        turn.busy = false;
                    }
                    else {
                        LOG_WARN("LevelLoader", "Lua_LoadTileMap: Could not find player entity to configure controller");
                    }

                }
            }

            // TileMap loaded successfully
        }
        else {
            LOG_ERROR("LevelLoader", "Failed to load TileMap from: %s", jsonPath);
        }

        lua_pushboolean(L, success);
        return 1;
    }

    // ========================================================================
    // AP INDICATOR / ENTITY MANAGEMENT API
    // ========================================================================

    /**
     * @brief Spawns a new sprite entity
     * @params texture, x, y, width, height, layer, rotation
     * @return integer (Entity ID)
     *
     * Implementation details:
     * - Calls Spawner::SpawnSprite
     * - Manually updates MeshRenderer layer and Transform rotation after spawn
     */
    int LevelLoader::Lua_SpawnSprite(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushinteger(L, 0);
            return 1;
        }

        // Parse parameters: SpawnSprite(texture, x, y, width, height, layer, rotation)
        const char* texture = luaL_checkstring(L, 1);
        float x = luaL_checknumber(L, 2);
        float y = luaL_checknumber(L, 3);
        float width = luaL_checknumber(L, 4);
        float height = luaL_checknumber(L, 5);
        int layer = luaL_optinteger(L, 6, 100);  // Default to UI layer
        float rotation = luaL_optnumber(L, 7, 0.0f);  // Optional rotation in degrees, default 0

        auto* spawner = loader->coreEngine->GetSpawner();
        auto* em = loader->coreEngine->GetEntityManager();

        if (!spawner || !em) {
            LOG_ERROR("LevelLoader", "SpawnSprite failed: spawner or entity manager not available");
            lua_pushinteger(L, 0);
            return 1;
        }

        // Spawn sprite entity
        Entity entity = spawner->SpawnSprite(texture, Vector2D(x, y), Vector2D(width, height));

        if (entity.GetID() == INVALID_ENTITY) {
            LOG_ERROR("LevelLoader", "Failed to spawn sprite: %s", texture);
            lua_pushinteger(L, 0);
            return 1;
        }

        // Set render layer
        if (em->HasComponent<MeshRenderer>(entity)) {
            auto& mr = em->GetComponent<MeshRenderer>(entity);
            mr.layer = layer;
        }

        // Set rotation (convert degrees to radians if needed, depends on your Transform component)
        if (em->HasComponent<Transform>(entity)) {
            auto& transform = em->GetComponent<Transform>(entity);
            transform.rotation = rotation;  // Assuming rotation is stored in degrees
        }

        // Sprite spawned

        // Return entity ID as integer
        lua_pushinteger(L, static_cast<lua_Integer>(entity.GetID()));
        return 1;
    }

    /**
     * @brief Spawns a new animated sprite entity with sprite sheet animation
     * @params texture, x, y, width, height, layer, rows, columns, frameCount, frameTime, loop
     * @return integer (Entity ID)
     */
    int LevelLoader::Lua_SpawnAnimatedSprite(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushinteger(L, 0);
            return 1;
        }

        // Parse parameters: SpawnAnimatedSprite(texture, x, y, width, height, layer, rows, columns, frameCount, frameTime, loop)
        const char* texture = luaL_checkstring(L, 1);
        float x = luaL_checknumber(L, 2);
        float y = luaL_checknumber(L, 3);
        float width = luaL_checknumber(L, 4);
        float height = luaL_checknumber(L, 5);
        int layer = luaL_optinteger(L, 6, 100);
        int rows = luaL_optinteger(L, 7, 1);
        int columns = luaL_optinteger(L, 8, 1);
        int frameCount = luaL_optinteger(L, 9, rows * columns);
        float frameTime = luaL_optnumber(L, 10, 0.1f);
        bool loop = lua_toboolean(L, 11);

        auto* spawner = loader->coreEngine->GetSpawner();
        auto* em = loader->coreEngine->GetEntityManager();
        auto* gfx = loader->coreEngine->GetGraphicsSystem();

        if (!spawner || !em || !gfx) {
            LOG_ERROR("LevelLoader", "SpawnAnimatedSprite failed: system not available");
            lua_pushinteger(L, 0);
            return 1;
        }

        // Spawn sprite entity
        Entity entity = spawner->SpawnSprite(texture, Vector2D(x, y), Vector2D(width, height));

        if (entity.GetID() == INVALID_ENTITY) {
            LOG_ERROR("LevelLoader", "Failed to spawn animated sprite: %s", texture);
            lua_pushinteger(L, 0);
            return 1;
        }

        // Set render layer
        if (em->HasComponent<MeshRenderer>(entity)) {
            auto& mr = em->GetComponent<MeshRenderer>(entity);
            mr.layer = layer;
        }

        // Add SpriteAnimation component
        em->AddComponent<SpriteAnimation>(entity);
        auto& anim = em->GetComponent<SpriteAnimation>(entity);
        anim.rows = rows;
        anim.columns = columns;
        anim.frameCount = frameCount;
        anim.frameTime = frameTime;
        anim.loop = loop;
        anim.playing = true;
        anim.currentFrame = 0;
        anim.startFrame = 0;  // IMPORTANT: Initialize startFrame to 0
        anim.elapsedTime = 0.0f;
        anim.useJsonConfig = false;  // Don't use JSON-based animation selection

        // Load sprite sheet texture
        anim.spriteSheet = gfx->GetResourceManager().LoadTexture(texture);
        
        // Calculate frame dimensions from texture size (CRITICAL for animation to work!)
        Texture* tex = gfx->GetResourceManager().GetTexture(anim.spriteSheet);
        if (tex) {
            int texW = tex->GetWidth();
            int texH = tex->GetHeight();
            anim.frameWidth = texW / columns;
            anim.frameHeight = texH / rows;
        } else {
            LOG_WARN("LevelLoader", "Failed to load texture for animated sprite: %s", texture);
        }

        LOG_INFO("LevelLoader", "Spawned animated sprite: %s (rows=%d, cols=%d, frames=%d)", 
                 texture, rows, columns, frameCount);

        lua_pushinteger(L, static_cast<lua_Integer>(entity.GetID()));
        return 1;
    }

    /**
     * @brief Adds or updates SpriteAnimation component on an existing sprite entity
     * @params entityID, texture, rows, columns, frameCount, frameTime, loop
     * @return boolean (success)
     */
    int LevelLoader::Lua_SetSpriteAnimationSheet(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, false);
            return 1;
        }

        // Parse parameters: SetSpriteAnimationSheet(entityID, texture, rows, columns, frameCount, frameTime, loop)
        lua_Integer entityID = luaL_checkinteger(L, 1);
        const char* texture = luaL_checkstring(L, 2);
        int rows = luaL_optinteger(L, 3, 1);
        int columns = luaL_optinteger(L, 4, 1);
        int frameCount = luaL_optinteger(L, 5, rows * columns);
        float frameTime = luaL_optnumber(L, 6, 0.1f);
        bool loop = lua_toboolean(L, 7);

        auto* em = loader->coreEngine->GetEntityManager();
        auto* gfx = loader->coreEngine->GetGraphicsSystem();

        if (!em || !gfx) {
            lua_pushboolean(L, false);
            return 1;
        }

        Entity entity(static_cast<uint32_t>(entityID));
        if (!entity.IsValid()) {
            LOG_WARN("LevelLoader", "SetSpriteAnimationSheet: Invalid entity ID=%lld", entityID);
            lua_pushboolean(L, false);
            return 1;
        }

        // Add SpriteAnimation component if not present
        if (!em->HasComponent<SpriteAnimation>(entity)) {
            em->AddComponent<SpriteAnimation>(entity);
        }

        auto& anim = em->GetComponent<SpriteAnimation>(entity);
        anim.rows = rows;
        anim.columns = columns;
        anim.frameCount = frameCount;
        anim.frameTime = frameTime;
        anim.loop = loop;
        anim.playing = true;
        anim.currentFrame = 0;
        anim.elapsedTime = 0.0f;
        anim.useJsonConfig = false;  // Don't use JSON-based animation selection

        // Load sprite sheet texture
        anim.spriteSheet = gfx->GetResourceManager().LoadTexture(texture);
        
        // Calculate frame dimensions from texture size (CRITICAL for animation to work!)
        Texture* tex = gfx->GetResourceManager().GetTexture(anim.spriteSheet);
        if (tex) {
            anim.frameWidth = tex->GetWidth() / columns;
            anim.frameHeight = tex->GetHeight() / rows;
            LOG_INFO("LevelLoader", "Animation texture: %dx%d, frame: %dx%d", 
                     tex->GetWidth(), tex->GetHeight(), anim.frameWidth, anim.frameHeight);
        } else {
            LOG_WARN("LevelLoader", "Failed to load texture for animation: %s", texture);
        }

        LOG_INFO("LevelLoader", "Set animation sheet on entity %lld: %s (rows=%d, cols=%d, frames=%d)", 
                 entityID, texture, rows, columns, frameCount);

        lua_pushboolean(L, true);
        return 1;
    }

    int LevelLoader::Lua_SetSpriteColor(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        // Parse parameters: SetSpriteColor(entityID, r, g, b, a)
        lua_Integer entityID = luaL_checkinteger(L, 1);
        float r = luaL_checknumber(L, 2);
        float g = luaL_checknumber(L, 3);
        float b = luaL_checknumber(L, 4);
        float a = luaL_optnumber(L, 5, 1.0f);  // Default alpha = 1.0

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) return 0;

        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<MeshRenderer>(entity)) {
            LOG_WARN("LevelLoader", "SetSpriteColor: Invalid entity or no MeshRenderer (ID=%lld)", entityID);
            return 0;
        }

        auto& mr = em->GetComponent<MeshRenderer>(entity);
        mr.tint = glm::vec4(r, g, b, a);

        // Debug: Log tint changes
        static int logThrottle = 0;
        // Sprite color set

        return 0;
    }

    /**
     * @brief Tint a tile at specific grid coordinates
     * Lua usage: TintTile(x, y, r, g, b, a)
     * @param x Grid x coordinate
     * @param y Grid y coordinate
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     * @param a Alpha component (0.0-1.0, optional, default 1.0)
     */
    int LevelLoader::Lua_TintTile(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("LevelLoader", "TintTile: No loader or core engine");
            return 0;
        }

        // Parse parameters: TintTile(x, y, r, g, b, a)
        int gridX = static_cast<int>(luaL_checkinteger(L, 1));
        int gridY = static_cast<int>(luaL_checkinteger(L, 2));
        float r = luaL_checknumber(L, 3);
        float g = luaL_checknumber(L, 4);
        float b = luaL_checknumber(L, 5);
        float a = luaL_optnumber(L, 6, 1.0f);  // Default alpha = 1.0

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("LevelLoader", "TintTile: No entity manager");
            return 0;
        }

        // Get the grid
        const Framework::Grid& grid = Framework::GetGrid();
        LOG_INFO("LevelLoader", "TintTile: Attempting to tint tile at (%d, %d) with color (%.2f, %.2f, %.2f, %.2f)",
            gridX, gridY, r, g, b, a);

        if (!grid.InBounds(gridX, gridY)) {
            LOG_WARN("LevelLoader", "TintTile: Grid position (%d, %d) out of bounds (grid size: %dx%d)",
                gridX, gridY, grid.cols, grid.rows);
            return 0;
        }

        // Get the tile entity at these coordinates
        Entity tileEntity = grid.TileAt(gridX, gridY);
        if (tileEntity.GetID() == Framework::INVALID_ENTITY) {
            LOG_WARN("LevelLoader", "TintTile: No tile entity at (%d, %d)", gridX, gridY);
            return 0;
        }

        LOG_INFO("LevelLoader", "TintTile: Found tile entity %u at (%d, %d)", tileEntity.GetID(), gridX, gridY);

        // Apply tint to the tile's sprite
        if (em->HasComponent<MeshRenderer>(tileEntity)) {
            auto& mr = em->GetComponent<MeshRenderer>(tileEntity);
            glm::vec4 oldTint = mr.tint;
            mr.tint = glm::vec4(r, g, b, a);
            LOG_INFO("LevelLoader", "TintTile: Applied tint to tile %u at (%d, %d) - Old: (%.2f,%.2f,%.2f,%.2f) New: (%.2f,%.2f,%.2f,%.2f)",
                tileEntity.GetID(), gridX, gridY,
                oldTint.r, oldTint.g, oldTint.b, oldTint.a,
                r, g, b, a);
        } else {
            LOG_WARN("LevelLoader", "TintTile: Tile entity %u at (%d, %d) has no MeshRenderer component",
                tileEntity.GetID(), gridX, gridY);
        }

        return 0;
    }

    int LevelLoader::Lua_SetSpriteGray(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        // Parse parameters: SetSpriteGray(entityID, grayAmount)
        lua_Integer entityID = luaL_checkinteger(L, 1);
        float grayAmount = luaL_checknumber(L, 2);
        if (grayAmount < 0.0f) grayAmount = 0.0f;
        if (grayAmount > 1.0f) grayAmount = 1.0f;

        auto* em = loader->coreEngine->GetEntityManager();
        auto* gfx = loader->coreEngine->GetGraphicsSystem();
        if (!em || !gfx) return 0;

        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<MeshRenderer>(entity)) {
            LOG_WARN("LevelLoader", "SetSpriteGray: Invalid entity or no MeshRenderer (ID=%lld)", entityID);
            return 0;
        }

        auto& mr = em->GetComponent<MeshRenderer>(entity);
        if (!mr.material.IsValid()) {
            LOG_WARN("LevelLoader", "SetSpriteGray: Entity %lld has no valid material", entityID);
            return 0;
        }

        auto* gs = static_cast<GraphicsSystemV2*>(gfx);
        Material* mat = gs->GetResourceManager().GetMaterial(mr.material);
        if (!mat) {
            LOG_WARN("LevelLoader", "SetSpriteGray: Failed to get material for entity %lld", entityID);
            return 0;
        }

        mat->parameters["grayAmount"] = grayAmount;
        return 0;
    }

    int LevelLoader::Lua_SetSpriteTexture(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        lua_Integer entityID = luaL_checkinteger(L, 1);
        const char* texturePath = luaL_checkstring(L, 2);

        auto* em = loader->coreEngine->GetEntityManager();
        auto* gfx = loader->coreEngine->GetGraphicsSystem();
        if (!em || !gfx) return 0;

        Entity entity(static_cast<uint32_t>(entityID));
        if (!entity.IsValid() || !em->HasComponent<MeshRenderer>(entity)) return 0;

        auto& mr = em->GetComponent<MeshRenderer>(entity);
        mr.spriteName = texturePath;

        // Load the new texture
        auto& resourceManager = gfx->GetResourceManager();
        TextureHandle newTexture = resourceManager.LoadTexture(texturePath);
        if (!newTexture.IsValid()) return 0;
        
        // CRITICAL: Update mr.texture - this is what the renderer actually uses!
        mr.texture = newTexture;
        
        // If entity has SpriteAnimation, update the spriteSheet too
        if (em->HasComponent<SpriteAnimation>(entity)) {
            auto& anim = em->GetComponent<SpriteAnimation>(entity);
            anim.spriteSheet = newTexture;
        }

        // Update the material's texture
        if (mr.material.IsValid()) {
            Material* mat = resourceManager.GetMaterial(mr.material);
            if (mat) {
                mat->albedoTexture = newTexture;
            }
        }

        return 0;
    }

    int LevelLoader::Lua_SetSpritePosition(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        // Parse parameters: SetSpritePosition(entityID, x, y)
        lua_Integer entityID = luaL_checkinteger(L, 1);
        float x = luaL_checknumber(L, 2);
        float y = luaL_checknumber(L, 3);

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) return 0;

        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<Transform>(entity)) {
            LOG_WARN("LevelLoader", "SetSpritePosition: Invalid entity or no Transform (ID=%lld)", entityID);
            return 0;
        }

        auto& transform = em->GetComponent<Transform>(entity);
        transform.position.x = x;
        transform.position.y = y;

        return 0;
    }

    int LevelLoader::Lua_DestroyEntity(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        // Parse parameters: DestroyEntity(entityID)
        lua_Integer entityID = luaL_checkinteger(L, 1);

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) return 0;

        Entity entity(static_cast<uint32_t>(entityID));

        if (entity.IsValid()) {
            em->DestroyEntity(entity);
            // Entity destroyed
        }
        else {
            LOG_WARN("LevelLoader", "DestroyEntity: Invalid entity (ID=%lld)", entityID);
        }

        return 0;
    }

    int LevelLoader::Lua_ClearAllEntities(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("LevelLoader", "ClearAllEntities failed - no engine instance");
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("LevelLoader", "ClearAllEntities failed - no EntityManager");
            return 0;
        }

        // Get count before clearing for logging
        size_t entityCount = em->GetAllEntities().size();

        // Clear all entities using existing ECS function
        em->ClearAllEntities();

        // All entities cleared
        return 0;
    }

    /**
     * @brief Gets current and max Action Points (AP) for the player
     * @return currentAP (int), maxAP (int)
     *
     * Implementation details:
     * - Scans for an entity with CircleCollider but NO EnemyAI component
     * - Returns 0,0 if player or AP component is missing
     */
    int LevelLoader::Lua_GetPlayerAP(lua_State* L) {
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_WARN("LevelLoader", "GetPlayerAP: No EntityManager");
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        // Find player entity (has CircleCollider but NOT EnemyAI)
        // Note: Movement component is removed in Level 3 for grid-based movement
        Entity player(INVALID_ENTITY);
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<CircleCollider>(e) &&
                !em->HasComponent<EnemyAI>(e)) {
                player = e;
                break;
            }
        }

        if (player.GetID() == INVALID_ENTITY) {
            LOG_WARN("LevelLoader", "GetPlayerAP: Player entity not found");
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        if (!em->HasComponent<AP>(player)) {
            LOG_WARN("LevelLoader", "GetPlayerAP: Player %u has no AP component", player.GetID());
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        auto& ap = em->GetComponent<AP>(player);

        // Player AP retrieved

        // Return currentAP, maxAP
        lua_pushinteger(L, ap.actionPoints);
        lua_pushinteger(L, ap.maxActionPoints);
        return 2;
    }

    // Get player's Attack AP (AttackAP component)
    int LevelLoader::Lua_GetPlayerAttackAP(lua_State* L) {
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        // Find player entity (CircleCollider but NOT EnemyAI)
        Entity player(INVALID_ENTITY);
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<CircleCollider>(e) &&
                !em->HasComponent<EnemyAI>(e)) {
                player = e;
                break;
            }
        }

        if (player.GetID() == INVALID_ENTITY ||
            !em->HasComponent<AttackAP>(player)) {
            LOG_WARN("LevelLoader", "GetPlayerAttackAP: Player not found or no AttackAP");
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        auto& aap = em->GetComponent<AttackAP>(player);

        // Return currentAttackAP, maxAttackAP
        lua_pushinteger(L, aap.points);
        lua_pushinteger(L, aap.maxPoints);
        return 2;
    }

    int LevelLoader::Lua_GetCameraPosition(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->graphicsSystem) {
            lua_pushnumber(L, 0.0);
            lua_pushnumber(L, 0.0);
            lua_pushnumber(L, 0.0);
            return 3;
        }

        glm::vec3 camPos = loader->graphicsSystem->GetCamera().GetPosition();

        // Return x, y, z
        lua_pushnumber(L, camPos.x);
        lua_pushnumber(L, camPos.y);
        lua_pushnumber(L, camPos.z);
        return 3;
    }

    /**
     * @brief Finds the player entity
     * Lua usage: local playerID = FindPlayer()
     * @return Player entity ID or 0 if not found
     */
    int LevelLoader::Lua_FindPlayer(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushinteger(L, 0);
            return 1;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_pushinteger(L, 0);
            return 1;
        }

        // Find player entity (has CircleCollider but NOT EnemyAI)
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<CircleCollider>(e) &&
                !em->HasComponent<EnemyAI>(e)) {
                // Player found
                lua_pushinteger(L, e.GetID());
                return 1;
            }
        }

        LOG_WARN("LevelLoader", "FindPlayer: No player found");
        lua_pushinteger(L, 0);
        return 1;
    }

    /**
     * @brief Gets all enemy entities
     * Lua usage: local enemies = GetAllEnemies() -- returns table of enemy IDs
     * @return Lua table of enemy entity IDs
     */
    int LevelLoader::Lua_GetAllEnemies(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_newtable(L);
            return 1;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_newtable(L);
            return 1;
        }

        lua_newtable(L);
        int index = 1;

        // Find all entities with EnemyAI component
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<EnemyAI>(e)) {
                lua_pushinteger(L, index++);
                lua_pushinteger(L, e.GetID());
                lua_settable(L, -3);
                // Enemy found
            }
        }

        // All enemies retrieved
        return 1;
    }

    /**
     * @brief Gets all player entities
     * Lua usage: players = GetAllPlayers() -- returns {playerID1, playerID2, playerID3}
     * @return Lua table of player entity IDs
     */
    int LevelLoader::Lua_GetAllPlayers(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_newtable(L);
            return 1;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_newtable(L);
            return 1;
        }

        lua_newtable(L);
        int index = 1;

        // Find all entities with AP component (indicates player in turn-based system)
        // Players have: AP, CircleCollider, Health components
        // Enemies have: EnemyAI component (we exclude those)
        // NOTE: Changed from Movement to AP because Movement was unreliable for entity 547
        for (Entity e : em->GetAllEntities()) {
            bool hasAP = em->HasComponent<AP>(e);
            bool hasCircleCollider = em->HasComponent<CircleCollider>(e);
            bool hasEnemyAI = em->HasComponent<EnemyAI>(e);

            if (hasAP && hasCircleCollider && !hasEnemyAI) {
                lua_pushinteger(L, index++);
                lua_pushinteger(L, e.GetID());
                lua_settable(L, -3);
            }
        }

        // All players retrieved
        return 1;
    }

    /**
     * @brief Sets an enemy's target entity (what it chases)
     * Lua usage: SetEnemyTarget(enemyID, playerID)
     * @param enemyID Enemy entity ID
     * @param targetID Target entity ID (usually player)
     * @return true if successful, false otherwise
     */
    int LevelLoader::Lua_SetEnemyTarget(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, 0);
            return 1;
        }

        uint32_t enemyID = (uint32_t)lua_tointeger(L, 1);
        uint32_t targetID = (uint32_t)lua_tointeger(L, 2);

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_pushboolean(L, 0);
            return 1;
        }

        Entity enemy{ enemyID };
        Entity target{ targetID };

        // Set the enemy's AI target
        if (em->HasComponent<EnemyAI>(enemy)) {
            auto& ai = em->GetComponent<EnemyAI>(enemy);
            ai.targetEntity = target;
            ai.moveDelay = 0.7f;

            LOG_INFO("LevelLoader", "SetEnemyTarget: Enemy %u now targeting %u", enemyID, targetID);
            lua_pushboolean(L, 1);
            return 1;
        }

        LOG_WARN("LevelLoader", "SetEnemyTarget: Enemy %u has no EnemyAI component", enemyID);
        lua_pushboolean(L, 0);
        return 1;
    }

    /**
     * @brief Gets the current turn phase
     * Lua usage: local turn = GetCurrentTurn() -- returns "Player" or "Enemy"
     * @return "Player" for Player turn, "Enemy" for Enemy turn
     */
    int LevelLoader::Lua_GetCurrentTurn(lua_State* L) {
        auto& turn = Turn();
        lua_pushstring(L, turn.phase == TurnPhase::Player ? "Player" : "Enemy");
        return 1;
    }

    /**
     * @brief Gets player's chest collection progress
     * Lua usage: local collected, required = GetChestProgress()
     * @return collected count, required count
     */
    int LevelLoader::Lua_GetChestProgress(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        // Find player entity
        Entity player(INVALID_ENTITY);
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<CircleCollider>(e) &&
                !em->HasComponent<EnemyAI>(e)) {
                player = e;
                break;
            }
        }

        if (player.GetID() == INVALID_ENTITY) {
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        // Get inventory
        int collected = 0;
        if (em->HasComponent<Inventory>(player)) {
            auto& inventory = em->GetComponent<Inventory>(player);
            collected = inventory.GetChestCount();
        }

        // Find goal to get required count
        int required = 0;
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<Goal>(e)) {
                auto& goal = em->GetComponent<Goal>(e);
                required = goal.chestsRequired;
                break;
            }
        }

        lua_pushinteger(L, collected);
        lua_pushinteger(L, required);
        return 2;
    }

    /**
     * @brief Get entity's current HP (Health Points)
     * @param entityID The entity ID
     * @return current HP, max HP (two numbers)
     *
     * Usage: local currentHP, maxHP = GetEntityHP(entityID)
     */
    int LevelLoader::Lua_GetEntityHP(lua_State* L)
    {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("LevelLoader", "GetEntityHP: No core engine");
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("LevelLoader", "GetEntityHP: No entity manager");
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<Health>(entity)) {
            LOG_WARN("LevelLoader", "GetEntityHP: Entity %d invalid or missing Health component", entityID);
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        auto& hp = em->GetComponent<Health>(entity);
        LOG_INFO("LevelLoader", "GetEntityHP: Entity %d has HP=%d/%d",
                 entityID, hp.currentHealth, hp.maxHealth);
        lua_pushinteger(L, hp.currentHealth);
        lua_pushinteger(L, hp.maxHealth);
        return 2;
    }

    /**
     * @brief Set the currently active character for party turn system
     * @param entityID The entity ID of the active character
     *
     * Usage: SetActiveCharacter(entityID)
     * Called by PartyTurnManager when switching characters
     */
    int LevelLoader::Lua_SetActiveCharacter(lua_State* L)
    {
        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        g_activeCharacterID = static_cast<uint32_t>(entityID);
        LOG_INFO("LevelLoader", "SetActiveCharacter: Active character set to entity %d", entityID);
        return 0;
    }

    /**
     * @brief Check if an entity is the currently active character
     * @param entityID The entity ID to check
     * @return boolean true if this entity is the active character
     *
     * Usage: local isActive = IsActiveCharacter(entityID)
     * Used by PlayerScript to determine if it should process input
     */
    int LevelLoader::Lua_IsActiveCharacter(lua_State* L)
    {
        int entityID = static_cast<int>(luaL_checknumber(L, 1));

        LOG_INFO("LevelLoader", "[Bridge] IsActiveCharacter called for entity %d", entityID);

        LevelLoader* loader = GetLevelLoader(L);
        if (!loader) {
            LOG_ERROR("LevelLoader", "[Bridge] IsActiveCharacter: No loader found!");
            lua_pushboolean(L, false);
            return 1;
        }

        // Get the LevelLoader's Lua state (where PartyTurnManager is running)
        lua_State* levelL = loader->L;
        if (!levelL) {
            LOG_ERROR("LevelLoader", "[Bridge] IsActiveCharacter: No level Lua state!");
            lua_pushboolean(L, false);
            return 1;
        }

        // Call IsActiveCharacter(entityID) in the LevelLoader's Lua state
        lua_getglobal(levelL, "IsActiveCharacter");
        if (!lua_isfunction(levelL, -1)) {
            LOG_ERROR("LevelLoader", "[Bridge] IsActiveCharacter: Function not found in level Lua state!");
            lua_pop(levelL, 1);
            lua_pushboolean(L, false);
            return 1;
        }

        // Push the entity ID argument
        lua_pushinteger(levelL, entityID);

        // Call the function (1 argument, 1 return value)
        int result = lua_pcall(levelL, 1, 1, 0);
        if (result != LUA_OK) {
            const char* error = lua_tostring(levelL, -1);
            LOG_ERROR("LevelLoader", "[Bridge] IsActiveCharacter: Error calling Lua function: %s", error);
            lua_pop(levelL, 1);  // Pop error
            lua_pushboolean(L, false);
            return 1;
        }

        // Get the return value
        bool isActive = lua_toboolean(levelL, -1);
        lua_pop(levelL, 1);  // Pop return value

        LOG_INFO("LevelLoader", "[Bridge] IsActiveCharacter: Entity %d is %s",
                 entityID, isActive ? "ACTIVE" : "INACTIVE");

        // Return the result in the entity's Lua state
        lua_pushboolean(L, isActive);
        return 1;
    }

    /**
     * @brief Set entity's HP (Health Points)
     * @param entityID The entity ID
     * @param currentHP New current HP value
     * @param maxHP New max HP value (optional, keeps existing if not provided)
     *
     * Usage: SetEntityHP(entityID, 10, 20) or SetEntityHP(entityID, 10)
     */
    int LevelLoader::Lua_SetEntityHP(lua_State* L)
    {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("LevelLoader", "SetEntityHP: No core engine");
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("LevelLoader", "SetEntityHP: No entity manager");
            return 0;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        int currentHP = static_cast<int>(luaL_checknumber(L, 2));
        int maxHP = -1;  // Optional parameter
        if (lua_gettop(L) >= 3) {
            maxHP = static_cast<int>(luaL_checknumber(L, 3));
        }

        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<Health>(entity)) {
            LOG_WARN("LevelLoader", "SetEntityHP: Entity %d invalid or missing Health component", entityID);
            return 0;
        }

        auto& hp = em->GetComponent<Health>(entity);
        hp.currentHealth = currentHP;
        if (maxHP >= 0) {
            hp.maxHealth = maxHP;
        }

        // Update dead flag
        hp.isDead = (hp.currentHealth <= 0);

        LOG_INFO("LevelLoader", "SetEntityHP: Entity %d HP set to %d/%d (dead=%d)",
                 entityID, hp.currentHealth, hp.maxHealth, hp.isDead);

        // If entity died, destroy it
        if (hp.isDead) {
            LOG_WARN("LevelLoader", "SetEntityHP: Entity %d died - beginning cleanup", entityID);

            // Clear tile occupancy
            if (em->HasComponent<Transform>(entity)) {
                auto& transform = em->GetComponent<Transform>(entity);
                auto tileOpt = Framework::WorldToTile(transform.position);
                if (tileOpt.has_value()) {
                    Framework::SetOccupant(tileOpt.value(), Entity{ INVALID_ENTITY });
                    LOG_INFO("LevelLoader", "  -> Cleared tile occupancy at (%d, %d)",
                        tileOpt.value().x, tileOpt.value().y);
                }
            }

            // Destroy entity
            LOG_WARN("LevelLoader", "  -> Destroying entity %d", entityID);
            em->DestroyEntity(entity);
        }

        return 0;
    }

    int LevelLoader::Lua_GetPlayerHP(lua_State* L)
    {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushinteger(L, 0); // current
            lua_pushinteger(L, 0); // max
            return 2;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        // Find the player: has Health, NOT EnemyAI
        Framework::Entity player(Framework::INVALID_ENTITY);
        for (Framework::Entity e : em->GetAllEntities()) {
            if (em->HasComponent<Framework::Health>(e) &&
                !em->HasComponent<Framework::EnemyAI>(e)) {
                player = e;
                break;
            }
        }

        if (!player.IsValid() || !em->HasComponent<Framework::Health>(player)) {
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        auto& hp = em->GetComponent<Framework::Health>(player);

        // IMPORTANT: use currentHealth for first return, maxHealth for second
        lua_pushinteger(L, hp.currentHealth);
        lua_pushinteger(L, hp.maxHealth);
        return 2;
    }

    int LevelLoader::Lua_SetSpriteVisibility(lua_State* L)
    {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        lua_Integer entityID = luaL_checkinteger(L, 1);
        int visibleInt = lua_toboolean(L, 2);  // 0/1  bool

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) return 0;

        Entity e(static_cast<uint32_t>(entityID));
        if (!e.IsValid() || !em->HasComponent<MeshRenderer>(e))
            return 0;

        auto& mr = em->GetComponent<MeshRenderer>(e);
        mr.visible = (visibleInt != 0);

        return 0;
    }

    // SetSpriteBlendMode(entityID, "Opaque" | "AlphaBlend" | "Additive" | "Multiply", forceOpaqueAlpha=false)
    // forceOpaqueAlpha: If true, ignores texture alpha and renders all pixels as opaque (fixes black pixels with alpha=0)
    int LevelLoader::Lua_SetSpriteBlendMode(lua_State* L)
    {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        lua_Integer entityID = luaL_checkinteger(L, 1);
        const char* blendModeStr = luaL_checkstring(L, 2);
        bool forceOpaqueAlpha = lua_toboolean(L, 3);  // Optional third parameter (default false)

        auto* em = loader->coreEngine->GetEntityManager();
        auto* gs = static_cast<GraphicsSystemV2*>(loader->coreEngine->GetGraphicsSystem());
        if (!em || !gs) return 0;

        Entity e(static_cast<uint32_t>(entityID));
        if (!e.IsValid() || !em->HasComponent<MeshRenderer>(e)) {
            LOG_WARN("LevelLoader", "SetSpriteBlendMode: Entity %d has no MeshRenderer", entityID);
            return 0;
        }

        auto& mr = em->GetComponent<MeshRenderer>(e);
        if (!mr.material.IsValid()) {
            LOG_WARN("LevelLoader", "SetSpriteBlendMode: Entity %d has no valid material", entityID);
            return 0;
        }

        // Get material and set blend mode
        auto* mat = gs->GetResourceManager().GetMaterial(mr.material);
        if (!mat) {
            LOG_WARN("LevelLoader", "SetSpriteBlendMode: Failed to get material for entity %d", entityID);
            return 0;
        }

        // Parse blend mode string
        std::string mode(blendModeStr);
        if (mode == "Opaque") {
            mat->blendMode = BlendMode::Opaque;
        } else if (mode == "AlphaBlend") {
            mat->blendMode = BlendMode::AlphaBlend;
        } else if (mode == "Additive") {
            mat->blendMode = BlendMode::Additive;
        } else if (mode == "Multiply") {
            mat->blendMode = BlendMode::Multiply;
        } else {
            LOG_WARN("LevelLoader", "SetSpriteBlendMode: Unknown blend mode '%s', use: Opaque, AlphaBlend, Additive, or Multiply", blendModeStr);
            return 0;
        }

        // Set force opaque alpha flag
        mat->forceOpaqueAlpha = forceOpaqueAlpha;

        LOG_INFO("LevelLoader", "SetSpriteBlendMode: Entity %d set to %s, forceOpaqueAlpha=%s",
                 entityID, blendModeStr, forceOpaqueAlpha ? "true" : "false");
        return 0;
    }

    // SetSpriteFilterMode(entityID, useNearest=true)
    // useNearest: true = pixel-perfect (GL_NEAREST), false = smooth (GL_LINEAR)
    // Use true for pixel art/UI to prevent black box artifacts from filtering
    int LevelLoader::Lua_SetSpriteFilterMode(lua_State* L)
    {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        lua_Integer entityID = luaL_checkinteger(L, 1);
        bool useNearest = lua_toboolean(L, 2);  // Default false if not provided

        auto* em = loader->coreEngine->GetEntityManager();
        auto* gs = static_cast<GraphicsSystemV2*>(loader->coreEngine->GetGraphicsSystem());
        if (!em || !gs) return 0;

        Entity e(static_cast<uint32_t>(entityID));
        if (!e.IsValid() || !em->HasComponent<MeshRenderer>(e)) {
            LOG_WARN("LevelLoader", "SetSpriteFilterMode: Entity %d has no MeshRenderer", entityID);
            return 0;
        }

        auto& mr = em->GetComponent<MeshRenderer>(e);
        if (!mr.material.IsValid()) {
            LOG_WARN("LevelLoader", "SetSpriteFilterMode: Entity %d has no valid material", entityID);
            return 0;
        }

        // Get material to access texture
        auto* mat = gs->GetResourceManager().GetMaterial(mr.material);
        if (!mat || !mat->albedoTexture.IsValid()) {
            LOG_WARN("LevelLoader", "SetSpriteFilterMode: Entity %d has no texture", entityID);
            return 0;
        }

        // Get texture and set filter mode
        auto* tex = gs->GetResourceManager().GetTexture(mat->albedoTexture);
        if (tex) {
            tex->SetFilterMode(useNearest);
            LOG_INFO("LevelLoader", "SetSpriteFilterMode: Entity %d set to %s filtering",
                     entityID, useNearest ? "NEAREST" : "LINEAR");
        } else {
            LOG_WARN("LevelLoader", "SetSpriteFilterMode: Failed to get texture for entity %d", entityID);
        }

        return 0;
    }

    // Toggle editor mode
    int LevelLoader::lua_ToggleEditorMode(lua_State* L) {
        (void)L;
        if (CORE) {
            CORE->ToggleEditorMode();
            if (CORE->GetImGuiSystem()) {
                if (CORE->IsEditorMode()) {
                    CORE->GetImGuiSystem()->Enable();
                }
                else {
                    CORE->GetImGuiSystem()->Disable();
                }
            }
        }
        return 0;
    }

    // Check if in editor mode
    int LevelLoader::lua_IsEditorMode(lua_State* L) {
        bool isEditor = CORE ? CORE->IsEditorMode() : false;
        lua_pushboolean(L, isEditor);
        return 1;
    }

    /**
     * @brief Check if gameplay should be disabled (buttons grayed out, etc.)
     * @return boolean - true if gameplay should be disabled
     * 
     * Returns true when:
     * - In editor mode AND not playing (STOP state)
     * 
     * Returns false when:
     * - Not in editor mode (normal gameplay)
     * - In editor mode but playing (PLAY button was clicked)
     * 
     * Usage in Lua:
     *   if ShouldDisableGameplay() then
     *       -- Gray out buttons, disable input
     *   end
     */
    int LevelLoader::Lua_ShouldDisableGameplay(lua_State* L) {
        if (!CORE) {
            lua_pushboolean(L, false);
            return 1;
        }
        
        bool isEditorMode = CORE->IsEditorMode();
        bool isPlaying = CORE->IsPlaying();
        
        // Disable gameplay only when in editor mode AND not playing
        bool shouldDisable = isEditorMode && !isPlaying;
        
        lua_pushboolean(L, shouldDisable);
        return 1;
    }

    // ========================================================================
    // SCRIPT COMPONENT MANAGEMENT
    // ========================================================================

    /**
     * @brief Add a ScriptComponent to an entity with specified script path
     * @param entityID (number) The entity ID to add component to
     * @param scriptPath (string) Path to the Lua script file
     * @return true if successful
     *
     * Usage: AddScriptComponentToEntity(entityID, "assets/scripts/PlayerScript.lua")
     */
    int LevelLoader::Lua_AddScriptComponentToEntity(lua_State* L) {
        // Get parameters
        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        const char* scriptPath = luaL_checkstring(L, 2);

        // Get entity manager
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_ERROR("LevelLoader", "AddScriptComponentToEntity: EntityManager not available");
            lua_pushboolean(L, false);
            return 1;
        }

        // Create entity and validate
        Entity entity(static_cast<uint32_t>(entityID));
        if (!entity.IsValid()) {
            LOG_ERROR("LevelLoader", "AddScriptComponentToEntity: Invalid entity ID %d", entityID);
            lua_pushboolean(L, false);
            return 1;
        }

        // Check if entity already has ScriptComponent
        if (em->HasComponent<ScriptComponent>(entity)) {
            LOG_WARN("LevelLoader", "Entity %d already has ScriptComponent, removing old one", entityID);
            em->RemoveComponent<ScriptComponent>(entity);
        }

        // Add ScriptComponent
        auto& script = em->AddComponent<ScriptComponent>(entity);
        script.scriptPath = scriptPath;

        LOG_INFO("LevelLoader", "Added ScriptComponent to entity %d: %s", entityID, scriptPath);

        lua_pushboolean(L, true);
        return 1;
    }

    /**
     * @brief Remove ScriptComponent from an entity
     * @param entityID (number) The entity ID to remove component from
     * @return true if successful
     *
     * Usage: RemoveScriptComponentFromEntity(entityID)
     */
    int LevelLoader::Lua_RemoveScriptComponentFromEntity(lua_State* L) {
        // Get parameter
        int entityID = static_cast<int>(luaL_checknumber(L, 1));

        // Get entity manager
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_ERROR("LevelLoader", "RemoveScriptComponentFromEntity: EntityManager not available");
            lua_pushboolean(L, false);
            return 1;
        }

        // Create entity and validate
        Entity entity(static_cast<uint32_t>(entityID));
        if (!entity.IsValid()) {
            LOG_ERROR("LevelLoader", "RemoveScriptComponentFromEntity: Invalid entity ID %d", entityID);
            lua_pushboolean(L, false);
            return 1;
        }

        // Check if entity has ScriptComponent
        if (!em->HasComponent<ScriptComponent>(entity)) {
            LOG_WARN("LevelLoader", "Entity %d does not have ScriptComponent", entityID);
            lua_pushboolean(L, false);
            return 1;
        }

        // Remove ScriptComponent
        em->RemoveComponent<ScriptComponent>(entity);

        LOG_INFO("LevelLoader", "Removed ScriptComponent from entity %d", entityID);

        lua_pushboolean(L, true);
        return 1;
    }

    // ========================================================================
    // PLAYER GRID MOVEMENT API IMPLEMENTATIONS
    // ========================================================================

    /**
     * @brief Get player's current grid position
     * @return x, y (two numbers) or nil if failed
     * Usage: local x, y = GetPlayerGridPosition()
     * * Implementation details:
     * - Uses Framework::WorldToTile to convert Transform position
     */
    int LevelLoader::Lua_GetPlayerGridPosition(lua_State* L) {
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        // Find player (has CircleCollider but NOT EnemyAI)
        Entity player = Framework::INVALID_ENTITY;
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<CircleCollider>(e) && !em->HasComponent<EnemyAI>(e)) {
                player = e;
                break;
            }
        }

        if (player.GetID() == Framework::INVALID_ENTITY || !em->HasComponent<Transform>(player)) {
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        auto& transform = em->GetComponent<Transform>(player);
        auto tileOpt = Framework::WorldToTile(transform.position);

        if (!tileOpt.has_value()) {
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        lua_pushnumber(L, tileOpt->x);
        lua_pushnumber(L, tileOpt->y);
        return 2;
    }

    /**
     * @brief Check if grid position is valid (in bounds)
     * @param x, y Grid coordinates
     * @return true if valid
     */
    int LevelLoader::Lua_IsValidGridPosition(lua_State* L) {
        int x = static_cast<int>(luaL_checknumber(L, 1));
        int y = static_cast<int>(luaL_checknumber(L, 2));

        Framework::GridCoord coord{ x, y };
        bool valid = Framework::InBounds(coord);

        lua_pushboolean(L, valid);
        return 1;
    }

    /**
     * @brief Check if tile is walkable
     * @param x, y Grid coordinates
     * @return true if walkable
     */
    int LevelLoader::Lua_IsWalkableTile(lua_State* L) {
        int x = static_cast<int>(luaL_checknumber(L, 1));
        int y = static_cast<int>(luaL_checknumber(L, 2));

        Framework::GridCoord coord{ x, y };
        bool walkable = Framework::IsWalkable(coord);

        lua_pushboolean(L, walkable);
        return 1;
    }

    /**
     * @brief Move player to grid tile
     * @param x, y Grid coordinates
     *
     * Implementation details:
     * - Clears occupancy at old tile
     * - Sets new transform position via Framework::TileToWorld
     * - Updates occupancy at new tile
     */
    int LevelLoader::Lua_MovePlayerToTile(lua_State* L) {
        int x = static_cast<int>(luaL_checknumber(L, 1));
        int y = static_cast<int>(luaL_checknumber(L, 2));

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        // Find player (has CircleCollider but NOT EnemyAI)
        Entity player = Framework::INVALID_ENTITY;
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<CircleCollider>(e) && !em->HasComponent<EnemyAI>(e)) {
                player = e;
                break;
            }
        }

        if (player.GetID() == Framework::INVALID_ENTITY || !em->HasComponent<Transform>(player)) {
            return 0;
        }

        auto& transform = em->GetComponent<Transform>(player);
        auto currentTileOpt = Framework::WorldToTile(transform.position);

        if (currentTileOpt.has_value()) {
            // Clear old occupancy
            Framework::SetOccupant(*currentTileOpt, Framework::Entity{ Framework::INVALID_ENTITY });
        }

        // Move to new position
        Framework::GridCoord newCoord{ x, y };
        transform.position = Framework::TileToWorld(newCoord);
        Framework::SetOccupant(newCoord, player);

        return 0;
    }

    /**
     * @brief Show border outline around tile
     * @param x, y Grid coordinates
     * @param thickness Border thickness
     * @param duration Duration in milliseconds
     */
    int LevelLoader::Lua_ShowTileBorder(lua_State* L) {
        (void)L;
        // int x = static_cast<int>(luaL_checknumber(L, 1));
        // int y = static_cast<int>(luaL_checknumber(L, 2));
        // float thickness = static_cast<float>(luaL_checknumber(L, 3));
        // int duration = static_cast<int>(luaL_checknumber(L, 4));

        // TODO: Implement visual border outline
        // For now, this is a placeholder
        return 0;
    }

    /**
     * @brief Pulse tile animation with color tinting
     * @param x, y Grid coordinates
     * @param duration Duration in seconds
     * @param r, g, b RGB color components (0.0-1.0)
     *
     * Lua usage: PulseTile(x, y, duration, r, g, b)
     * Example: PulseTile(5, 3, 0.3, 1.0, 0.0, 0.0) -- red pulse for 0.3 seconds
     */
    int LevelLoader::Lua_PulseTile(lua_State* L) {
        // Parse parameters
        int x = static_cast<int>(luaL_checknumber(L, 1));
        int y = static_cast<int>(luaL_checknumber(L, 2));
        float durationSeconds = static_cast<float>(luaL_checknumber(L, 3));
        float r = static_cast<float>(luaL_checknumber(L, 4));
        float g = static_cast<float>(luaL_checknumber(L, 5));
        float b = static_cast<float>(luaL_checknumber(L, 6));

        // Get entity manager
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_WARN("PulseTile", "No EntityManager available");
            return 0;
        }

        // Get tile entity at grid position
        const Grid& grid = GetGrid();
        if (!InBounds(GridCoord{x, y})) {
            LOG_WARN("PulseTile", "Grid position (%d, %d) out of bounds", x, y);
            return 0;
        }

        Entity tileEntity = grid.TileAt(x, y);
        if (tileEntity.GetID() == INVALID_ENTITY) {
            LOG_WARN("PulseTile", "No tile entity at (%d, %d)", x, y);
            return 0;
        }

        // Check if tile has Renderable component
        if (!em->HasComponent<Renderable>(tileEntity)) {
            LOG_WARN("PulseTile", "Tile at (%d, %d) has no Renderable component", x, y);
            return 0;
        }

        auto& renderable = em->GetComponent<Renderable>(tileEntity);

        // Check if this tile is already being tinted
        for (auto& state : s_activeTileTints) {
            if (state.tileEntity == tileEntity) {
                // Update expiry time and color
                state.expiryTimeMs = GetTickCount64() + static_cast<ULONGLONG>(durationSeconds * 1000.0f);
                renderable.tint = glm::vec4(r, g, b, 1.0f);
                return 0;
            }
        }

        // New tile tint - store original color
        TileTintState state;
        state.tileEntity = tileEntity;
        state.originalTint = renderable.tint;
        state.expiryTimeMs = GetTickCount64() + static_cast<ULONGLONG>(durationSeconds * 1000.0f);
        s_activeTileTints.push_back(state);

        // Apply new tint
        renderable.tint = glm::vec4(r, g, b, 1.0f);

        return 0;
    }

    /**
     * @brief Consume player AP
     * @param amount Amount of AP to consume
     */
    int LevelLoader::Lua_ConsumePlayerAP(lua_State* L) {
        int amount = static_cast<int>(luaL_checknumber(L, 1));

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        // Find player (has CircleCollider but NOT EnemyAI)
        Entity player = Framework::INVALID_ENTITY;
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<CircleCollider>(e) && !em->HasComponent<EnemyAI>(e)) {
                player = e;
                break;
            }
        }

        if (player.GetID() != Framework::INVALID_ENTITY && em->HasComponent<AP>(player)) {
            auto& ap = em->GetComponent<AP>(player);
            ap.actionPoints -= amount;
            if (ap.actionPoints < 0) ap.actionPoints = 0;
        }

        return 0;
    }

    /**
     * @brief Refill player AP to maximum
     * Lua usage: RefillPlayerAP()
     * Refills both movement AP and attack AP
     */
    int LevelLoader::Lua_RefillPlayerAP(lua_State* L) {
        (void)L;
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_WARN("LevelLoader", "RefillPlayerAP: No EntityManager");
            return 0;
        }

        // Find player (has CircleCollider but NOT EnemyAI)
        Entity player = Framework::INVALID_ENTITY;
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<CircleCollider>(e) && !em->HasComponent<EnemyAI>(e)) {
                player = e;
                break;
            }
        }

        if (player.GetID() == Framework::INVALID_ENTITY) {
            LOG_WARN("LevelLoader", "RefillPlayerAP: Player entity not found");
            return 0;
        }

        LOG_INFO("LevelLoader", "RefillPlayerAP: Found player entity %u", player.GetID());

        // Refill movement AP
        if (em->HasComponent<AP>(player)) {
            auto& ap = em->GetComponent<AP>(player);
            LOG_INFO("LevelLoader", "RefillPlayerAP: Before refill - AP=%d, MaxAP=%d",
                     ap.actionPoints, ap.maxActionPoints);
            ap.actionPoints = ap.maxActionPoints;
            LOG_INFO("LevelLoader", "RefillPlayerAP: After refill - AP=%d", ap.actionPoints);
        } else {
            LOG_WARN("LevelLoader", "RefillPlayerAP: Player has no AP component");
        }

        // Refill attack AP
        if (em->HasComponent<AttackAP>(player)) {
            auto& aap = em->GetComponent<AttackAP>(player);
            aap.points = aap.maxPoints;
            LOG_INFO("LevelLoader", "RefillPlayerAP: Attack AP refilled to %d", aap.points);
        }

        return 0;
    }

    /**
     * @brief Get current turn index
     * Lua usage: local turnIndex = GetTurnIndex()
     * @return Turn index (increments each time turn phase changes)
     */
    int LevelLoader::Lua_GetTurnIndex(lua_State* L) {
        auto& turn = Turn();
        lua_pushinteger(L, turn.turnIndex);
        return 1;
    }

    /**
     * @brief End the player turn and switch to enemy turn
     * Lua usage: EndPlayerTurn()
     */
    int LevelLoader::Lua_EndPlayerTurn(lua_State* L) {
        (void)L;  // Unused parameter
        Framework::EndPlayerTurn();
        LOG_INFO("LevelLoader", "Player turn ended via Lua");
        return 0;
    }

    /**
     * @brief End the enemy turn and switch to player turn
     * Lua usage: EndEnemyTurn()
     */
    int LevelLoader::Lua_EndEnemyTurn(lua_State* L) {
        (void)L;  // Unused parameter
        Framework::EndEnemyTurn();
        LOG_INFO("LevelLoader", "Enemy turn ended via Lua");
        return 0;
    }

    /**
     * @brief Set player sprite flip X
     * @param flip true to flip, false for normal
     */
    int LevelLoader::Lua_SetPlayerFlipX(lua_State* L) {
        bool flip = lua_toboolean(L, 1);

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        // Find player (has CircleCollider but NOT EnemyAI)
        Entity player = Framework::INVALID_ENTITY;
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<CircleCollider>(e) && !em->HasComponent<EnemyAI>(e)) {
                player = e;
                break;
            }
        }

        if (player.GetID() != Framework::INVALID_ENTITY && em->HasComponent<SpriteAnimation>(player)) {
            auto& anim = em->GetComponent<SpriteAnimation>(player);
            anim.flipX = flip;
        }

        return 0;
    }

    /**
     * @brief Enable or disable C++ grid movement (when using Lua PlayerScript)
     * @param enabled true to enable C++ grid movement, false to disable
     * Usage: SetGridMovementEnabled(false) -- Disable C++ movement, use Lua script instead
     */
    int LevelLoader::Lua_SetGridMovementEnabled(lua_State* L) {
        bool enabled = lua_toboolean(L, 1);

        auto* pc = CORE ? CORE->GetPlayerController() : nullptr;
        if (!pc) {
            LOG_WARN("LevelLoader", "SetGridMovementEnabled: No PlayerController");
            return 0;
        }

        pc->SetGridMovementEnabled(enabled);
        LOG_INFO("LevelLoader", "Grid movement %s via Lua", enabled ? "ENABLED" : "DISABLED");

        return 0;
    }

    /**
     * @brief Check if chest exists at tile
     * @param x, y Grid coordinates
     * @return true if chest exists
     */
    int LevelLoader::Lua_HasChestAtTile(lua_State* L) {
        int x = static_cast<int>(luaL_checknumber(L, 1));
        int y = static_cast<int>(luaL_checknumber(L, 2));

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_WARN("LevelLoader", "HasChestAtTile: No EntityManager");
            lua_pushboolean(L, false);
            return 1;
        }

        const Framework::Grid& grid = Framework::GetGrid();
        Entity tileEntity = grid.TileAt(x, y);

        bool hasChest = false;
        if (tileEntity.IsValid() && em->HasComponent<Chest>(tileEntity)) {
            hasChest = true;
            LOG_INFO("LevelLoader", "HasChestAtTile(%d, %d): Found chest at entity %u", x, y, tileEntity.GetID());
        } else {
            LOG_INFO("LevelLoader", "HasChestAtTile(%d, %d): No chest (entity valid=%d)",
                     x, y, tileEntity.IsValid());
        }

        lua_pushboolean(L, hasChest);
        return 1;
    }

    /**
     * @brief Collect chest at tile
     * @param x, y Grid coordinates
     */
    int LevelLoader::Lua_CollectChest(lua_State* L) {
        int x = static_cast<int>(luaL_checknumber(L, 1));
        int y = static_cast<int>(luaL_checknumber(L, 2));

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        const Framework::Grid& grid = Framework::GetGrid();
        Entity tileEntity = grid.TileAt(x, y);

        if (tileEntity.IsValid() && em->HasComponent<Chest>(tileEntity)) {
            auto& chest = em->GetComponent<Chest>(tileEntity);
            if (!chest.collected) {
                chest.collected = true;
                // TODO: Update visual representation
            }
        }

        return 0;
    }

    /**
     * @brief Check if goal exists at tile
     * @param x, y Grid coordinates
     * @return true if goal exists
     */
    int LevelLoader::Lua_HasGoalAtTile(lua_State* L) {
        int x = static_cast<int>(luaL_checknumber(L, 1));
        int y = static_cast<int>(luaL_checknumber(L, 2));

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            lua_pushboolean(L, false);
            return 1;
        }

        const Framework::Grid& grid = Framework::GetGrid();
        Entity tileEntity = grid.TileAt(x, y);

        bool hasGoal = false;
        if (tileEntity.IsValid() && em->HasComponent<Goal>(tileEntity)) {
            hasGoal = true;
        }

        lua_pushboolean(L, hasGoal);
        return 1;
    }

    // ========================================================================
    // ENEMY/ENTITY API IMPLEMENTATIONS
    // ========================================================================

    /**
     * @brief Get entity's current AP (Action Points)
     * @param entityID The entity ID
     * @return current AP, max AP (two numbers)
     *
     * Usage: local currentAP, maxAP = GetEntityAP(entityID)
     */
    int LevelLoader::Lua_GetEntityAP(lua_State* L) {
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_ERROR("LevelLoader", "GetEntityAP: No EntityManager");
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<AP>(entity)) {
            LOG_WARN("LevelLoader", "GetEntityAP: Entity %d invalid or missing AP component", entityID);
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        auto& ap = em->GetComponent<AP>(entity);
        LOG_INFO("LevelLoader", "GetEntityAP: Entity %d has AP=%d/%d",
                 entityID, ap.actionPoints, ap.maxActionPoints);
        lua_pushinteger(L, ap.actionPoints);
        lua_pushinteger(L, ap.maxActionPoints);
        return 2;
    }

    /**
     * @brief Get entity's current Attack AP
     * @param entityID The entity ID
     * @return current AttackAP, max AttackAP (two numbers)
     *
     * Usage: local currentAttackAP, maxAttackAP = GetEntityAttackAP(entityID)
     */
    int LevelLoader::Lua_GetEntityAttackAP(lua_State* L) {
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_ERROR("LevelLoader", "GetEntityAttackAP: No EntityManager");
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<AttackAP>(entity)) {
            LOG_WARN("LevelLoader", "GetEntityAttackAP: Entity %d invalid or missing AttackAP component", entityID);
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        auto& attackAP = em->GetComponent<AttackAP>(entity);
        LOG_INFO("LevelLoader", "GetEntityAttackAP: Entity %d has AttackAP=%d/%d",
                 entityID, attackAP.points, attackAP.maxPoints);
        lua_pushinteger(L, attackAP.points);
        lua_pushinteger(L, attackAP.maxPoints);
        return 2;
    }

    /**
     * @brief Get entity's current AP (Action Points) - legacy name
     * @param entityID The entity ID
     * @return AP value, or 0 if entity doesn't have AP component
     *
     * Usage: local ap = GetEnemyAP(enemyID)
     */
    int LevelLoader::Lua_GetEnemyAP(lua_State* L) {
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            lua_pushnumber(L, 0);
            return 1;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        Entity entity(static_cast<uint32_t>(entityID));

        if (!em->HasComponent<AP>(entity)) {
            lua_pushnumber(L, 0);
            return 1;
        }

        auto& ap = em->GetComponent<AP>(entity);
        lua_pushnumber(L, ap.actionPoints);
        return 1;
    }

    /**
     * @brief Refill entity AP to maximum
     * Lua usage: RefillEntityAP(entityID)
     * @param entityID The entity ID
     */
    int LevelLoader::Lua_RefillEntityAP(lua_State* L) {
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_ERROR("LevelLoader", "RefillEntityAP: No EntityManager");
            return 0;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<AP>(entity)) {
            LOG_WARN("LevelLoader", "RefillEntityAP: Entity %d invalid or missing AP component", entityID);
            return 0;
        }

        auto& ap = em->GetComponent<AP>(entity);
        int oldAP = ap.actionPoints;
        ap.actionPoints = ap.maxActionPoints;
        LOG_INFO("LevelLoader", "RefillEntityAP: Entity %d AP refilled %d -> %d",
                 entityID, oldAP, ap.actionPoints);

        return 0;
    }

    /**
     * @brief Refill enemy AP to maximum - legacy name
     * Lua usage: RefillEnemyAP(entityID)
     * @param entityID The enemy entity ID
     */
    int LevelLoader::Lua_RefillEnemyAP(lua_State* L) {
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        Entity entity(static_cast<uint32_t>(entityID));

        if (entity.IsValid() && em->HasComponent<AP>(entity)) {
            auto& ap = em->GetComponent<AP>(entity);
            ap.actionPoints = ap.maxActionPoints;
            LOG_INFO("LevelLoader", "RefillEnemyAP: Enemy %u AP refilled to %d", entity.GetID(), ap.actionPoints);
        }

        return 0;
    }

    /**
     * @brief Get entity's grid position
     * @param entityID The entity ID
     * @return x, y grid coordinates (or nil if invalid)
     *
     * Usage: local x, y = GetEntityGridPosition(entityID)
     */
    int LevelLoader::Lua_GetEntityGridPosition(lua_State* L) {
        int entityID = static_cast<int>(luaL_checknumber(L, 1));

        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        Entity entity(static_cast<uint32_t>(entityID));
        if (!entity.IsValid() || !em->HasComponent<Transform>(entity)) {
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        auto& transform = em->GetComponent<Transform>(entity);
        auto tileOpt = Framework::WorldToTile(transform.position);

        if (!tileOpt.has_value()) {
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        lua_pushnumber(L, tileOpt->x);
        lua_pushnumber(L, tileOpt->y);
        return 2;
    }

    /**
     * @brief Get entity's world position (from Transform component)
     * @param entityID The entity ID
     * @return x, y World coordinates, or nil, nil if entity invalid/no Transform
     *
     * Usage: local worldX, worldY = GetEntityWorldPosition(entityID)
     *
     * This is used by camera following to center on the active character.
     */
    int LevelLoader::Lua_GetEntityWorldPosition(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            std::cout << "[GetEntityWorldPosition] ERROR: No core engine!" << std::endl;
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            std::cout << "[GetEntityWorldPosition] ERROR: No entity manager!" << std::endl;
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<Transform>(entity)) {
            std::cout << "[GetEntityWorldPosition] ERROR: Entity " << entityID
                      << " invalid or missing Transform!" << std::endl;
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        auto& transform = em->GetComponent<Transform>(entity);
        std::cout << "[GetEntityWorldPosition] Entity " << entityID << " world position: ("
                  << transform.position.x << ", " << transform.position.y << ")" << std::endl;

        lua_pushnumber(L, transform.position.x);
        lua_pushnumber(L, transform.position.y);
        return 2;
    }

    /**
     * @brief Move entity to specified tile with full validation
     * @param entityID The entity ID
     * @param x Grid X coordinate
     * @param y Grid Y coordinate
     * @return boolean true if move succeeded, false if failed
     *
     * Usage: local success = MoveEntityToTile(entityID, x, y)
     */
    int LevelLoader::Lua_MoveEntityToTile(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("LevelLoader", "MoveEntityToTile: No core engine");
            lua_pushboolean(L, false);
            return 1;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("LevelLoader", "MoveEntityToTile: No entity manager");
            lua_pushboolean(L, false);
            return 1;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        int x = static_cast<int>(luaL_checknumber(L, 2));
        int y = static_cast<int>(luaL_checknumber(L, 3));

        Entity entity(static_cast<uint32_t>(entityID));

        // Validate entity has Transform
        if (!entity.IsValid() || !em->HasComponent<Transform>(entity)) {
            LOG_ERROR("LevelLoader", "MoveEntityToTile: Entity %d invalid or missing Transform", entityID);
            lua_pushboolean(L, false);
            return 1;
        }

        // Validate target coordinates
        Framework::GridCoord targetCoord{ x, y };

        if (!Framework::InBounds(targetCoord)) {
            LOG_WARN("LevelLoader", "MoveEntityToTile: Entity %d target (%d, %d) out of bounds",
                     entityID, x, y);
            lua_pushboolean(L, false);
            return 1;
        }

        if (!Framework::IsWalkable(targetCoord)) {
            LOG_WARN("LevelLoader", "MoveEntityToTile: Entity %d target (%d, %d) not walkable",
                     entityID, x, y);
            lua_pushboolean(L, false);
            return 1;
        }

        auto& transform = em->GetComponent<Transform>(entity);

        // Get current position to clear occupancy
        auto currentTileOpt = Framework::WorldToTile(transform.position);
        if (currentTileOpt.has_value()) {
            LOG_INFO("LevelLoader", "MoveEntityToTile: Entity %d clearing occupancy at (%d, %d)",
                     entityID, currentTileOpt->x, currentTileOpt->y);
            Framework::SetOccupant(*currentTileOpt, Framework::Entity{ Framework::INVALID_ENTITY });
        }

        // Move to new position
        transform.position = Framework::TileToWorld(targetCoord);
        Framework::SetOccupant(targetCoord, entity);

        LOG_INFO("LevelLoader", "MoveEntityToTile: Entity %d moved to (%d, %d) successfully",
                 entityID, x, y);
        lua_pushboolean(L, true);
        return 1;
    }

    /**
     * @brief Consume entity's AP
     * @param entityID The entity ID
     * @param amount Amount of AP to consume
     *
     * Usage: ConsumeEntityAP(entityID, 1)
     */
    int LevelLoader::Lua_ConsumeEntityAP(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("LevelLoader", "ConsumeEntityAP: No core engine");
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("LevelLoader", "ConsumeEntityAP: No entity manager");
            return 0;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        int amount = static_cast<int>(luaL_checknumber(L, 2));

        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<AP>(entity)) {
            LOG_WARN("LevelLoader", "ConsumeEntityAP: Entity %d invalid or missing AP component", entityID);
            return 0;
        }

        auto& ap = em->GetComponent<AP>(entity);
        int oldAP = ap.actionPoints;
        ap.actionPoints -= amount;
        if (ap.actionPoints < 0) {
            ap.actionPoints = 0;
        }

        LOG_INFO("LevelLoader", "ConsumeEntityAP: Entity %d AP consumed %d -> %d (-%d)",
                 entityID, oldAP, ap.actionPoints, amount);

        return 0;
    }

    /**
     * @brief Consume entity's Attack AP
     * @param entityID The entity ID
     * @param amount Amount of Attack AP to consume
     *
     * Usage: ConsumeEntityAttackAP(entityID, 1)
     */
    int LevelLoader::Lua_ConsumeEntityAttackAP(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            LOG_ERROR("LevelLoader", "ConsumeEntityAttackAP: No core engine");
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("LevelLoader", "ConsumeEntityAttackAP: No entity manager");
            return 0;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        int amount = static_cast<int>(luaL_checknumber(L, 2));

        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<AttackAP>(entity)) {
            LOG_WARN("LevelLoader", "ConsumeEntityAttackAP: Entity %d invalid or missing AttackAP component", entityID);
            return 0;
        }

        auto& attackAP = em->GetComponent<AttackAP>(entity);
        int oldAP = attackAP.points;
        attackAP.points -= amount;
        if (attackAP.points < 0) {
            attackAP.points = 0;
        }

        LOG_INFO("LevelLoader", "ConsumeEntityAttackAP: Entity %d AttackAP consumed %d -> %d (-%d)",
                 entityID, oldAP, attackAP.points, amount);

        return 0;
    }

    /**
     * @brief Refill entity's Attack AP to maximum
     * @param entityID The entity ID
     *
     * Usage: RefillEntityAttackAP(entityID)
     */
    int LevelLoader::Lua_RefillEntityAttackAP(lua_State* L) {
        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_ERROR("LevelLoader", "RefillEntityAttackAP: No EntityManager");
            return 0;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<AttackAP>(entity)) {
            LOG_WARN("LevelLoader", "RefillEntityAttackAP: Entity %d invalid or missing AttackAP component", entityID);
            return 0;
        }

        auto& attackAP = em->GetComponent<AttackAP>(entity);
        int oldAP = attackAP.points;
        attackAP.points = attackAP.maxPoints;
        LOG_INFO("LevelLoader", "RefillEntityAttackAP: Entity %d AttackAP refilled %d -> %d",
                 entityID, oldAP, attackAP.points);

        return 0;
    }

    /**
     * @brief Consume entity's AP - legacy name
     * @param entityID The entity ID
     * @param amount Amount of AP to consume
     *
     * Usage: ConsumeEnemyAP(enemyID, 1)
     */
    int LevelLoader::Lua_ConsumeEnemyAP(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            return 0;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        int amount = static_cast<int>(luaL_checknumber(L, 2));

        Entity entity(static_cast<uint32_t>(entityID));

        if (!em->HasComponent<AP>(entity)) {
            return 0;
        }

        auto& ap = em->GetComponent<AP>(entity);
        ap.actionPoints -= amount;
        if (ap.actionPoints < 0) {
            ap.actionPoints = 0;
        }

        return 0;
    }

    /**
     * @brief Deal damage to entity
     * @param entityID The entity ID
     * @param amount Damage amount
     *
     * Usage: DamageEntity(targetID, 1)
     */
    int LevelLoader::Lua_DamageEntity(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, 0);  // Return false
            return 1;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_pushboolean(L, 0);  // Return false
            return 1;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        int amount = static_cast<int>(luaL_checknumber(L, 2));

        Entity entity(static_cast<uint32_t>(entityID));

        if (!em->HasComponent<Health>(entity)) {
            lua_pushboolean(L, 0);  // Return false - entity has no health
            return 1;
        }

        auto& health = em->GetComponent<Health>(entity);
        health.currentHealth -= amount;

        LOG_INFO("LevelLoader", "DamageEntity: Entity %u took %d damage, HP: %d -> %d",
            entity.GetID(), amount, health.currentHealth + amount, health.currentHealth);

        if (health.currentHealth <= 0) {
            health.currentHealth = 0;
            health.isDead = true;

            LOG_WARN("LevelLoader", "!!! Entity %u DIED - Beginning cleanup !!!", entity.GetID());

            // Clear tile occupancy before destroying entity
            if (em->HasComponent<Transform>(entity)) {
                auto& transform = em->GetComponent<Transform>(entity);
                auto tileOpt = Framework::WorldToTile(transform.position);
                if (tileOpt.has_value()) {
                    Framework::SetOccupant(tileOpt.value(), Entity{ INVALID_ENTITY });
                    LOG_INFO("LevelLoader", "  -> Cleared tile occupancy at (%d, %d)",
                        tileOpt.value().x, tileOpt.value().y);
                }
            }

            // Log what components this entity has before destruction
            LOG_INFO("LevelLoader", "  -> Entity components before destruction:");
            if (em->HasComponent<Transform>(entity)) LOG_INFO("LevelLoader", "     - Transform");
            if (em->HasComponent<Renderable>(entity)) LOG_INFO("LevelLoader", "     - Renderable");
            if (em->HasComponent<AP>(entity)) LOG_INFO("LevelLoader", "     - AP");
            if (em->HasComponent<CircleCollider>(entity)) LOG_INFO("LevelLoader", "     - CircleCollider");

            // Delete the entity completely
            LOG_WARN("LevelLoader", "  -> CALLING DestroyEntity(%u)...", entity.GetID());
            em->DestroyEntity(entity);
            LOG_WARN("LevelLoader", "  -> DestroyEntity(%u) COMPLETE", entity.GetID());
            LOG_WARN("LevelLoader", "!!! Entity %u destruction finished !!!", entity.GetID());
        }

        lua_pushboolean(L, 1);  // Return true - damage successful
        return 1;
    }

    /**
     * @brief Find A* path from start to goal
     * @param startX Start grid X
     * @param startY Start grid Y
     * @param goalX Goal grid X
     * @param goalY Goal grid Y
     * @return Table of path coordinates (array of {x=, y=} tables)
     *
     * Usage: local path = FindPathToTarget(startX, startY, goalX, goalY)
     * for i, node in ipairs(path) do
     * print(node.x, node.y)
     * end
     * * Implementation details:
     * - Wrapper for Framework::PathfindingSystem::FindPath
     * - Converts std::vector of grid coords to a Lua table
     */
    int LevelLoader::Lua_FindPathToTarget(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_newtable(L);  // Return empty table
            return 1;
        }

        int startX = static_cast<int>(luaL_checknumber(L, 1));
        int startY = static_cast<int>(luaL_checknumber(L, 2));
        int goalX = static_cast<int>(luaL_checknumber(L, 3));
        int goalY = static_cast<int>(luaL_checknumber(L, 4));

        Framework::GridCoord start{ startX, startY };
        Framework::GridCoord goal{ goalX, goalY };

        // Get grid instance
        const auto& grid = Framework::GetGrid();

        // Call PathfindingSystem::FindPath
        // NOTE: This requires making FindPath public or accessible
        std::vector<Framework::GridCoord> path = Framework::PathfindingSystem::FindPath(start, goal, grid);

        // Convert path to Lua table
        lua_newtable(L);
        for (size_t i = 0; i < path.size(); ++i) {
            lua_pushnumber(L, i + 1);  // Lua arrays are 1-indexed
            lua_newtable(L);

            lua_pushstring(L, "x");
            lua_pushnumber(L, path[i].x);
            lua_settable(L, -3);

            lua_pushstring(L, "y");
            lua_pushnumber(L, path[i].y);
            lua_settable(L, -3);

            lua_settable(L, -3);
        }

        return 1;
    }

    // ========================================================================
    // GRID CONVERSION API
    // ========================================================================

    /**
     * @brief Convert grid tile coordinates to world position
     * @param tileX Grid X coordinate
     * @param tileY Grid Y coordinate
     * @return worldX, worldY (two return values, or nil if invalid)
     *
     * Usage: local worldX, worldY = TileToWorld(5, 10)
     */
    int LevelLoader::Lua_TileToWorld(lua_State* L) {
        int tileX = static_cast<int>(luaL_checknumber(L, 1));
        int tileY = static_cast<int>(luaL_checknumber(L, 2));

        Framework::GridCoord coord{ tileX, tileY };

        // Validate coordinates
        if (!Framework::InBounds(coord)) {
            LOG_WARN("LevelLoader", "TileToWorld: Coordinates (%d, %d) out of bounds", tileX, tileY);
            lua_pushnil(L);
            lua_pushnil(L);
            return 2;
        }

        // Convert to world position
        Framework::Vector2D worldPos = Framework::TileToWorld(coord);

        lua_pushnumber(L, worldPos.x);
        lua_pushnumber(L, worldPos.y);
        return 2;
    }

    // ========================================================================
    // SAVE/LOAD API - JSON Serialization for Lua
    // ========================================================================

    /**
     * @brief Save current scene to JSON file
     * @param filepath Path to save file
     * @param levelName Name of the level (for metadata)
     * @return boolean success
     *
     * Usage: local success = SaveSceneToJSON("assets/saves/level3.json", "Level3")
     */
    int LevelLoader::Lua_SaveSceneToJSON(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, false);
            return 1;
        }

        const char* filepath = luaL_checkstring(L, 1);
        const char* levelName = luaL_optstring(L, 2, "Unknown");

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("LevelLoader", "SaveSceneToJSON: EntityManager not available");
            lua_pushboolean(L, false);
            return 1;
        }

        // Set graphics system for texture loading during serialization
        SaveLoadSystem::SetGraphicsSystem(loader->graphicsSystem);

        bool success = SaveLoadSystem::SaveToJSON(filepath, em, levelName);
        
        if (success) {
            LOG_INFO("LevelLoader", "Scene saved to: %s", filepath);
        } else {
            LOG_ERROR("LevelLoader", "Failed to save scene to: %s", filepath);
        }

        lua_pushboolean(L, success);
        return 1;
    }

    /**
     * @brief Load scene from JSON file
     * @param filepath Path to load file
     * @param clearExisting Whether to clear existing entities (default: true)
     * @return boolean success
     *
     * Usage: local success = LoadSceneFromJSON("assets/saves/level3.json", true)
     */
    int LevelLoader::Lua_LoadSceneFromJSON(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, false);
            return 1;
        }

        const char* filepath = luaL_checkstring(L, 1);
        bool clearExisting = lua_isboolean(L, 2) ? lua_toboolean(L, 2) : true;

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            LOG_ERROR("LevelLoader", "LoadSceneFromJSON: EntityManager not available");
            lua_pushboolean(L, false);
            return 1;
        }

        // Set graphics system for texture loading during deserialization
        SaveLoadSystem::SetGraphicsSystem(loader->graphicsSystem);

        bool success = SaveLoadSystem::LoadFromJSON(filepath, em, clearExisting);
        
        if (success) {
            LOG_INFO("LevelLoader", "Scene loaded from: %s", filepath);
            // Rebuild spatial partition after loading
            RebuildSpatialPartition();
        } else {
            LOG_ERROR("LevelLoader", "Failed to load scene from: %s", filepath);
        }

        lua_pushboolean(L, success);
        return 1;
    }

    /**
     * @brief Auto-save current scene
     * @param levelName Name of the level
     * @return boolean success
     *
     * Usage: local success = AutoSaveScene("Level3")
     */
    int LevelLoader::Lua_AutoSaveScene(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, false);
            return 1;
        }

        const char* levelName = luaL_checkstring(L, 1);

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_pushboolean(L, false);
            return 1;
        }

        SaveLoadSystem::SetGraphicsSystem(loader->graphicsSystem);
        bool success = SaveLoadSystem::AutoSave(em, levelName);
        
        if (success) {
            LOG_INFO("LevelLoader", "Auto-saved: %s", levelName);
        }

        lua_pushboolean(L, success);
        return 1;
    }

    /**
     * @brief Load auto-save for a level
     * @param levelName Name of the level
     * @return boolean success
     *
     * Usage: local success = LoadAutoSave("Level3")
     */
    int LevelLoader::Lua_LoadAutoSave(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, false);
            return 1;
        }

        const char* levelName = luaL_checkstring(L, 1);

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            lua_pushboolean(L, false);
            return 1;
        }

        SaveLoadSystem::SetGraphicsSystem(loader->graphicsSystem);
        bool success = SaveLoadSystem::LoadAutoSave(em, levelName);
        
        if (success) {
            LOG_INFO("LevelLoader", "Loaded auto-save: %s", levelName);
            RebuildSpatialPartition();
        }

        lua_pushboolean(L, success);
        return 1;
    }

    /**
     * @brief Check if auto-save exists for a level
     * @param levelName Name of the level
     * @return boolean exists
     *
     * Usage: local exists = HasAutoSave("Level3")
     */
    int LevelLoader::Lua_HasAutoSave(lua_State* L) {
        const char* levelName = luaL_checkstring(L, 1);
        bool exists = SaveLoadSystem::HasAutoSave(levelName);
        lua_pushboolean(L, exists);
        return 1;
    }

    /**
     * @brief Clear auto-save for a level
     * @param levelName Name of the level
     * @return boolean success
     *
     * Usage: local success = ClearAutoSave("Level3")
     */
    int LevelLoader::Lua_ClearAutoSave(lua_State* L) {
        const char* levelName = luaL_checkstring(L, 1);
        bool success = SaveLoadSystem::ClearAutoSave(levelName);
        lua_pushboolean(L, success);
        return 1;
    }

// ============================================================================
// PROCEDURAL MAP API
// ============================================================================

    int LevelLoader::Lua_LoadProceduralMap(lua_State* L) {
        std::cout << "[Lua_LoadProceduralMap] Called!\n";

        int width = static_cast<int>(luaL_checknumber(L, 1));
        int height = static_cast<int>(luaL_checknumber(L, 2));
        const char* algorithm = luaL_checkstring(L, 3);

        LevelLoader* loader = GetLevelLoader(L);
        CoreEngine* core = loader->coreEngine;

        EntitySpawner* spawner = core->GetSpawner();
        EntityManager* em = core->GetEntityManager();

        if (!spawner || !em) {
            return luaL_error(L, "EntitySpawner or EntityManager not available");
        }

        // Configure generation
        MapGen::Config config;
        config.width = width;
        config.height = height;
        config.algorithm = algorithm;

        // Grid parameters - MATCH YOUR TileMap.json
        const float TILE_SIZE = 128.0f;
        Vector2D startPos(-0.6f, -0.4f);
        Vector2D spacing(0.1f, 0.1f);
        Vector2D tileSize(TILE_SIZE, TILE_SIZE);

        // Generate and load
        MapGen::GeneratedMap map = ProceduralMapLoader::LoadProceduralLevel(
            config, spawner, em, startPos, spacing, tileSize
        );

        // ========================================
        // NEW: RETURN SPAWN POSITIONS AS LUA TABLE
        // ========================================

        std::cout << "[Lua_LoadProceduralMap] Creating return table...\n";

        lua_newtable(L);  // Main table

        // Player spawn
        lua_pushstring(L, "playerX");
        lua_pushnumber(L, map.playerSpawn.x);
        lua_settable(L, -3);

        lua_pushstring(L, "playerY");
        lua_pushnumber(L, map.playerSpawn.y);
        lua_settable(L, -3);

        // Goal spawn
        lua_pushstring(L, "goalX");
        lua_pushnumber(L, map.goalSpawn.x);
        lua_settable(L, -3);

        lua_pushstring(L, "goalY");
        lua_pushnumber(L, map.goalSpawn.y);
        lua_settable(L, -3);

        // Calculate exact world position using same math as tile spawning
        float playerWorldX = startPos.x + (map.playerSpawn.x * spacing.x);
        float playerWorldY = startPos.y + (map.playerSpawn.y * spacing.y);

        lua_pushstring(L, "playerWorldX");
        lua_pushnumber(L, playerWorldX);
        lua_settable(L, -3);

        lua_pushstring(L, "playerWorldY");
        lua_pushnumber(L, playerWorldY);
        lua_settable(L, -3);

        // Goal world coords
        float goalWorldX = startPos.x + (map.goalSpawn.x * spacing.x);
        float goalWorldY = startPos.y + (map.goalSpawn.y * spacing.y);

        lua_pushstring(L, "goalWorldX");
        lua_pushnumber(L, goalWorldX);
        lua_settable(L, -3);

        lua_pushstring(L, "goalWorldY");
        lua_pushnumber(L, goalWorldY);
        lua_settable(L, -3);

        // Enemies with world coords
        lua_pushstring(L, "enemies");
        lua_newtable(L);
        for (size_t i = 0; i < map.enemySpawns.size(); i++) {
            lua_pushnumber(L, i + 1);
            lua_newtable(L);

            lua_pushstring(L, "x");
            lua_pushnumber(L, map.enemySpawns[i].x);
            lua_settable(L, -3);

            lua_pushstring(L, "y");
            lua_pushnumber(L, map.enemySpawns[i].y);
            lua_settable(L, -3);

            // World coords
            lua_pushstring(L, "worldX");
            lua_pushnumber(L, startPos.x + (map.enemySpawns[i].x * spacing.x));
            lua_settable(L, -3);

            lua_pushstring(L, "worldY");
            lua_pushnumber(L, startPos.y + (map.enemySpawns[i].y * spacing.y));
            lua_settable(L, -3);

            lua_settable(L, -3);
        }
        lua_settable(L, -3);

        // Chests with world coords
        lua_pushstring(L, "chests");
        lua_newtable(L);
        for (size_t i = 0; i < map.chestSpawns.size(); i++) {
            lua_pushnumber(L, i + 1);
            lua_newtable(L);

            lua_pushstring(L, "x");
            lua_pushnumber(L, map.chestSpawns[i].x);
            lua_settable(L, -3);

            lua_pushstring(L, "y");
            lua_pushnumber(L, map.chestSpawns[i].y);
            lua_settable(L, -3);

            // World coords
            lua_pushstring(L, "worldX");
            lua_pushnumber(L, startPos.x + (map.chestSpawns[i].x * spacing.x));
            lua_settable(L, -3);

            lua_pushstring(L, "worldY");
            lua_pushnumber(L, startPos.y + (map.chestSpawns[i].y * spacing.y));
            lua_settable(L, -3);

            lua_settable(L, -3);
        }
        lua_settable(L, -3);

        // ========================================
        // PARTY SPAWNS - 3 floor tiles with spacing
        // Ensures players don't spawn adjacent and block each other
        // ========================================
        std::cout << "[Lua_LoadProceduralMap] Finding spaced party spawn positions...\n";

        std::vector<MapGen::Position> partySpawns;
        partySpawns.push_back(map.playerSpawn);  // First is always valid

        // Search pattern - prioritize positions that are NOT directly adjacent
        // We want at least 2 tiles apart so players can move around each other
        const int searchOffsets[][2] = {
            // Distance 2 (preferred - gives room to move)
            {2, 0}, {-2, 0}, {0, 2}, {0, -2},
            {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
            {1, 2}, {-1, 2}, {1, -2}, {-1, -2},
            {2, 2}, {-2, 2}, {2, -2}, {-2, -2},
            // Distance 3 (also good)
            {3, 0}, {-3, 0}, {0, 3}, {0, -3},
            {3, 1}, {3, -1}, {-3, 1}, {-3, -1},
            {1, 3}, {-1, 3}, {1, -3}, {-1, -3},
            // Distance 1 (fallback only - adjacent)
            {1, 0}, {-1, 0}, {0, 1}, {0, -1},
            {1, 1}, {-1, 1}, {1, -1}, {-1, -1},
        };

        const int numOffsets = sizeof(searchOffsets) / sizeof(searchOffsets[0]);

        // Minimum distance between party members (Manhattan distance)
        const int MIN_PARTY_DISTANCE = 2;

        for (int i = 0; i < numOffsets && partySpawns.size() < 3; i++) {
            int testX = map.playerSpawn.x + searchOffsets[i][0];
            int testY = map.playerSpawn.y + searchOffsets[i][1];

            // Check bounds
            if (testX < 0 || testX >= map.width || testY < 0 || testY >= map.height) {
                continue;
            }

            // Check if it's a FLOOR tile (not a wall)
            if (map.getTile(testX, testY) != MapGen::TileType::FLOOR) {
                continue;
            }

            // Check distance from ALL existing party spawns
            bool tooClose = false;
            for (const auto& existingSpawn : partySpawns) {
                int manhattanDist = std::abs(testX - existingSpawn.x) + std::abs(testY - existingSpawn.y);
                if (manhattanDist < MIN_PARTY_DISTANCE) {
                    tooClose = true;
                    break;
                }
            }

            if (tooClose) {
                continue;  // Skip positions too close to existing party members
            }

            // Valid position found!
            partySpawns.push_back(MapGen::Position(testX, testY));
            std::cout << "[Lua_LoadProceduralMap] Party spawn " << partySpawns.size()
                << ": (" << testX << ", " << testY << ")\n";
        }

        // If we couldn't find 3 spaced positions, fall back to adjacent (with warning)
        if (partySpawns.size() < 3) {
            std::cout << "[Lua_LoadProceduralMap] WARNING: Couldn't find 3 spaced spawns, using fallback\n";

            // Try again without distance restriction
            for (int i = 0; i < numOffsets && partySpawns.size() < 3; i++) {
                int testX = map.playerSpawn.x + searchOffsets[i][0];
                int testY = map.playerSpawn.y + searchOffsets[i][1];

                if (testX < 0 || testX >= map.width || testY < 0 || testY >= map.height) {
                    continue;
                }

                if (map.getTile(testX, testY) != MapGen::TileType::FLOOR) {
                    continue;
                }

                // Check not already used (but ignore distance)
                bool alreadyUsed = false;
                for (const auto& pos : partySpawns) {
                    if (pos.x == testX && pos.y == testY) {
                        alreadyUsed = true;
                        break;
                    }
                }

                if (!alreadyUsed) {
                    partySpawns.push_back(MapGen::Position(testX, testY));
                    std::cout << "[Lua_LoadProceduralMap] Fallback spawn " << partySpawns.size()
                        << ": (" << testX << ", " << testY << ")\n";
                }
            }
        }

        // Last resort: duplicate player spawn
        while (partySpawns.size() < 3) {
            std::cout << "[Lua_LoadProceduralMap] WARNING: Duplicating player spawn\n";
            partySpawns.push_back(map.playerSpawn);
        }

        // Add partySpawns array to the Lua table
        lua_pushstring(L, "partySpawns");
        lua_newtable(L);
        for (size_t i = 0; i < partySpawns.size(); i++) {
            lua_pushnumber(L, static_cast<lua_Number>(i + 1));
            lua_newtable(L);

            lua_pushstring(L, "x");
            lua_pushnumber(L, partySpawns[i].x);
            lua_settable(L, -3);

            lua_pushstring(L, "y");
            lua_pushnumber(L, partySpawns[i].y);
            lua_settable(L, -3);

            lua_pushstring(L, "worldX");
            lua_pushnumber(L, startPos.x + (partySpawns[i].x * spacing.x));
            lua_settable(L, -3);

            lua_pushstring(L, "worldY");
            lua_pushnumber(L, startPos.y + (partySpawns[i].y * spacing.y));
            lua_settable(L, -3);

            lua_settable(L, -3);
        }
        lua_settable(L, -3);

        // Add map dimensions
        lua_pushstring(L, "width");
        lua_pushnumber(L, map.width);
        lua_settable(L, -3);

        lua_pushstring(L, "height");
        lua_pushnumber(L, map.height);
        lua_settable(L, -3);

        std::cout << "[Lua_LoadProceduralMap] Added " << partySpawns.size() << " spaced party spawns\n";

        return 1;
    }

    // ============================================================================
    // ENTITY SPAWNING API
    // ============================================================================

    int LevelLoader::Lua_SpawnPlayerAt(lua_State* L) {
        float worldX = static_cast<float>(luaL_checknumber(L, 1));
        float worldY = static_cast<float>(luaL_checknumber(L, 2));

        LevelLoader* loader = GetLevelLoader(L);
        EntitySpawner* spawner = loader->coreEngine->GetSpawner();
        EntityManager* em = loader->coreEngine->GetEntityManager();

        if (!spawner || !em) {
            return luaL_error(L, "Spawner or EM not available");
        }

        // Spawn player
        Entity player = spawner->SpawnPlayer(Vector2D(worldX, worldY));

        // CRITICAL: Ensure Movement component is present (required for GetAllPlayers())
        // Sometimes this component gets removed by camera/active character systems
        if (!em->HasComponent<Movement>(player)) {
            std::cout << "[SpawnPlayerAt] WARNING: Entity " << player.GetID() << " missing Movement component - adding it now" << std::endl;
            em->AddComponent<Movement>(player);
            auto& movement = em->GetComponent<Movement>(player);
            movement.moveSpeed = 0.2f;
        } else {
            std::cout << "[SpawnPlayerAt] Entity " << player.GetID() << " already has Movement component" << std::endl;
        }

        // ADD INVENTORY COMPONENT (like TileMapLoader does!)
        if (!em->HasComponent<Inventory>(player)) {
            em->AddComponent<Inventory>(player);
        }

        lua_pushnumber(L, player.GetID());
        return 1;
    }

    int LevelLoader::Lua_SpawnEnemyAt(lua_State* L) {
        float worldX = static_cast<float>(luaL_checknumber(L, 1));
        float worldY = static_cast<float>(luaL_checknumber(L, 2));

        LevelLoader* loader = GetLevelLoader(L);
        CoreEngine* core = loader->coreEngine;
        EntitySpawner* spawner = core->GetSpawner();

        if (!spawner) {
            return luaL_error(L, "EntitySpawner not available");
        }

        Entity enemy = spawner->SpawnEnemy(Vector2D(worldX, worldY));

        lua_pushnumber(L, enemy.GetID());
        return 1;
    }

    int LevelLoader::Lua_SpawnChestAt(lua_State* L) {
        float worldX = static_cast<float>(luaL_checknumber(L, 1));
        float worldY = static_cast<float>(luaL_checknumber(L, 2));

        LevelLoader* loader = GetLevelLoader(L);
        EntitySpawner* spawner = loader->coreEngine->GetSpawner();
        EntityManager* em = loader->coreEngine->GetEntityManager();
        Grid& grid = GetGrid();

        if (!spawner || !em) {
            return luaL_error(L, "Spawner or EM not available");
        }

        // Spawn chest sprite
        Entity chest = spawner->SpawnSprite(
            "assets/TileMap/Chest_1.png",
            Vector2D(worldX, worldY),
            Vector2D(grid.spacing.x * 0.8f, grid.spacing.y * 0.8f)  // 80% size like TileMapLoader
        );

        // ADD CHEST COMPONENT (like TileMapLoader does!)
        static int nextChestID = 0;
        em->AddComponent<Chest>(chest, nextChestID++);

        lua_pushnumber(L, chest.GetID());
        return 1;
    }

    int LevelLoader::Lua_SpawnGoalAt(lua_State* L) {
        float worldX = static_cast<float>(luaL_checknumber(L, 1));
        float worldY = static_cast<float>(luaL_checknumber(L, 2));

        LevelLoader* loader = GetLevelLoader(L);
        EntitySpawner* spawner = loader->coreEngine->GetSpawner();
        EntityManager* em = loader->coreEngine->GetEntityManager();
        Grid& grid = GetGrid();

        if (!spawner || !em) {
            return luaL_error(L, "Spawner or EM not available");
        }

        // Spawn goal sprite
        Entity goal = spawner->SpawnSprite(
            "assets/TileMap/Portal.png",
            Vector2D(worldX, worldY),
            Vector2D(grid.spacing.x * 0.9f, grid.spacing.y * 0.9f)  // 90% size like TileMapLoader
        );

        // ADD GOAL COMPONENT (like TileMapLoader does!)
        // Need to know total chests - pass as parameter or get from mapData
        int totalChests = 0;  // TODO: Pass this from Lua or calculate
        em->AddComponent<Goal>(goal, totalChests);

        lua_pushnumber(L, goal.GetID());
        return 1;
    }


    //int LevelLoader::Lua_TileToWorld(lua_State* L) {
    //    int gridX = static_cast<int>(luaL_checknumber(L, 1));
    //    int gridY = static_cast<int>(luaL_checknumber(L, 2));

    //    Grid& grid = GetGrid();

    //    if (!grid.InBounds(gridX, gridY)) {
    //        lua_pushnumber(L, 0.0);
    //        lua_pushnumber(L, 0.0);
    //        return 2;
    //    }

    //    // Use EXACT Grid math
    //    float worldX = grid.startPos.x + (static_cast<float>(gridX) * grid.spacing.x);
    //    float worldY = grid.startPos.y + (static_cast<float>(gridY) * grid.spacing.y);

    //    lua_pushnumber(L, worldX);
    //    lua_pushnumber(L, worldY);
    //    return 2;
    //}

    /**
     * @brief End the current character's turn and advance to next party member
     * @return none
     *
     * Usage: EndCharacterTurn()
     *
     * This is a bridge function that allows entity scripts (running in per-entity
     * Lua states) to call the EndCharacterTurn() function in the LevelLoader's
     * Lua state where PartyTurnManager is running.
     */
    int LevelLoader::Lua_EndCharacterTurn(lua_State* L) {
        std::cout << "[LevelLoader API] EndCharacterTurn() called from entity script" << std::endl;

        LevelLoader* loader = GetLevelLoader(L);
        if (!loader) {
            std::cout << "[LevelLoader API] ERROR: GetLevelLoader returned NULL!" << std::endl;
            return 0;
        }

        // Get the LevelLoader's Lua state (where PartyTurnManager is running)
        lua_State* levelL = loader->L;
        if (!levelL) {
            std::cout << "[LevelLoader API] ERROR: LevelLoader's Lua state is NULL!" << std::endl;
            return 0;
        }

        std::cout << "[LevelLoader API] Calling EndCharacterTurn() in LevelLoader's Lua state..." << std::endl;

        // Call the EndCharacterTurn function in the LevelLoader's Lua state
        lua_getglobal(levelL, "EndCharacterTurn");
        if (!lua_isfunction(levelL, -1)) {
            std::cout << "[LevelLoader API] ERROR: EndCharacterTurn is not a function!" << std::endl;
            lua_pop(levelL, 1);
            return 0;
        }

        // Call the function (0 arguments, 0 return values)
        int result = lua_pcall(levelL, 0, 0, 0);
        if (result != LUA_OK) {
            const char* error = lua_tostring(levelL, -1);
            std::cout << "[LevelLoader API] ERROR calling EndCharacterTurn: " << error << std::endl;
            lua_pop(levelL, 1);
            return 0;
        }

        std::cout << "[LevelLoader API] EndCharacterTurn() executed successfully!" << std::endl;
        return 0;
    }

    /**
     * @brief Check if UI is currently animating (AP refill animation)
     * @return boolean - true if animating, false otherwise
     *
     * Usage: local isAnimating = IsUIAnimating()
     *
     * This is a bridge function that allows entity scripts (running in per-entity
     * Lua states) to check if the UIManager in the LevelLoader's Lua state is
     * currently animating. Used to block player input during AP refill animations.
     */
    int LevelLoader::Lua_IsUIAnimating(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader) {
            lua_pushboolean(L, false);
            return 1;
        }

        // Get the LevelLoader's Lua state (where UIManager is running)
        lua_State* levelL = loader->L;
        if (!levelL) {
            lua_pushboolean(L, false);
            return 1;
        }

        // Call UIManager.IsAPAnimating() in the LevelLoader's Lua state
        lua_getglobal(levelL, "UIManager");
        if (!lua_istable(levelL, -1)) {
            lua_pop(levelL, 1);
            lua_pushboolean(L, false);
            return 1;
        }

        lua_getfield(levelL, -1, "IsAPAnimating");
        if (!lua_isfunction(levelL, -1)) {
            lua_pop(levelL, 2);  // Pop function and UIManager table
            lua_pushboolean(L, false);
            return 1;
        }

        // Call the function (0 arguments, 1 return value)
        int result = lua_pcall(levelL, 0, 1, 0);
        if (result != LUA_OK) {
            lua_pop(levelL, 2);  // Pop error and UIManager table
            lua_pushboolean(L, false);
            return 1;
        }

        // Get the return value
        bool isAnimating = lua_toboolean(levelL, -1);
        lua_pop(levelL, 2);  // Pop return value and UIManager table

        // Return the result in the entity's Lua state
        lua_pushboolean(L, isAnimating);
        return 1;
    }

    /**
     * @brief Check if the turn scroll animation is currently playing
     * @return boolean - true if playing, false otherwise
     *
     * Usage: local isPlaying = IsTurnScrollPlaying()
     *
     * This is a bridge function that allows entity scripts (running in per-entity
     * Lua states) to check if the "Your Turn" scroll animation is playing.
     * Used to block player input during the animation.
     */
    int LevelLoader::Lua_IsTurnScrollPlaying(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader) {
            lua_pushboolean(L, false);
            return 1;
        }

        // Get the LevelLoader's Lua state (where UIManager is running)
        lua_State* levelL = loader->L;
        if (!levelL) {
            lua_pushboolean(L, false);
            return 1;
        }

        // Get UIManager.IsTurnScrollPlaying() in the LevelLoader's Lua state
        lua_getglobal(levelL, "UIManager");
        if (!lua_istable(levelL, -1)) {
            lua_pop(levelL, 1);
            lua_pushboolean(L, false);
            return 1;
        }

        lua_getfield(levelL, -1, "IsTurnScrollPlaying");
        if (!lua_isfunction(levelL, -1)) {
            lua_pop(levelL, 2);  // Pop function and UIManager table
            lua_pushboolean(L, false);
            return 1;
        }

        // Call the function (0 arguments, 1 return value)
        int result = lua_pcall(levelL, 0, 1, 0);
        if (result != LUA_OK) {
            lua_pop(levelL, 2);  // Pop error and UIManager table
            lua_pushboolean(L, false);
            return 1;
        }

        // Get the return value
        bool isPlaying = lua_toboolean(levelL, -1);
        lua_pop(levelL, 2);  // Pop return value and UIManager table

        // Return the result in the entity's Lua state
        lua_pushboolean(L, isPlaying);
        return 1;
    }

    /**
     * @brief Check if we're in turn transition cooldown
     * @return boolean - true if in cooldown, false otherwise
     *
     * Usage: local inTransition = IsInTurnTransition()
     *
     * This is a bridge function that allows entity scripts (running in per-entity
     * Lua states) to check if PartyTurnManager in the LevelLoader's Lua state is
     * currently in turn transition cooldown. Used to block player input after turn switches.
     */
    int LevelLoader::Lua_IsInTurnTransition(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader) {
            lua_pushboolean(L, false);
            return 1;
        }

        // Get the LevelLoader's Lua state (where PartyTurnManager is running)
        lua_State* levelL = loader->L;
        if (!levelL) {
            lua_pushboolean(L, false);
            return 1;
        }

        // Call IsInTurnTransition() in the LevelLoader's Lua state
        lua_getglobal(levelL, "IsInTurnTransition");
        if (!lua_isfunction(levelL, -1)) {
            lua_pop(levelL, 1);
            lua_pushboolean(L, false);
            return 1;
        }

        // Call the function (0 arguments, 1 return value)
        int result = lua_pcall(levelL, 0, 1, 0);
        if (result != LUA_OK) {
            lua_pop(levelL, 1);  // Pop error
            lua_pushboolean(L, false);
            return 1;
        }

        // Get the return value
        bool inTransition = lua_toboolean(levelL, -1);
        lua_pop(levelL, 1);  // Pop return value

        // Return the result in the entity's Lua state
        lua_pushboolean(L, inTransition);
        return 1;
    }

    /**
     * @brief Trigger attack AP crystal consume animation
     *
     * Usage: TriggerAttackAPAnimation()
     *
     * This is a bridge function that allows entity scripts (running in per-entity
     * Lua states) to trigger the attack AP crystal shatter animation in UIManager
     * (which runs in the LevelLoader's Lua state).
     */
    int LevelLoader::Lua_TriggerAttackAPAnimation(lua_State* L) {
        LOG_INFO("LevelLoader", "[TriggerAttackAPAnimation] Called from entity script");

        LevelLoader* loader = GetLevelLoader(L);
        if (!loader) {
            LOG_ERROR("LevelLoader", "[TriggerAttackAPAnimation] Failed: No loader");
            return 0;
        }

        // Get the LevelLoader's Lua state (where UIManager is running)
        lua_State* levelL = loader->L;
        if (!levelL) {
            LOG_ERROR("LevelLoader", "[TriggerAttackAPAnimation] Failed: No LevelLoader Lua state");
            return 0;
        }

        LOG_INFO("LevelLoader", "[TriggerAttackAPAnimation] Getting UIManager...");

        // Get UIManager table
        lua_getglobal(levelL, "UIManager");
        if (!lua_istable(levelL, -1)) {
            LOG_ERROR("LevelLoader", "[TriggerAttackAPAnimation] Failed: UIManager is not a table");
            lua_pop(levelL, 1);
            return 0;
        }

        LOG_INFO("LevelLoader", "[TriggerAttackAPAnimation] Getting UIManager.GetComponent...");

        // Get UIManager.GetComponent function
        lua_getfield(levelL, -1, "GetComponent");
        if (!lua_isfunction(levelL, -1)) {
            LOG_ERROR("LevelLoader", "[TriggerAttackAPAnimation] Failed: GetComponent is not a function");
            lua_pop(levelL, 2);  // Pop function and UIManager table
            return 0;
        }

        LOG_INFO("LevelLoader", "[TriggerAttackAPAnimation] Calling GetComponent('attackAP')...");

        // Push "attackAP" as the argument
        lua_pushstring(levelL, "attackAP");

        // Call UIManager.GetComponent("attackAP") -> 1 argument, 1 return value
        int result = lua_pcall(levelL, 1, 1, 0);
        if (result != LUA_OK) {
            const char* error = lua_tostring(levelL, -1);
            LOG_ERROR("LevelLoader", "[TriggerAttackAPAnimation] Failed to call GetComponent: %s", error);
            lua_pop(levelL, 2);  // Pop error and UIManager table
            return 0;
        }

        // Now we have the attackAP component on the stack
        if (!lua_istable(levelL, -1)) {
            LOG_ERROR("LevelLoader", "[TriggerAttackAPAnimation] Failed: attackAP component is not a table");
            lua_pop(levelL, 2);  // Pop component and UIManager table
            return 0;
        }

        LOG_INFO("LevelLoader", "[TriggerAttackAPAnimation] Getting ConsumeOneAP method...");

        // Get the ConsumeOneAP method from the component
        lua_getfield(levelL, -1, "ConsumeOneAP");
        if (!lua_isfunction(levelL, -1)) {
            LOG_ERROR("LevelLoader", "[TriggerAttackAPAnimation] Failed: ConsumeOneAP is not a function");
            lua_pop(levelL, 3);  // Pop function, component, and UIManager table
            return 0;
        }

        LOG_INFO("LevelLoader", "[TriggerAttackAPAnimation] Calling ConsumeOneAP()...");

        // Push the component table as 'self' for the method call
        lua_pushvalue(levelL, -2);  // Duplicate the component table

        // Call attackAPComponent:ConsumeOneAP() -> 1 argument (self), 0 return values
        result = lua_pcall(levelL, 1, 0, 0);
        if (result != LUA_OK) {
            const char* error = lua_tostring(levelL, -1);
            LOG_ERROR("LevelLoader", "[TriggerAttackAPAnimation] Failed to call ConsumeOneAP: %s", error);
            lua_pop(levelL, 3);  // Pop error, component, and UIManager table
            return 0;
        }

        LOG_INFO("LevelLoader", "[TriggerAttackAPAnimation] SUCCESS! Animation triggered");

        // Clean up the stack
        lua_pop(levelL, 2);  // Pop component and UIManager table

        return 0;
    }

    /**
     * @brief Restore all attack AP crystals (visual only)
     *
     * Usage: RestoreAllAttackAPCrystals()
     *
     * This is a bridge function that allows entity scripts to restore all attack AP
     * crystal visuals when AP is refilled.
     */
    int LevelLoader::Lua_RestoreAllAttackAPCrystals(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader) {
            return 0;
        }

        // Get the LevelLoader's Lua state (where UIManager is running)
        lua_State* levelL = loader->L;
        if (!levelL) {
            return 0;
        }

        // Get UIManager table
        lua_getglobal(levelL, "UIManager");
        if (!lua_istable(levelL, -1)) {
            lua_pop(levelL, 1);
            return 0;
        }

        // Get UIManager.GetComponent function
        lua_getfield(levelL, -1, "GetComponent");
        if (!lua_isfunction(levelL, -1)) {
            lua_pop(levelL, 2);  // Pop function and UIManager table
            return 0;
        }

        // Push "attackAP" as the argument
        lua_pushstring(levelL, "attackAP");

        // Call UIManager.GetComponent("attackAP") -> 1 argument, 1 return value
        int result = lua_pcall(levelL, 1, 1, 0);
        if (result != LUA_OK) {
            lua_pop(levelL, 2);  // Pop error and UIManager table
            return 0;
        }

        // Now we have the attackAP component on the stack
        if (!lua_istable(levelL, -1)) {
            lua_pop(levelL, 2);  // Pop component and UIManager table
            return 0;
        }

        // Get the RestoreAllAP method from the component
        lua_getfield(levelL, -1, "RestoreAllAP");
        if (!lua_isfunction(levelL, -1)) {
            lua_pop(levelL, 3);  // Pop function, component, and UIManager table
            return 0;
        }

        // Push the component table as 'self' for the method call
        lua_pushvalue(levelL, -2);  // Duplicate the component table

        // Call attackAPComponent:RestoreAllAP() -> 1 argument (self), 0 return values
        result = lua_pcall(levelL, 1, 0, 0);
        if (result != LUA_OK) {
            lua_pop(levelL, 3);  // Pop error, component, and UIManager table
            return 0;
        }

        // Clean up the stack
        lua_pop(levelL, 2);  // Pop component and UIManager table

        return 0;
    }

    // ========================================================================
    // ANIMATION CONTROL API
    // ========================================================================

    /**
     * @brief Set animation group for an entity
     * Lua usage: SetAnimationGroup(entityID, group)
     * @param entityID Entity ID
     * @param group Animation group (0=Idle, 1=Walk, 2=Attack, 3=Injured, 4=Death)
     */
    int LevelLoader::Lua_SetAnimationGroup(lua_State* L) {
        lua_Integer entityID = luaL_checkinteger(L, 1);
        lua_Integer group = luaL_checkinteger(L, 2);

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        Entity e(static_cast<EntityID>(entityID));
        if (!em->HasComponent<SpriteAnimation>(e)) {
            LOG_WARN("LUA_ANIM", "Entity %u has no SpriteAnimation component", entityID);
            return 0;
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        anim.group = static_cast<AnimGroup>(group);

        return 0;
    }

    /**
     * @brief Set animation direction for an entity
     * Lua usage: SetAnimationDirection(entityID, direction)
     * @param entityID Entity ID
     * @param direction Animation direction (0=Front, 1=Back, 2=Side, 3=None)
     */
    int LevelLoader::Lua_SetAnimationDirection(lua_State* L) {
        lua_Integer entityID = luaL_checkinteger(L, 1);
        lua_Integer direction = luaL_checkinteger(L, 2);

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        Entity e(static_cast<EntityID>(entityID));
        if (!em->HasComponent<SpriteAnimation>(e)) {
            LOG_WARN("LUA_ANIM", "Entity %u has no SpriteAnimation component", entityID);
            return 0;
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        anim.direction = static_cast<AnimDirection>(direction);

        return 0;
    }

    /**
     * @brief Set horizontal flip for an entity's sprite
     * Lua usage: SetAnimationFlipX(entityID, flipX)
     * @param entityID Entity ID
     * @param flipX true to flip horizontally, false for normal
     */
    int LevelLoader::Lua_SetAnimationFlipX(lua_State* L) {
        lua_Integer entityID = luaL_checkinteger(L, 1);
        bool flipX = lua_toboolean(L, 2);

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        Entity e(static_cast<EntityID>(entityID));
        if (!em->HasComponent<SpriteAnimation>(e)) {
            LOG_WARN("LUA_ANIM", "Entity %u has no SpriteAnimation component", entityID);
            return 0;
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        anim.flipX = flipX;

        return 0;
    }

    /**
     * @brief Set whether animation is playing
     * Lua usage: SetAnimationPlaying(entityID, playing)
     * @param entityID Entity ID
     * @param playing true to play, false to pause
     */
    int LevelLoader::Lua_SetAnimationPlaying(lua_State* L) {
        lua_Integer entityID = luaL_checkinteger(L, 1);
        bool playing = lua_toboolean(L, 2);

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        Entity e(static_cast<EntityID>(entityID));
        if (!em->HasComponent<SpriteAnimation>(e)) {
            LOG_WARN("LUA_ANIM", "Entity %u has no SpriteAnimation component", entityID);
            return 0;
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        anim.playing = playing;

        return 0;
    }

    /**
     * @brief Set whether animation should loop
     * Lua usage: SetAnimationLoop(entityID, loop)
     * @param entityID Entity ID
     * @param loop true to loop, false for one-shot
     */
    int LevelLoader::Lua_SetAnimationLoop(lua_State* L) {
        lua_Integer entityID = luaL_checkinteger(L, 1);
        bool loop = lua_toboolean(L, 2);

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        Entity e(static_cast<EntityID>(entityID));
        if (!em->HasComponent<SpriteAnimation>(e)) {
            LOG_WARN("LUA_ANIM", "Entity %u has no SpriteAnimation component", entityID);
            return 0;
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        anim.loop = loop;

        return 0;
    }

    /**
     * @brief Set animation frame range (startFrame and frameCount)
     * Lua usage: SetAnimationFrameRange(entityID, startFrame, frameCount, resetToStart)
     * @param entityID Entity ID
     * @param startFrame First frame index in the animation range
     * @param frameCount Number of frames in the animation
     * @param resetToStart (optional) If true, reset currentFrame to 0 (default: true)
     */
    int LevelLoader::Lua_SetAnimationFrameRange(lua_State* L) {
        lua_Integer entityID = luaL_checkinteger(L, 1);
        int startFrame = static_cast<int>(luaL_checkinteger(L, 2));
        int frameCount = static_cast<int>(luaL_checkinteger(L, 3));
        bool resetToStart = true;
        if (lua_gettop(L) >= 4) {
            resetToStart = lua_toboolean(L, 4);
        }

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) return 0;

        Entity e(static_cast<EntityID>(entityID));
        if (!em->HasComponent<SpriteAnimation>(e)) {
            LOG_WARN("LUA_ANIM", "SetAnimationFrameRange: Entity %lld has no SpriteAnimation component", entityID);
            return 0;
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        anim.startFrame = startFrame;
        anim.frameCount = frameCount;

        if (resetToStart) {
            anim.currentFrame = 0;
            anim.elapsedTime = 0.0f;
        }

        LOG_INFO("LUA_ANIM", "SetAnimationFrameRange: Entity %lld -> startFrame=%d, frameCount=%d",
                 entityID, startFrame, frameCount);

        return 0;
    }

    /**
     * @brief Get current animation group for an entity
     * Lua usage: group = GetAnimationGroup(entityID)
     * @param entityID Entity ID
     * @return Animation group (0=Idle, 1=Walk, 2=Attack, 3=Injured, 4=Death)
     */
    int LevelLoader::Lua_GetAnimationGroup(lua_State* L) {
        lua_Integer entityID = luaL_checkinteger(L, 1);

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            lua_pushinteger(L, 0);  // Default to Idle
            return 1;
        }

        Entity e(static_cast<EntityID>(entityID));
        if (!em->HasComponent<SpriteAnimation>(e)) {
            lua_pushinteger(L, 0);  // Default to Idle
            return 1;
        }

        auto& anim = em->GetComponent<SpriteAnimation>(e);
        lua_pushinteger(L, static_cast<int>(anim.group));
        return 1;
    }

    /**
     * @brief Get movement direction for an entity
     * Lua usage: dirX, dirY = GetEntityMovementDirection(entityID)
     * @param entityID Entity ID
     * @return dirX, dirY Movement vector components (0, 0 if no Movement component)
     */
    int LevelLoader::Lua_GetEntityMovementDirection(lua_State* L) {
        lua_Integer entityID = luaL_checkinteger(L, 1);

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            lua_pushnumber(L, 0.0);
            lua_pushnumber(L, 0.0);
            return 2;
        }

        Entity e(static_cast<EntityID>(entityID));
        if (!em->HasComponent<Movement>(e)) {
            lua_pushnumber(L, 0.0);
            lua_pushnumber(L, 0.0);
            return 2;
        }

        auto& movement = em->GetComponent<Movement>(e);
        lua_pushnumber(L, movement.direction.x);
        lua_pushnumber(L, movement.direction.y);
        return 2;
    }

    // ============================================================================
    // ENEMY TURN MANAGEMENT SYSTEM (C++ Implementation for Entity Scripts)
    // ============================================================================

    // Static state for enemy turn management
    namespace EnemyTurnState {
        static bool turnActive = false;
        static int activeEnemyIndex = 0;
        static float actionTimer = 0.0f;
        static float actionDelay = 0.5f;  // 0.5 seconds between enemies
        static std::vector<int> enemyList;
        static bool needsReinitialize = true;
    }

    /**
     * @brief Initialize enemy turn system
     * Lua usage: InitializeEnemyTurn()
     * Call this when enemy turn starts
     */
    int LevelLoader::Lua_InitializeEnemyTurn(lua_State* L) {
        LOG_INFO("LevelLoader", "[EnemyTurnSystem] InitializeEnemyTurn() called");

        auto* em = CORE ? CORE->GetEntityManager() : nullptr;
        if (!em) {
            LOG_ERROR("LevelLoader", "[EnemyTurnSystem] No EntityManager");
            return 0;
        }

        // Get all enemies
        EnemyTurnState::enemyList.clear();
        for (Entity e : em->GetAllEntities()) {
            if (em->HasComponent<EnemyAI>(e)) {
                EnemyTurnState::enemyList.push_back(e.GetID());
                LOG_INFO("LevelLoader", "[EnemyTurnSystem] Found enemy: %d", e.GetID());
            }
        }

        LOG_INFO("LevelLoader", "[EnemyTurnSystem] Found %d enemies total", (int)EnemyTurnState::enemyList.size());

        if (EnemyTurnState::enemyList.empty()) {
            LOG_WARN("LevelLoader", "[EnemyTurnSystem] No enemies found!");
            EnemyTurnState::turnActive = false;
            return 0;
        }

        // Start with first enemy
        EnemyTurnState::turnActive = true;
        EnemyTurnState::activeEnemyIndex = 1;  // 1-indexed like Lua
        EnemyTurnState::actionTimer = 0.0f;  // First enemy acts immediately
        EnemyTurnState::needsReinitialize = false;

        LOG_INFO("LevelLoader", "[EnemyTurnSystem] Starting with enemy %d (index 1/%d)",
            EnemyTurnState::enemyList[0], (int)EnemyTurnState::enemyList.size());

        return 0;
    }

    /**
     * @brief Check if specific enemy is the active one
     * Lua usage: local isActive = IsActiveEnemy(entityID)
     * @param entityID Entity ID to check
     * @return true if this enemy should act, false otherwise
     */
    int LevelLoader::Lua_IsActiveEnemy(lua_State* L) {
        int entityID = (int)luaL_checkinteger(L, 1);

        // Reinitialize if needed
        if (EnemyTurnState::needsReinitialize) {
            Lua_InitializeEnemyTurn(L);
        }

        if (!EnemyTurnState::turnActive || EnemyTurnState::activeEnemyIndex == 0) {
            lua_pushboolean(L, false);
            return 1;
        }

        if (EnemyTurnState::activeEnemyIndex > (int)EnemyTurnState::enemyList.size()) {
            lua_pushboolean(L, false);
            return 1;
        }

        int activeID = EnemyTurnState::enemyList[EnemyTurnState::activeEnemyIndex - 1];
        bool isActive = (entityID == activeID);

        lua_pushboolean(L, isActive);
        return 1;
    }

    /**
     * @brief Check if enemy action timer is ready
     * Lua usage: local ready = IsEnemyActionReady()
     * @return true if enemy can act (timer expired), false if still in delay
     */
    int LevelLoader::Lua_IsEnemyActionReady(lua_State* L) {
        bool ready = EnemyTurnState::turnActive && (EnemyTurnState::actionTimer <= 0.0f);
        lua_pushboolean(L, ready);
        return 1;
    }

    /**
     * @brief Mark current enemy as done and advance to next
     * Lua usage: MarkEnemyActionComplete()
     * Call this when enemy finishes its turn
     */
    int LevelLoader::Lua_MarkEnemyActionComplete(lua_State* L) {
        if (!EnemyTurnState::turnActive) {
            LOG_WARN("LevelLoader", "[EnemyTurnSystem] MarkEnemyActionComplete called but turn not active");
            return 0;
        }

        LOG_INFO("LevelLoader", "[EnemyTurnSystem] Enemy %d completed action",
            EnemyTurnState::enemyList[EnemyTurnState::activeEnemyIndex - 1]);

        // Move to next enemy
        EnemyTurnState::activeEnemyIndex++;

        // Check if all enemies have acted
        if (EnemyTurnState::activeEnemyIndex > (int)EnemyTurnState::enemyList.size()) {
            LOG_INFO("LevelLoader", "[EnemyTurnSystem] All enemies have acted - ending enemy turn");
            EnemyTurnState::turnActive = false;
            EnemyTurnState::activeEnemyIndex = 0;
            EnemyTurnState::needsReinitialize = true;

            // Call Lua OnEnemyTurnEnded() if it exists
            lua_getglobal(L, "OnEnemyTurnEnded");
            if (lua_isfunction(L, -1)) {
                if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                    const char* err = lua_tostring(L, -1);
                    LOG_ERROR("LevelLoader", "[EnemyTurnSystem] Error calling OnEnemyTurnEnded: %s", err);
                    lua_pop(L, 1);
                }
            } else {
                lua_pop(L, 1);
            }
        } else {
            LOG_INFO("LevelLoader", "[EnemyTurnSystem] Moving to enemy %d (index %d/%d)",
                EnemyTurnState::enemyList[EnemyTurnState::activeEnemyIndex - 1],
                EnemyTurnState::activeEnemyIndex,
                (int)EnemyTurnState::enemyList.size());

            // Reset action timer for next enemy
            EnemyTurnState::actionTimer = EnemyTurnState::actionDelay;
        }

        return 0;
    }

    /**
     * @brief Update enemy turn manager (call every frame)
     * Lua usage: UpdateEnemyTurnManager(dt)
     * @param dt Delta time in seconds
     */
    int LevelLoader::Lua_UpdateEnemyTurnManager(lua_State* L) {
        float dt = (float)luaL_checknumber(L, 1);

        if (!EnemyTurnState::turnActive) {
            return 0;
        }

        // Update action timer
        if (EnemyTurnState::actionTimer > 0.0f) {
            EnemyTurnState::actionTimer -= dt;
            if (EnemyTurnState::actionTimer < 0.0f) {
                EnemyTurnState::actionTimer = 0.0f;
            }
        }

        return 0;
    }

    // ========================================================================
    // TILE OCCUPANCY API
    // ========================================================================

    /**
     * @brief Set the entity occupying a tile
     * @param gridX Grid X coordinate
     * @param gridY Grid Y coordinate
     * @param entityID Entity ID to set as occupant (0 to clear)
     * @return boolean success
     *
     * Usage: SetTileOccupant(x, y, entityID) or SetTileOccupant(x, y, 0) to clear
     */
    int LevelLoader::Lua_SetTileOccupant(lua_State* L) {
        int gridX = static_cast<int>(luaL_checknumber(L, 1));
        int gridY = static_cast<int>(luaL_checknumber(L, 2));
        int entityID = static_cast<int>(luaL_checknumber(L, 3));

        Framework::GridCoord coord{ gridX, gridY };

        if (!Framework::InBounds(coord)) {
            LOG_WARN("LevelLoader", "SetTileOccupant: Grid position (%d, %d) out of bounds", gridX, gridY);
            lua_pushboolean(L, false);
            return 1;
        }

        Entity occupant = (entityID > 0) ? Entity(static_cast<uint32_t>(entityID)) : Entity{ INVALID_ENTITY };
        bool success = Framework::SetOccupant(coord, occupant);

        if (success) {
            LOG_INFO("LevelLoader", "SetTileOccupant: Tile (%d, %d) occupant set to entity %d",
                     gridX, gridY, entityID);
        } else {
            LOG_WARN("LevelLoader", "SetTileOccupant: Failed to set occupant at (%d, %d)", gridX, gridY);
        }

        lua_pushboolean(L, success);
        return 1;
    }

    /**
     * @brief Get the entity occupying a tile
     * @param gridX Grid X coordinate
     * @param gridY Grid Y coordinate
     * @return entityID (0 if no occupant or invalid tile)
     *
     * Usage: local entityID = GetTileOccupant(x, y)
     */
    int LevelLoader::Lua_GetTileOccupant(lua_State* L) {
        int gridX = static_cast<int>(luaL_checknumber(L, 1));
        int gridY = static_cast<int>(luaL_checknumber(L, 2));

        Framework::GridCoord coord{ gridX, gridY };

        if (!Framework::InBounds(coord)) {
            lua_pushinteger(L, 0);
            return 1;
        }

        const auto& grid = Framework::GetGrid();
        Entity tileEntity = grid.TileAt(gridX, gridY);

        if (tileEntity.GetID() == INVALID_ENTITY || !grid.em) {
            lua_pushinteger(L, 0);
            return 1;
        }

        if (!grid.em->HasComponent<GridTiles>(tileEntity)) {
            lua_pushinteger(L, 0);
            return 1;
        }

        const auto& gridTile = grid.em->GetComponent<GridTiles>(tileEntity);
        uint32_t occupantID = gridTile.occupant.GetID();

        lua_pushinteger(L, (occupantID == INVALID_ENTITY) ? 0 : static_cast<lua_Integer>(occupantID));
        return 1;
    }

    /**
     * @brief Check if a tile is occupied by any entity
     * @param gridX Grid X coordinate
     * @param gridY Grid Y coordinate
     * @return boolean true if occupied, false otherwise
     *
     * Usage: local isOccupied = IsTileOccupied(x, y)
     */
    int LevelLoader::Lua_IsTileOccupied(lua_State* L) {
        int gridX = static_cast<int>(luaL_checknumber(L, 1));
        int gridY = static_cast<int>(luaL_checknumber(L, 2));

        Framework::GridCoord coord{ gridX, gridY };

        if (!Framework::InBounds(coord)) {
            lua_pushboolean(L, false);
            return 1;
        }

        const auto& grid = Framework::GetGrid();
        Entity tileEntity = grid.TileAt(gridX, gridY);

        if (tileEntity.GetID() == INVALID_ENTITY || !grid.em) {
            lua_pushboolean(L, false);
            return 1;
        }

        if (!grid.em->HasComponent<GridTiles>(tileEntity)) {
            lua_pushboolean(L, false);
            return 1;
        }

        const auto& gridTile = grid.em->GetComponent<GridTiles>(tileEntity);

        // Check if there's an occupant
        if (gridTile.occupant.GetID() == INVALID_ENTITY) {
            lua_pushboolean(L, false);
            return 1;
        }

        // Check if occupant is dead (dead entities don't count as occupants)
        if (grid.em->HasComponent<Health>(gridTile.occupant)) {
            const auto& health = grid.em->GetComponent<Health>(gridTile.occupant);
            if (health.isDead) {
                lua_pushboolean(L, false);
                return 1;
            }
        }

        lua_pushboolean(L, true);
        return 1;
    }

    // ========================================================================
    // PROJECTILE SKILL API
    // ========================================================================

    /**
     * @brief Spawns a skill-based projectile from Lua
     * @param worldX, worldY  World-space spawn position
     * @param dirX, dirY      Direction vector (will be normalized)
     * @param speed            Projectile speed in world units/second
     * @param damage           Damage dealt on hit
     * @param pierce           Boolean: true = pass through enemies
     * @return entityID of the spawned projectile
     *
     * Usage from Lua:
     *   local projID = SpawnSkillProjectile(wx, wy, dx, dy, 3.0, 2, false)
     */
    int LevelLoader::Lua_SpawnSkillProjectile(lua_State* L) {
        float worldX = static_cast<float>(luaL_checknumber(L, 1));
        float worldY = static_cast<float>(luaL_checknumber(L, 2));
        float dirX   = static_cast<float>(luaL_checknumber(L, 3));
        float dirY   = static_cast<float>(luaL_checknumber(L, 4));
        float speed  = static_cast<float>(luaL_optnumber(L, 5, 3.0));
        int   damage = static_cast<int>(luaL_optinteger(L, 6, 1));
        bool  pierce = lua_toboolean(L, 7) != 0;

        // Normalize direction
        float len = std::sqrt(dirX * dirX + dirY * dirY);
        if (len > 0.0001f) {
            dirX /= len;
            dirY /= len;
        }

        CoreEngine* core = CORE;
        if (!core) {
            lua_pushnil(L);
            return 1;
        }

        EntitySpawner* spawner = core->GetSpawner();
        EntityManager* em = core->GetEntityManager();
        if (!spawner || !em) {
            lua_pushnil(L);
            return 1;
        }

        // Spawn the projectile entity using the existing spawner
        Vector2D position(worldX, worldY);
        Vector2D direction(dirX, dirY);
        Entity projectile = spawner->SpawnProjectile(position, direction, speed);

        // Configure damage and pierce on the projectile component
        if (em->HasComponent<ProjectileMovement>(projectile)) {
            auto& movement = em->GetComponent<ProjectileMovement>(projectile);
            movement.damage = damage;
            movement.pierce = pierce;
        }

        lua_pushinteger(L, projectile.GetID());
        return 1;
    }

} // namespace Framework