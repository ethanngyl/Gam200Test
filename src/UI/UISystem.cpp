/*
===============================================================================
 File:          UISystem.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Universal UI System

  Design notes:
     - Provides a centralized UI management layer integrated with the engine
     - Maintains button lifecycle (create, update, remove, clear)
     - Uses world-space positions for flexible layout in gameplay scenes
     - Loads default visual properties from game_config.txt

  Technical details:
     - Converts mouse coordinates from screen to world space with DPI correction
     - Updates button visual states based on hover, press, or disable state
     - Logs all major UI interactions via the Log system
     - Applies configurable default tints to newly created buttons

  Thread-safety:
     - Operates on the main thread only; no synchronization required
===============================================================================
*/


// ========================================================================
// ADDED: ImGui headers for viewport coordinate conversion
// ========================================================================
#include "ImguiSystem.h"
#include "imgui.h"
#include "Precompiled.h"
#include "ConfigReader.h"
#include "EntitySpawner.h"

namespace Framework {

    // ========================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ========================================================================

    UISystem::UISystem(CoreEngine* engine)
        : engine(engine)
        , isEnabled(true)
    {
        LOG_INFO("UI", "UISystem created");
    }

    UISystem::~UISystem()
    {
        ClearAllButtons();
        LOG_INFO("UI", "UISystem destroyed");
    }

    // ========================================================================
    // SYSTEM LIFECYCLE
    // ========================================================================

    void UISystem::Initialize()
    {
        // Load configuration
        LoadConfig();

        LOG_INFO("UI", "UISystem initialized");
        LOG_INFO("UI", "  Default layer: %d", config_.defaultLayer);
        LOG_INFO("UI", "  Default normal tint: (%.2f, %.2f, %.2f, %.2f)",
            config_.defaultNormalTint.r, config_.defaultNormalTint.g,
            config_.defaultNormalTint.b, config_.defaultNormalTint.a);
        LOG_INFO("UI", "  Default hover tint: (%.2f, %.2f, %.2f, %.2f)",
            config_.defaultHoverTint.r, config_.defaultHoverTint.g,
            config_.defaultHoverTint.b, config_.defaultHoverTint.a);
    }


    void UISystem::Update(float dt)
    {
        DBG_SCOPE_SYS("UI System", eng::debug::Subsystem::Gameplay);

        (void)dt;

        if (!isEnabled || !engine) return;

        auto input = engine->GetInputSystem();
        auto windowSystem = engine->GetWindowSystem();

        if (!input || !windowSystem) return;

        GLFWwindow* window = windowSystem->GetWindow();
        if (!window) return;

        static int lastWidth = 0, lastHeight = 0;
        int currentWidth = windowSystem->GetWidth();
        int currentHeight = windowSystem->GetHeight();

        if (currentWidth != lastWidth || currentHeight != lastHeight) {
            LOG_INFO("UI", "Window resized: %dx%d -> %dx%d",
                lastWidth, lastHeight, currentWidth, currentHeight);

            for (size_t i = 0; i < buttons.size(); ++i) {
                if (buttons[i]) {
                    LOG_DEBUG("UI", "  Button[%zu] world pos: (%.2f, %.2f) size: (%.2f, %.2f)",
                        i,
                        buttons[i]->position.x, buttons[i]->position.y,
                        buttons[i]->size.x, buttons[i]->size.y);
                }
            }

            lastWidth = currentWidth;
            lastHeight = currentHeight;
        }


        float mouseScreenX, mouseScreenY;
        bool useViewportCoords = false;

        auto imguiSystem = engine->GetImGuiSystem();
        if (imguiSystem && imguiSystem->IsRenderingToViewport()) {

            ImVec2 mousePos = ImGui::GetMousePos();  
            ImVec2 viewportOffset = imguiSystem->GetViewportPos();  
            ImVec2 viewportSize = imguiSystem->GetViewportSize();   

            float localX = mousePos.x - viewportOffset.x;
            float localY = mousePos.y - viewportOffset.y;

            bool mouseInViewport = (localX >= 0 && localX <= viewportSize.x &&
                localY >= 0 && localY <= viewportSize.y);

            if (!mouseInViewport) {
                for (auto& button : buttons) {
                    if (button) {
                        button->isHovered = false;
                        button->isPressed = false;
                        UpdateButtonVisuals(button.get());
                    }
                }
                return; 
            }

            mouseScreenX = localX;
            mouseScreenY = localY;
            useViewportCoords = true;

            // Debug logging (uncomment if needed)
            // LOG_DEBUG("UI", "Viewport mode: Global (%.1f, %.1f) -> Local (%.1f, %.1f)",
            //           mousePos.x, mousePos.y, localX, localY);
        }
        else {
            input->GetMousePosition(mouseScreenX, mouseScreenY);

            // Debug logging (uncomment if needed)
            // LOG_DEBUG("UI", "Fullscreen mode: Mouse (%.1f, %.1f)", mouseScreenX, mouseScreenY);
        }

        Vector2D mouseWorld = ScreenToWorld(mouseScreenX, mouseScreenY, useViewportCoords);

        for (auto& button : buttons) {
            if (!button) continue;
            UpdateButton(button.get(), mouseWorld);
        }
    }


