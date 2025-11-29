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
#include "Component.h"    // Movement, CircleCollider, AP components
#include "Pathfinding.h"  // EnemyAI component
#include "Turn.h"         // Turn system
#include "Pause/GlobalPauseManager.h"  // GlobalPause namespace

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

        LOG_INFO("LevelLoader", "DrawButtonText #%d this frame: '%s' (buttonID=%lld)",
                 callsThisFrame, text, buttonID);

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

        bool success = TileMapLevelLoader::LoadLevel(jsonPath, spawner, em, startPos, spacing,tileSize);

        if (success) {
            LOG_INFO("LevelLoader", "TileMap loaded successfully from: %s", jsonPath);
        } else {
            LOG_ERROR("LevelLoader", "Failed to load TileMap from: %s", jsonPath);
        }

        lua_pushboolean(L, success);
        return 1;
    }

    // ========================================================================
    // AP INDICATOR / ENTITY MANAGEMENT API
    // ========================================================================

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
        } else {
            LOG_WARN("LevelLoader", "DestroyEntity: Invalid entity (ID=%lld)", entityID);
        }

        return 0;
    }

    int LevelLoader::Lua_GetPlayerAP(lua_State* L) {
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

        if (player.GetID() == INVALID_ENTITY || !em->HasComponent<AP>(player)) {
            LOG_WARN("LevelLoader", "GetPlayerAP: Player not found or has no AP component");
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
            return 2;
        }

        auto& ap = em->GetComponent<AP>(player);

        // Return currentAP, maxAP
        lua_pushinteger(L, ap.actionPoints);
        lua_pushinteger(L, ap.maxActionPoints);
        return 2;
    }

    // Get player's Attack AP (AttackAP component)
    int LevelLoader::Lua_GetPlayerAttackAP(lua_State* L) {
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
     * Lua usage: local turn = GetCurrentTurn() -- 0 = Player, 1 = Enemy
     * @return 0 for Player turn, 1 for Enemy turn
     */
    int LevelLoader::Lua_GetCurrentTurn(lua_State* L) {
        auto& turn = Turn();
        lua_pushinteger(L, turn.phase == TurnPhase::Player ? 0 : 1);
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
        int visibleInt = lua_toboolean(L, 2);  // 0/1 → bool

        auto* em = loader->coreEngine->GetEntityManager();
        if (!em) return 0;

        Entity e(static_cast<uint32_t>(entityID));
        if (!e.IsValid() || !em->HasComponent<MeshRenderer>(e))
            return 0;

        auto& mr = em->GetComponent<MeshRenderer>(e);
        mr.visible = (visibleInt != 0);

        return 0;
    }

    // Toggle editor mode
    int LevelLoader::lua_ToggleEditorMode(lua_State* L) {
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

} // namespace Framework