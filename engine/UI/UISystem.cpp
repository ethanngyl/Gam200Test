/*
===============================================================================
 File:          UISystem.cpp
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Universal UI System (implementation)

  Design notes:
     - Provides a centralized UI management layer integrated with the engine
     - Maintains button lifecycle (create, update, remove, clear)
     - Uses world-space positions for flexible layout in gameplay scenes

  Technical details:
     - Converts mouse coordinates from screen to world space with DPI correction
     - Updates button visual states based on hover, press, or disable state
     - Logs all major UI interactions via the Log system

  Thread-safety:
     - Operates on the main thread only; no synchronization required
===============================================================================
*/


#include "Precompiled.h"

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
        LOG_INFO("UI", "UISystem initialized");
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

            // 打印按钮信息
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

        // Get mouse position in screen space
        float mouseScreenX, mouseScreenY;
        input->GetMousePosition(mouseScreenX, mouseScreenY);

        // Convert to world space
        Vector2D mouseWorld = ScreenToWorld(mouseScreenX, mouseScreenY);

        // Update all buttons
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
    // BUTTON CREATION
    // ========================================================================

    UIButton* UISystem::CreateButton(
        const std::string& texturePath,
        const Vector2D& position,
        const Vector2D& size,
        ButtonCallback onClick)
    {
        return CreateButton(
            texturePath, position, size, onClick,
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),  // Normal
            glm::vec4(1.2f, 1.2f, 1.2f, 1.0f)   // Hover
        );
    }

    UIButton* UISystem::CreateButton(
        const std::string& texturePath,
        const Vector2D& position,
        const Vector2D& size,
        ButtonCallback onClick,
        const glm::vec4& normalTint,
        const glm::vec4& hoverTint)
    {
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
        button->normalTint = normalTint;
        button->hoverTint = hoverTint;

        // Spawn entity
        SpawnButtonEntity(button.get());

        LOG_INFO("UI", "Button created at (%.2f, %.2f) size (%.2f, %.2f)",
            position.x, position.y, size.x, size.y);

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

    Vector2D UISystem::ScreenToWorld(float screenX, float screenY)
    {
        if (!engine) return Vector2D(0, 0);

        auto graphics = engine->GetGraphicsSystem();
        auto windowSystem = engine->GetWindowSystem();

        if (!graphics || !windowSystem) return Vector2D(0, 0);

        GLFWwindow* window = windowSystem->GetWindow();
        if (!window) return Vector2D(0, 0);

        // Get actual window size
        int actualWidth, actualHeight;
        glfwGetWindowSize(window, &actualWidth, &actualHeight);

        // Get framebuffer size (for high-DPI displays)
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        // Calculate DPI scale
        float scaleX = (float)fbWidth / (float)actualWidth;
        float scaleY = (float)fbHeight / (float)actualHeight;

        // Adjust mouse coordinates
        float adjustedX = screenX * scaleX;
        float adjustedY = screenY * scaleY;

        // Convert to NDC (-1 to 1)
        float ndcX = (2.0f * adjustedX) / fbWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * adjustedY) / fbHeight;

        // Convert to world space
        glm::mat4 invViewProj = glm::inverse(graphics->GetCamera().GetViewProjectionMatrix());
        glm::vec4 worldPos = invViewProj * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);

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