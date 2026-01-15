/*
===============================================================================
 File:          LevelLoader_API.cpp (Compatible with InputSystem)
 Author:        ETHAN NG
 Email:         n.ethanyongle@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Level Loader Lua API Implementation

 Overview:
    The LevelLoader_API file serves as the bridge between the C++ engine core
    and the Lua scripting layer. It defines a suite of static functions
    registered to Lua, allowing scripts to control Audio, UI, Input, Entities,
    and Grid-based gameplay logic.

  Design notes:
     - Uses standard Lua C API (lua_State*) for all function bindings
     - Validates engine system pointers (Audio, UI, Graphics) before execution
     - Handles coordinate space conversions (World to Screen, World to Grid)
     - Integrates deeply with ECS to manipulate components (AP, Transform, Health)
===============================================================================
*/

#include "Precompiled.h"
#include "LevelLoader.h"
#include "UISystem.h"
#include "Audio/AudioSystem.h"
#include "GraphicsSystemV2.h"
#include "Input.h"
#include "LevelLoader_JSON.h"
#include "ImguiSystem.h"
#include "TileMapLoader.h"
#include "Component.h"    // Movement, CircleCollider, AP components
#include "Pathfinding.h"  // EnemyAI component
#include "Turn.h"         // Turn system
#include "Pause/GlobalPauseManager.h"  // GlobalPause namespace
#include "Grid/GridECS.h" // Grid system functions
#include "PlayerManager.h"
#include "SaveLoadSystem.h"  // JSON Save/Load system

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
            LOG_INFO("LevelLoader", "Button created with layer %d", layer);
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

        // DEBUG: Track how many times DrawButtonText is called per frame
        static int frameCounter = 0;
        static int currentFrame = -1;
        static int callsThisFrame = 0;

        int thisFrame = static_cast<int>(frameCounter / 60); // Approximate frame number
        if (thisFrame != currentFrame) {
            if (callsThisFrame > 2) {  // Expected: 2 calls (Play + Exit buttons)
                LOG_WARN("LevelLoader", "!!! DrawButtonText called %d times last frame (expected 2) !!!", callsThisFrame);
            }
            currentFrame = thisFrame;
            callsThisFrame = 0;
        }
        callsThisFrame++;
        frameCounter++;

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

        static int debugCount = 0;
        if (debugCount++ % 60 == 0) {  // Print once per second
            LOG_INFO("LevelLoader", "========== DrawButtonText: '%s' ==========", text);
            LOG_INFO("LevelLoader", "  Editor: %s | FBO: %u",
                editorEnabled ? "ENABLED" : "DISABLED",
                editorEnabled ? imgui->GetViewportFBO() : 0);
            LOG_INFO("LevelLoader", "  Viewport: %dx%d", fbWidth, fbHeight);
            LOG_INFO("LevelLoader", "  World pos: (%.2f, %.2f)", worldX, worldY);
            LOG_INFO("LevelLoader", "  Screen: (%.2f, %.2f)", screenX, screenY);
            LOG_INFO("LevelLoader", "  Final: (%.2f, %.2f) [SNAPPED TO BUTTON]", centeredX, centeredY);
            LOG_INFO("LevelLoader", "  Scale: base=%.2f viewport=%.2f final=%.2f",
                scale, viewportScale, finalScale);
            LOG_INFO("LevelLoader", "  Offsets: raw=(%.2f, %.2f) scaled=(%.2f, %.2f)",
                offsetX, offsetY, scaledOffsetX, scaledOffsetY);
            LOG_INFO("LevelLoader", "========================================");
        }
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

                static int bindLog = 0;
                if (bindLog++ % 60 == 0) {
                    LOG_INFO("LevelLoader", "Text rendering to viewport FBO %u (was %u)", viewportFBO, previousFBO);
                }
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
        // Check key state
        bool pressed = (keyCode != KEY_UNKNOWN) && input->IsKeyPressed(keyCode);
        lua_pushboolean(L, pressed);
        return 1;
    }

    int LevelLoader::Lua_LoadJSON(lua_State* L) {
        const char* filepath = luaL_checkstring(L, 1);

        LOG_INFO("LevelLoader", "Loading JSON file: %s", filepath);

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
                LOG_INFO("LevelLoader", "Game paused (editor toggled on)");
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
                LOG_INFO("LevelLoader", "Editor enabled via SetEditorMode");
            }
            else {
                imgui->Disable();
                LOG_INFO("LevelLoader", "Editor disabled via SetEditorMode");
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
        LOG_INFO("LevelLoader", "Pause toggled - Now %s", GlobalPause::IsPaused() ? "PAUSED" : "UNPAUSED");
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
                        pc->SetGridMovementEnabled(true);

                        // Optional but recommended: enables walk SFX calls from PlayerManager
                        pc->SetAudioSystem(audio);

                        LOG_INFO("LevelLoader", "Configured PlayerController for player ID=%u (grid movement enabled)", player.GetID());

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

            LOG_INFO("LevelLoader", "TileMap loaded successfully from: %s", jsonPath);
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

        LOG_INFO("LevelLoader", "Spawned sprite '%s' at (%.2f, %.2f) size (%.2f, %.2f), layer=%d, rotation=%.1f°, ID=%u",
            texture, x, y, width, height, layer, rotation, entity.GetID());

        // Return entity ID as integer
        lua_pushinteger(L, static_cast<lua_Integer>(entity.GetID()));
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
        if (logThrottle++ % 60 == 0) {  // Log once per second
            LOG_INFO("LevelLoader", "SetSpriteColor: Entity %lld -> tint(%.2f, %.2f, %.2f, %.2f)",
                entityID, r, g, b, a);
        }

        return 0;
    }

    int LevelLoader::Lua_SetSpriteTexture(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) return 0;

        // Parse parameters: SetSpriteTexture(entityID, texturePath)
        lua_Integer entityID = luaL_checkinteger(L, 1);
        const char* texturePath = luaL_checkstring(L, 2);

        auto* em = loader->coreEngine->GetEntityManager();
        auto* gfx = loader->coreEngine->GetGraphicsSystem();
        if (!em || !gfx) return 0;

        Entity entity(static_cast<uint32_t>(entityID));

        if (!entity.IsValid() || !em->HasComponent<MeshRenderer>(entity)) {
            LOG_WARN("LevelLoader", "SetSpriteTexture: Invalid entity or no MeshRenderer (ID=%lld)", entityID);
            return 0;
        }

        auto& mr = em->GetComponent<MeshRenderer>(entity);
        mr.spriteName = texturePath;

        // Request graphics system to update the mesh and material for this sprite
        gfx->AssignMeshAndMaterial(mr, texturePath);

        LOG_INFO("LevelLoader", "SetSpriteTexture: Entity %lld -> texture '%s'", entityID, texturePath);

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
            LOG_INFO("LevelLoader", "Destroyed entity ID=%lld", entityID);
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

        LOG_INFO("LevelLoader", "Cleared all entities (%zu destroyed)", entityCount);
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

        LOG_INFO("LevelLoader", "GetPlayerAP: Player %u has %d/%d AP",
                 player.GetID(), ap.actionPoints, ap.maxActionPoints);

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
                LOG_INFO("LevelLoader", "FindPlayer: Found player entity ID=%u", e.GetID());
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
                LOG_INFO("LevelLoader", "GetAllEnemies: Found enemy ID=%u", e.GetID());
            }
        }

        LOG_INFO("LevelLoader", "GetAllEnemies: Found %d enemies total", index - 1);
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
     * @brief Pulse tile animation
     * @param x, y Grid coordinates
     * @param scale Pulse scale multiplier
     * @param duration Duration in milliseconds
     */
    int LevelLoader::Lua_PulseTile(lua_State* L) {
        (void)L;
        // int x = static_cast<int>(luaL_checknumber(L, 1));
        // int y = static_cast<int>(luaL_checknumber(L, 2));
        // float scale = static_cast<float>(luaL_checknumber(L, 3));
        // int duration = static_cast<int>(luaL_checknumber(L, 4));

        // TODO: Implement tile pulse animation
        // For now, this is a placeholder
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
     * @brief Refill enemy AP to maximum
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
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            return 0;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        Entity entity(static_cast<uint32_t>(entityID));

        if (!em->HasComponent<Transform>(entity)) {
            return 0;
        }

        auto& transform = em->GetComponent<Transform>(entity);
        auto tileOpt = Framework::WorldToTile(transform.position);

        if (!tileOpt.has_value()) {
            return 0;
        }

        lua_pushnumber(L, tileOpt->x);
        lua_pushnumber(L, tileOpt->y);
        return 2;
    }

    /**
     * @brief Move entity to specified tile
     * @param entityID The entity ID
     * @param x Grid X coordinate
     * @param y Grid Y coordinate
     *
     * Usage: MoveEntityToTile(enemyID, x, y)
     */
    int LevelLoader::Lua_MoveEntityToTile(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            return 0;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        int x = static_cast<int>(luaL_checknumber(L, 2));
        int y = static_cast<int>(luaL_checknumber(L, 3));

        Entity entity(static_cast<uint32_t>(entityID));

        if (!em->HasComponent<Transform>(entity)) {
            return 0;
        }

        auto& transform = em->GetComponent<Transform>(entity);

        // Get current position to clear occupancy
        auto currentTileOpt = Framework::WorldToTile(transform.position);
        if (currentTileOpt.has_value()) {
            Framework::SetOccupant(*currentTileOpt, Framework::Entity{ Framework::INVALID_ENTITY });
        }

        // Move to new position
        Framework::GridCoord newCoord{ x, y };
        transform.position = Framework::TileToWorld(newCoord);
        Framework::SetOccupant(newCoord, entity);

        return 0;
    }

    /**
     * @brief Consume entity's AP
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
            return 0;
        }

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) {
            return 0;
        }

        int entityID = static_cast<int>(luaL_checknumber(L, 1));
        int amount = static_cast<int>(luaL_checknumber(L, 2));

        Entity entity(static_cast<uint32_t>(entityID));

        if (!em->HasComponent<Health>(entity)) {
            return 0;
        }

        auto& health = em->GetComponent<Health>(entity);
        health.currentHealth -= amount;
        if (health.currentHealth <= 0) {
            health.currentHealth = 0;
            health.isDead = true;
        }

        return 0;
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

} // namespace Framework