    void UISystem::SendEngineMessage(Message* message)
    {
        // Handle engine messages if needed
        (void)message;
    }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    void UISystem::LoadConfig()
    {
        // Ensure ConfigReader is loaded
        if (!ConfigReader::IsConfigLoaded()) {
            ConfigReader::LoadConfig();
        }

        // Load default layer
        config_.defaultLayer = ConfigReader::GetInt("ui_button_default_layer", 10);

        // Load normal tint
        config_.defaultNormalTint = glm::vec4(
            ConfigReader::GetFloat("ui_button_normal_tint_r", 1.0f),
            ConfigReader::GetFloat("ui_button_normal_tint_g", 1.0f),
            ConfigReader::GetFloat("ui_button_normal_tint_b", 1.0f),
            ConfigReader::GetFloat("ui_button_normal_tint_a", 1.0f)
        );

        // Load hover tint
        config_.defaultHoverTint = glm::vec4(
            ConfigReader::GetFloat("ui_button_hover_tint_r", 1.2f),
            ConfigReader::GetFloat("ui_button_hover_tint_g", 1.2f),
            ConfigReader::GetFloat("ui_button_hover_tint_b", 1.2f),
            ConfigReader::GetFloat("ui_button_hover_tint_a", 1.0f)
        );

        // Load pressed tint
        config_.defaultPressedTint = glm::vec4(
            ConfigReader::GetFloat("ui_button_pressed_tint_r", 0.9f),
            ConfigReader::GetFloat("ui_button_pressed_tint_g", 0.9f),
            ConfigReader::GetFloat("ui_button_pressed_tint_b", 0.9f),
            ConfigReader::GetFloat("ui_button_pressed_tint_a", 1.0f)
        );

        // Load disabled tint
        config_.defaultDisabledTint = glm::vec4(
            ConfigReader::GetFloat("ui_button_disabled_tint_r", 0.5f),
            ConfigReader::GetFloat("ui_button_disabled_tint_g", 0.5f),
            ConfigReader::GetFloat("ui_button_disabled_tint_b", 0.5f),
            ConfigReader::GetFloat("ui_button_disabled_tint_a", 0.5f)
        );

        // Clamp all tint values to reasonable range [0.0, 2.0]
        auto clampVec4 = [](glm::vec4& v) {
            v.r = std::clamp(v.r, 0.0f, 2.0f);
            v.g = std::clamp(v.g, 0.0f, 2.0f);
            v.b = std::clamp(v.b, 0.0f, 2.0f);
            v.a = std::clamp(v.a, 0.0f, 2.0f);
            };

        clampVec4(config_.defaultNormalTint);
        clampVec4(config_.defaultHoverTint);
        clampVec4(config_.defaultPressedTint);
        clampVec4(config_.defaultDisabledTint);

        // Clamp layer to reasonable range
        if (config_.defaultLayer < 0) config_.defaultLayer = 0;
        if (config_.defaultLayer > 100) config_.defaultLayer = 100;
    }

