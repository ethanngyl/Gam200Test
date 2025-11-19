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

        // Convert button ID back to pointer
        UIButton* button = reinterpret_cast<UIButton*>(static_cast<intptr_t>(buttonID));

        if (!button) {
            LOG_WARN("LevelLoader", "Invalid button ID for DrawButtonText");
            return 0;
        }

        // Get window system for framebuffer size
        auto windowSystem = loader->coreEngine->GetWindowSystem();
        if (!windowSystem) return 0;

        GLFWwindow* window = windowSystem->GetWindow();
        if (!window) return 0;

        // Get framebuffer size for accurate pixel coordinates
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        // World to Screen coordinate conversion
        // Step 1: Get button's world position
        float worldX = button->position.x;
        float worldY = button->position.y;

        // Step 2: Convert world coordinates to NDC using camera's ViewProjection matrix
        glm::mat4 viewProj = loader->graphicsSystem->GetCamera().GetViewProjectionMatrix();
        glm::vec4 clipSpace = viewProj * glm::vec4(worldX, worldY, 0.0f, 1.0f);

        // Step 3: NDC coordinates
        float ndcX = clipSpace.x;
        float ndcY = clipSpace.y;

        // Step 4: NDC to Screen pixel coordinates
        // NDC range: [-1, 1] -> Screen pixels: [0, fbWidth] and [0, fbHeight]
        float screenX = (ndcX + 1.0f) * 0.5f * fbWidth;
        float screenY = (ndcY + 1.0f) * 0.5f * fbHeight;

        // Step 5: Apply text offset and render
        float textX = screenX + offsetX;
        float textY = screenY + offsetY;

        glm::vec3 textColor(colorR, colorG, colorB);
        loader->graphicsSystem->DrawText4(font, text, textX, textY, scale, textColor);

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


} // namespace Framework