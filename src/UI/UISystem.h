/*
===============================================================================
 File:          UISystem.h
 Author:        GE YONGQI
 Email:         yongqi.ge@digipen.edu
 Date:          2025-10-31
 Contribution:  100%
 ------------------------------------------------------------------------------
  Universal UI System (header)

  Responsibilities:
     - Manages all in-game UI elements such as buttons and overlays
     - Handles creation, input detection, and rendering updates for UI objects
     - Provides screen-to-world coordinate conversion and visual feedback

  Highlights:
     - Button creation API supporting callbacks and custom tint states
     - DPI-aware coordinate transformation for consistent input detection
     - Integrated with engine systems (InputSystem, GraphicsSystem, EntityManager)
     - Configurable default visual properties via game_config.txt
===============================================================================
*/

#pragma once
#include "Precompiled.h"

namespace Framework {

    // Forward declarations
    class CoreEngine;
    class Camera;

    /**
     * @brief Callback function type for button clicks
     * Usage: void OnPlayClicked() { ... }
     */
    using ButtonCallback = std::function<void()>;

    /**
     * @brief UI Button structure
     * Represents a clickable button in world space
     */
    struct UIButton {
        Entity entity;              // Button entity
        Vector2D position;          // Center position in world space
        Vector2D size;              // Button size (width, height)
        std::string texturePath;    // Button texture path
        ButtonCallback onClick;     // Callback when clicked

        // States
        bool isHovered;             // Mouse is over button
        bool isPressed;             // Button is being pressed
        bool isEnabled;             // Button can be interacted with
        int layer;                  // Rendering layer

        // Visual properties
        glm::vec4 normalTint;       // Normal state color
        glm::vec4 hoverTint;        // Hover state color
        glm::vec4 pressedTint;      // Pressed state color
        glm::vec4 disabledTint;     // Disabled state color

        UIButton()
            : entity(0)
            , isHovered(false)
            , isPressed(false)
            , isEnabled(true)
            , layer(10)
            , normalTint(1.0f, 1.0f, 1.0f, 1.0f)
            , hoverTint(1.2f, 1.2f, 1.2f, 1.0f)
            , pressedTint(0.9f, 0.9f, 0.9f, 1.0f)
            , disabledTint(0.5f, 0.5f, 0.5f, 0.5f)
        {
        }

        /**
         * @brief Check if point is inside button (in world space)
         */
        bool Contains(const Vector2D& point) const {
            float halfW = size.x * 0.5f;
            float halfH = size.y * 0.5f;
            return (point.x >= position.x - halfW &&
                point.x <= position.x + halfW &&
                point.y >= position.y - halfH &&
                point.y <= position.y + halfH);
        }
    };

    /**
     * @brief Universal UI System
     * Manages all UI elements including buttons, labels, etc.
     * Handles input, rendering, and lifecycle of UI components
     */
    class UISystem : public EngineSystem {
    public:
        /**
         * @brief Configuration structure for UI system defaults
         */
        struct Config {
            int defaultLayer = 10;
            glm::vec4 defaultNormalTint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            glm::vec4 defaultHoverTint = glm::vec4(1.2f, 1.2f, 1.2f, 1.0f);
            glm::vec4 defaultPressedTint = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
            glm::vec4 defaultDisabledTint = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
        };

        UISystem(CoreEngine* engine);
        virtual ~UISystem();

        // ====================================================================
        // SYSTEM LIFECYCLE
        // ====================================================================
        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void SendEngineMessage(Message* message) override;

        // ====================================================================
        // CONFIGURATION
        // ====================================================================

        /**
         * @brief Load configuration from ConfigReader
         */
        void LoadConfig();

        /**
         * @brief Get current configuration
         */
        const Config& GetConfig() const { return config_; }

        // ====================================================================
        // BUTTON CREATION API
        // ====================================================================

        /**
         * @brief Create a new button with default visual settings
         * @param texturePath Path to button texture
         * @param position Button center position in world space
         * @param size Button size (width, height)
         * @param onClick Callback function when clicked
         * @return Pointer to created button (managed by UISystem)
         */
        UIButton* CreateButton(
            const std::string& texturePath,
            const Vector2D& position,
            const Vector2D& size,
            ButtonCallback onClick,
            int layer
        );

        /**
         * @brief Create a button with custom colors
         */
        UIButton* CreateButton(
            const std::string& texturePath,
            const Vector2D& position,
            const Vector2D& size,
            ButtonCallback onClick,
            const glm::vec4& normalTint,
            const glm::vec4& hoverTint,
            int layer
        );

        // ====================================================================
        // BUTTON MANAGEMENT
        // ====================================================================

        /**
         * @brief Remove a button from the system
         */
        void RemoveButton(UIButton* button);

        /**
         * @brief Clear all buttons
         */
        void ClearAllButtons();

        /**
         * @brief Enable/disable a button
         */
        void SetButtonEnabled(UIButton* button, bool enabled);

        /**
         * @brief Get number of buttons
         */
        size_t GetButtonCount() const { return buttons.size(); }

        // ====================================================================
        // COORDINATE CONVERSION
        // ====================================================================

        /**
         * @brief Convert screen coordinates to world coordinates
         * Handles window resizing and high-DPI displays
         */
        Vector2D ScreenToWorld(float screenX, float screenY, bool useViewportCoords = false);

        // ====================================================================
        // UTILITY
        // ====================================================================

        /**
         * @brief Enable/disable the entire UI system
         */
        void SetEnabled(bool enabled) { isEnabled = enabled; }
        bool IsEnabled() const { return isEnabled; }

    private:
        CoreEngine* engine;
        std::vector<std::unique_ptr<UIButton>> buttons;
        bool isEnabled;
        Config config_;

        // Helper functions
        void UpdateButton(UIButton* button, const Vector2D& mouseWorld);
        void UpdateButtonVisuals(UIButton* button);
        void SpawnButtonEntity(UIButton* button);

        // Apply default configuration to a button
        void ApplyDefaultConfig(UIButton* button);
    };

} // namespace Framework