/**
===============================================================================
 File:           LevelLoader_API.cpp (Compatible with InputSystem)
 Modifications:  Uses Framework::KeyCode enum from Input.h
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

    int LevelLoader::Lua_StopSound(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->audioSystem) return 0;

        const char* soundName = luaL_checkstring(L, 1);
        LOG_WARN("LevelLoader", "StopSound('%s') not supported - AudioSystem only has StopAllSounds()", soundName);

        return 0;
    }

    int LevelLoader::Lua_StopAllSounds(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->audioSystem) return 0;

        loader->audioSystem->StopAllSounds();
        return 0;
    }

    int LevelLoader::Lua_UpdateAudio(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->audioSystem) return 0;

        float dt = luaL_optnumber(L, 1, 0.0f);
        loader->audioSystem->Update(dt);
        return 0;
    }

    // ========================================================================
    // UI BUTTON API
    // ========================================================================

    int LevelLoader::Lua_CreateButton(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->uiSystem) {
            lua_pushinteger(L, 0);
            return 1;
        }

        // Parse parameters
        const char* texture = luaL_checkstring(L, 1);
        float posX = luaL_checknumber(L, 2);
        float posY = luaL_checknumber(L, 3);
        float scaleX = luaL_checknumber(L, 4);
        float scaleY = luaL_checknumber(L, 5);
        const char* callbackName = luaL_checkstring(L, 6);

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

        // Create button through UISystem
        UIButton* button = loader->uiSystem->CreateButton(
            texture,
            Vector2D(posX, posY),
            Vector2D(scaleX, scaleY),
            callback
        );

        if (button) {
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
            fbWidth = imgui->GetViewportWidth();
            fbHeight = imgui->GetViewportHeight();
        }
        else {
            auto windowSystem = loader->coreEngine->GetWindowSystem();
            if (!windowSystem) return 0;
            GLFWwindow* window = windowSystem->GetWindow();
            if (!window) return 0;
            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        }

        // ========================================
        // GET BUTTON'S ACTUAL SCREEN POSITION
        // ========================================
        // Use the button entity's Transform component for accurate positioning
        auto& transform = em->GetComponent<Transform>(button->entity);
        float worldX = transform.position.x;
        float worldY = transform.position.y;

        // Convert button center from world space to screen space
        glm::mat4 viewProj = loader->graphicsSystem->GetCamera().GetViewProjectionMatrix();
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
        // Simply center text at button's screen position with optional offsets
        // The button's screen position IS the center, no need for width estimation
        float centeredX = screenX + offsetX;
        float centeredY = screenY + offsetY;

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
            LOG_INFO("LevelLoader", "  Offsets: X=%.2f Y=%.2f", offsetX, offsetY);
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
    // ========================================================================
    // INPUT API (FIXED for InputSystem)
    // ========================================================================

    int LevelLoader::Lua_IsKeyDown(lua_State* L) {
        LevelLoader* loader = GetLevelLoader(L);
        if (!loader || !loader->coreEngine) {
            lua_pushboolean(L, false);
            return 1;
        }

        const char* keyName = luaL_checkstring(L, 1);
        auto* input = loader->coreEngine->GetInputSystem();
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
    // TILEMAP LOADING API
    // ========================================================================

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

        bool success = TileMapLevelLoader::LoadLevel(jsonPath, spawner, em, startPos, spacing);

        if (success) {
            LOG_INFO("LevelLoader", "TileMap loaded successfully from: %s", jsonPath);
        } else {
            LOG_ERROR("LevelLoader", "Failed to load TileMap from: %s", jsonPath);
        }

        lua_pushboolean(L, success);
        return 1;
    }

} // namespace Framework