    void UISystem::ApplyDefaultConfig(UIButton* button)
    {
        if (!button) return;

        button->layer = config_.defaultLayer;
        button->normalTint = config_.defaultNormalTint;
        button->hoverTint = config_.defaultHoverTint;
        button->pressedTint = config_.defaultPressedTint;
        button->disabledTint = config_.defaultDisabledTint;
    }

    // ========================================================================
    // BUTTON CREATION
    // ========================================================================

// ========================================================================
// BUTTON CREATION
// ========================================================================

    UIButton* UISystem::CreateButton(
        const std::string& texturePath,
        const Vector2D& position,
        const Vector2D& size,
        ButtonCallback onClick,
        int layer) // <--- 1. Added parameter
    {
        LOG_INFO("UI_DEBUG", "CreateButton C++ called. Layer value: %d", layer);
        if (!engine) {
            LOG_ERROR("UI", "Engine is null, cannot create button");
            return nullptr;
        }

        // Create button object
        auto button = std::make_unique<UIButton>();
        button->texturePath = texturePath;
        button->position = position;
        button->size = size;
        button->onClick = onClick;

        // Apply default configuration (Sets default layer, tints, etc.)
        ApplyDefaultConfig(button.get());

        // 2. Override the layer with the specific function argument
        button->layer = layer;

        // Spawn entity (Now uses the specific layer)
        SpawnButtonEntity(button.get());

        LOG_INFO("UI", "Button created at (%.2f, %.2f) size (%.2f, %.2f) on layer %d",
            position.x, position.y, size.x, size.y, layer);

        // Store button and return raw pointer
        UIButton* rawPtr = button.get();
        buttons.push_back(std::move(button));

        return rawPtr;
    }

    UIButton* UISystem::CreateButton(
        const std::string& texturePath,
        const Vector2D& position,
        const Vector2D& size,
        ButtonCallback onClick,
        const glm::vec4& normalTint,
        const glm::vec4& hoverTint,
        int layer) // <--- 1. Added parameter
    {
        LOG_INFO("UI_DEBUG", "CreateButton C++ called. Layer value: %d", layer);
        if (!engine) {
            LOG_ERROR("UI", "Engine is null, cannot create button");
            return nullptr;
        }

        // Create button object
        auto button = std::make_unique<UIButton>();
        button->texturePath = texturePath;
        button->position = position;
        button->size = size;
        button->onClick = onClick;

        // Apply default configuration first
        ApplyDefaultConfig(button.get());

        // Then override with custom tints
        button->normalTint = normalTint;
        button->hoverTint = hoverTint;

        // 2. Override the layer
        button->layer = layer;

        // Spawn entity
        SpawnButtonEntity(button.get());

        LOG_INFO("UI", "Button created at (%.2f, %.2f) size (%.2f, %.2f) with custom tints on layer %d",
            position.x, position.y, size.x, size.y, layer);

        // Store button and return raw pointer
        UIButton* rawPtr = button.get();
        buttons.push_back(std::move(button));

        return rawPtr;
    }

    // ========================================================================
    // BUTTON MANAGEMENT
    // ========================================================================

    void UISystem::RemoveButton(UIButton* button)
    {
        if (!button || !engine) return;

        auto entityManager = engine->GetEntityManager();
        if (!entityManager) return;

        // Destroy entity
        if (button->entity.IsValid()) {
            entityManager->DestroyEntity(button->entity);
        }

        // Remove from list
        buttons.erase(
            std::remove_if(buttons.begin(), buttons.end(),
                [button](const std::unique_ptr<UIButton>& b) {
                    return b.get() == button;
                }),
            buttons.end()
        );

        LOG_INFO("UI", "Button removed");
    }

    void UISystem::ClearAllButtons()
    {
        if (!engine) return;

        auto entityManager = engine->GetEntityManager();
        if (!entityManager) return;

        // Destroy all button entities
        for (auto& button : buttons) {
            if (button && button->entity.IsValid()) {
                entityManager->DestroyEntity(button->entity);
            }
        }

        buttons.clear();
        LOG_INFO("UI", "All buttons cleared");
    }

