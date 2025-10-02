/*
===============================================================================
 File:          GraphicsSystem.h
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  100%
 ------------------------------------------------------------------------------
 Declaration of the GraphicsSystem class.

 Description:
 -------------
 This file contains the declaration of the `GraphicsSystem` class, which is
 responsible for managing graphical rendering and input processing within the
 system. It interacts with OpenGL to render different mesh types (triangle,
 quad, line, circle), as well as managing their colors in either static or
 animated (rainbow) modes. Additionally, it supports rendering of entities
 based on the entity-manager framework, leveraging components like `Transform`
 and `Sprite`.

 Responsibilities:
 -----------------
 - `Initialize()`: Sets up OpenGL, loads shaders, initializes meshes and
   colors, and configures the rendering environment.
 - `Update()`: Updates the graphics system each frame by processing input,
   rendering entities, and handling mesh cycling and color changes.
 - `RenderEntities()`: Renders all entities with a `Transform` and `Sprite`
   component by using the appropriate mesh and applying transformations.
 - `SendEngineMessage()`: Processes system messages, such as handling a
   quit signal.
 - `ProcessInput()`: Handles user input for cycling through meshes and
   toggling color interpolation modes (static or animated).
 - `SetCurrentMeshColor()`: Sets the color for the current mesh based on
   the selected color mode (either static or dynamic).

 Platform-specific Notes:
 -------------------------
 - On Windows, the system uses OpenGL with GLEW for extension handling and
   provides basic OpenGL debugging support.
 - Shader paths are hardcoded to `"shaders/basic.vert"` and
   `"shaders/basic.frag"`. Ensure these files are available in the appropriate
   directory.
 - Color interpolation (dynamic rainbow effect) can be toggled with the
   spacebar. Pressing space will switch between a static color and a dynamic
   rainbow effect for the current mesh.

 Safety:
 --------
 - All functions are `noexcept` where practical, ensuring that even if an
   exception is thrown or a crash occurs, the system will attempt to log
   useful crash information.
 - The system makes a best-effort attempt to create a crash report, which is
   saved in a timestamped file in the executable's directory.
 - The `force_crash_for_test()` function can deliberately trigger a crash
   for testing crash logging behavior.
===============================================================================
*/

#pragma once
#include "Precompiled.h"  // Includes essential precompiled headers for the graphics system.
#include <glm/glm.hpp>
#include "ECSEntityManager.h"

// Forward declarations
struct GLFWwindow;

namespace Framework {
    class Shader;
    class Mesh;
}

namespace Framework {

    /**
     * @class GraphicsSystem
     * @brief A system responsible for rendering meshes and handling graphical input.
     *
     * The `GraphicsSystem` class manages the initialization and rendering of
     * graphical elements using OpenGL. It supports rendering multiple mesh
     * types, handling color interpolation (static or dynamic), and rendering
     * entities based on an entity-component system (ECS).
     */
    class GraphicsSystem : public InterfaceSystem {
    public:
        /**
         * @brief Constructs a new GraphicsSystem object.
         *
         * Initializes default values for member variables and prepares the system
         * for initialization.
         */
        GraphicsSystem();

        /**
         * @brief Destroys the GraphicsSystem object.
         *
         * Cleans up resources such as deleting shaders and meshes when the
         * graphics system is no longer needed.
         */
        virtual ~GraphicsSystem();

        /**
        * @brief Initializes the graphics system, including OpenGL context, shaders,
        * meshes, and colors.
        *
        * This function sets up the OpenGL context, loads the necessary shaders,
        * and creates default meshes and colors. It also prints information about
        * the OpenGL version and capabilities.
        */
        virtual void Initialize() override;

        /**
         * @brief Updates the graphics system each frame.
         *
         * This method handles frame updates, including rendering entities,
         * processing input (e.g., mesh cycling and color mode toggling), and
         * updating graphical elements.
         *
         * @param dt The time delta (in seconds) since the last frame.
         */
        virtual void Update(float dt) override;

        /**
         * @brief Renders all entities in the scene.
         *
         * This function retrieves all entities that have both `Transform` and
         * `Sprite` components, and renders them with the corresponding mesh,
         * applying the appropriate transformations (position, scale, rotation).
         */
        virtual void RenderEntities();

        /**
         * @brief Processes messages sent to the graphics system.
         *
         * This function processes messages from the engine, such as a quit
         * message, and performs actions based on the message type.
         *
         * @param message The message to process.
         */
        virtual void SendEngineMessage(Message* message) override;

        /**
         * @brief Sets the EntityManager for the graphics system.
         *
         * This allows the graphics system to access the entities and components
         * managed by the `EntityManager`, enabling entity rendering.
         *
         * @param em Pointer to the EntityManager instance.
         */
        void SetEntityManager(EntityManager* em) { entityManager = em; }

        /**
         * @brief Sets the window for the graphics system.
         *
         * This allows the graphics system to interact with the GLFW window,
         * enabling rendering and input handling.
         *
         * @param win Pointer to the GLFW window.
         */
        void SetWindow(GLFWwindow* win) { window = win; }

    private:
        /**
                * @brief Begins a new frame by clearing the screen.
                *
                * Clears the color and depth buffers to prepare for the next frame of
                * rendering.
                */
        void BeginFrame();

        /**
         * @brief Ends the current frame by swapping the buffers.
         *
         * Swaps the front and back buffers to display the rendered content on
         * the screen, and processes any pending events.
         */
        void EndFrame();

        /**
         * @brief Handles user input for interaction with the system.
         *
         * This function processes keyboard input, including actions like
         * switching meshes and toggling color interpolation (static vs dynamic).
         */
        void ProcessInput();

        /**
         * @brief Sets the color for the current mesh based on the selected color mode.
         *
         * If color interpolation is enabled, this function applies a dynamic
         * rainbow color. If it is disabled, it applies a static color to the mesh.
         */
        void SetCurrentMeshColor();

        /**
        * @brief Retrieves the mesh corresponding to the given sprite name.
        *
        * This function returns a mesh based on the provided sprite name (e.g., "triangle", "quad", etc.).
        * It searches the `meshes` vector to find a matching mesh. If no match is found, it returns
        * the default mesh (first in the list) or nullptr if no meshes are available.
        *
        * @param spriteName The name of the sprite (e.g., "triangle", "quad", etc.).
        * @return A pointer to the corresponding mesh object, or nullptr if no match is found.
        */
        Mesh* GetMeshForSprite(const std::string& spriteName);

        // Variables
        GLFWwindow* window;           ///< Pointer to the GLFW window.
        Shader* shader;               ///< Pointer to the shader program.
        Mesh* triangleMesh;           ///< Pointer to the triangle mesh for rendering.
        EntityManager* entityManager; ///< Pointer to the EntityManager for accessing entities.

        std::vector<Mesh*> meshes;         ///< List of available meshes for rendering.
        std::vector<glm::vec3> meshColors; ///< List of static colors for meshes.

        int currentMeshIndex = 0;     ///< Index of the currently selected mesh.

        // Interpolation-related variables
        float colorLerpTime = 0.0f;   ///< Time used for color interpolation (not actively used).
        float colorLerpSpeed = 0.25f; ///< Speed of color interpolation (if enabled).
        bool interpolateColor = true; ///< Flag to toggle between static and dynamic (rainbow) color modes.
    };

} // namespace Framework