    void UISystem::SetButtonEnabled(UIButton* button, bool enabled)
    {
        if (!button) return;

        button->isEnabled = enabled;
        UpdateButtonVisuals(button);

        LOG_INFO("UI", "Button %s", enabled ? "enabled" : "disabled");
    }

    // ========================================================================
    // COORDINATE CONVERSION
    // ========================================================================


    Vector2D UISystem::ScreenToWorld(float screenX, float screenY, bool useViewportCoords)
    {
        if (!engine) return Vector2D(0, 0);

        auto graphics = engine->GetGraphicsSystem();
        auto windowSystem = engine->GetWindowSystem();

        if (!graphics || !windowSystem) return Vector2D(0, 0);

        GLFWwindow* window = windowSystem->GetWindow();
        if (!window) return Vector2D(0, 0);

        // ========================================================================
        // STEP 1: Understand input coordinates
        // ========================================================================
        // glfwGetCursorPos() returns coordinates relative to the TOP-LEFT of window
        // (0, 0) = top-left corner of the window
        // (windowWidth, windowHeight) = bottom-right corner
        //
        // These are NOT screen coordinates (which would be global across all monitors)

        // ========================================================================
        // STEP 2: Get window and framebuffer sizes
        // ========================================================================
        // 🔧 FIX: Handle Viewport mode separately
        // ========================================================================

        int fbWidth, fbHeight;
        int windowWidth, windowHeight;

        if (useViewportCoords) {
            auto imguiSystem = engine->GetImGuiSystem();
            if (imguiSystem && imguiSystem->IsRenderingToViewport()) {
                ImVec2 viewportSize = imguiSystem->GetViewportSize();

                fbWidth = static_cast<int>(viewportSize.x);
                fbHeight = static_cast<int>(viewportSize.y);
                windowWidth = fbWidth;
                windowHeight = fbHeight;

                // Debug logging (uncomment if needed)
                // LOG_DEBUG("UI", "ScreenToWorld (Viewport mode): input (%.1f, %.1f), viewport (%d, %d)",
                //           screenX, screenY, fbWidth, fbHeight);
            }
            else {
                glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
                glfwGetWindowSize(window, &windowWidth, &windowHeight);
            }
        }
        else {
            // Framebuffer size = actual pixels OpenGL renders to
            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

            // Window size = what GLFW reports for cursor/window dimensions
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
        }

        // Key insight: On high-DPI displays, framebuffer can be 2x window size
        // Example (Retina display):
        //   Window: 1600x800
        //   Framebuffer: 3200x1600
        //   Scale: 2.0x

        // ========================================================================
        // STEP 3: Convert from window space to framebuffer space
        // ========================================================================

        float fbX = screenX * ((float)fbWidth / (float)windowWidth);
        float fbY = screenY * ((float)fbHeight / (float)windowHeight);

        // Example on 2x display:
        // Mouse at window (100, 50)
        // -> Framebuffer (200, 100)

        // ========================================================================
        // STEP 4: Convert framebuffer to NDC (Normalized Device Coordinates)
        // ========================================================================

        // Framebuffer: (0, 0) = top-left, (fbWidth, fbHeight) = bottom-right
        // NDC: (-1, 1) = top-left, (1, -1) = bottom-right

        float ndcX = (2.0f * fbX / (float)fbWidth) - 1.0f;
        float ndcY = 1.0f - (2.0f * fbY / (float)fbHeight);

        // Explanation:
        // fbX = 0        -> ndcX = -1.0 (left edge)
        // fbX = fbWidth  -> ndcX = +1.0 (right edge)
        // fbY = 0        -> ndcY = +1.0 (top edge)
        // fbY = fbHeight -> ndcY = -1.0 (bottom edge)

        // ========================================================================
        // STEP 5: Convert NDC to world space
        // ========================================================================

        glm::mat4 invViewProj;
        if (engine->IsPlaying())
        {
            // Gameplay: use main camera
            invViewProj =
                glm::inverse(graphics->GetCamera().GetViewProjectionMatrix());
        }
        else
        {
            // Editor mode: use editor camera
            invViewProj =
                glm::inverse(graphics->GetEditorCamera().GetViewProjectionMatrix());
        }
        glm::vec4 worldPos = invViewProj * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);

        // The camera's view-projection matrix transforms world -> NDC
        // The inverse transforms NDC -> world

        // ========================================================================
        // OPTIONAL: Debug logging
        // ========================================================================

        // Uncomment to see the transformation pipeline:
        /*
        LOG_DEBUG("UI", "Coordinate Transform:");
        LOG_DEBUG("UI", "  Window:      (%.1f, %.1f)", screenX, screenY);
        LOG_DEBUG("UI", "  Framebuffer: (%.1f, %.1f)", fbX, fbY);
        LOG_DEBUG("UI", "  NDC:         (%.3f, %.3f)", ndcX, ndcY);
        LOG_DEBUG("UI", "  World:       (%.3f, %.3f)", worldPos.x, worldPos.y);
        */

        return Vector2D(worldPos.x, worldPos.y);
    }

    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================

    void UISystem::UpdateButton(UIButton* button, const Vector2D& mouseWorld)
    {
        if (!button || !button->isEnabled) {
            if (button) {
                button->isHovered = false;
                button->isPressed = false;
                UpdateButtonVisuals(button);
            }
            return;
        }

        auto input = engine->GetInputSystem();
        if (!input) return;

        // Check hover
        bool wasHovered = button->isHovered;
        button->isHovered = button->Contains(mouseWorld);

        // Check click
        bool isMouseDown = input->IsKeyDown(MOUSE_LEFT);
        bool isMousePressed = input->IsKeyPressed(MOUSE_LEFT);

        if (button->isHovered) {
            // Mouse over button
            if (isMousePressed) {
                // Just pressed
                button->isPressed = true;
                LOG_INFO("UI", "Button pressed at (%.2f, %.2f)",
                    button->position.x, button->position.y);
            }
            else if (!isMouseDown && button->isPressed) {
                // Released on button = clicked!
                button->isPressed = false;
                if (button->onClick) {
                    LOG_INFO("UI", "Button clicked! Executing callback...");
                    button->onClick();
                }
            }
            else if (isMouseDown) {
                // Holding down
                button->isPressed = true;
            }
        }
        else {
            // Mouse not over button
            button->isPressed = false;
        }

        // Update visuals if state changed
        if (wasHovered != button->isHovered || button->isPressed) {
            UpdateButtonVisuals(button);
        }
    }

    void UISystem::UpdateButtonVisuals(UIButton* button)
    {
        if (!button || !engine) return;

        auto entityManager = engine->GetEntityManager();
        if (!entityManager) return;

        if (!entityManager->HasComponent<Renderable>(button->entity)) {
            return;
        }

        auto& renderable = entityManager->GetComponent<Renderable>(button->entity);

        // Set tint based on state
        if (!button->isEnabled) {
            renderable.tint = button->disabledTint;
        }
        else if (button->isPressed) {
            renderable.tint = button->pressedTint;
        }
        else if (button->isHovered) {
            renderable.tint = button->hoverTint;
        }
        else {
            renderable.tint = button->normalTint;
        }
    }

    void UISystem::SpawnButtonEntity(UIButton* button)
    {
        if (!button || !engine) return;

        auto spawner = engine->GetSpawner();
        auto entityManager = engine->GetEntityManager();

        if (!spawner || !entityManager) {
            LOG_ERROR("UI", "Required systems are null");
            return;
        }

        // Spawn sprite entity
        button->entity = spawner->SpawnSprite(
            button->texturePath,
            button->position,
            button->size
        );

        // Set rendering properties
        if (entityManager->HasComponent<Renderable>(button->entity)) {
            auto& renderable = entityManager->GetComponent<Renderable>(button->entity);
            renderable.layer = button->layer;
            renderable.tint = button->normalTint;
        }
    }

} // namespace Framework