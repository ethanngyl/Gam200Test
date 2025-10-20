/*
===============================================================================
 File:          GraphicsSystem.h
 Author:        Sim Kah Yan, TAN WEI LEONG
 Email:         kahyan.sim@digipen.edu, weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  70%(kah yan), 30%(TAN WEI LEONG)
 ------------------------------------------------------------------------------
 Brief:
 Declaration of the GraphicsSystem class, responsible for initializing and
 managing the rendering pipeline. This includes OpenGL context setup, shader
 management, background rendering, mesh creation, and per-frame entity drawing.

 Details:
 - Sets up viewport, shaders, meshes, and background textures during initialization.
 - Handles input for toggling mesh modes and color interpolation effects.
 - Draws both static background and dynamic entities with Transform and Sprite
   components from the ECS.
 - Provides utility functions to manage current mesh color and map sprites to meshes.
 - Acts as the rendering backend for the game engine.

 Description:
 -------------
 This file contains the declaration of the `GraphicsSystem` class, which is
 responsible for managing graphical rendering and input processing within the
 system. It interacts with OpenGL to render different mesh types (triangle,
 quad, line, circle), as well as managing their colors in either static or
 animated (rainbow) modes. Additionally, it supports rendering of entities
 based on the entity-manager framework, leveraging components like `Transform`
 and `Sprite`.

 Notes:
 - Requires GLFW, GLEW, and GLM for rendering.
 - Uses a y-up coordinate system with fixed viewport size (1600x800).
 - Uses orthographic projection for 2D rendering.
 - Assumes EntityManager is set externally before Update() is called.

 Safety:
 - Checks pointers before usage (shader, texture, meshes).
 - Proper cleanup in destructor to release GPU resources.
 - Input handling uses state tracking to prevent key repeat glitches.

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
     * @brief Core rendering system responsible for setting up OpenGL state and
     *        drawing both static background and ECS entities each frame.
     *
     * This class is part of the engine's system architecture. It encapsulates
     * the rendering loop, OpenGL initialization, mesh management, shader usage,
     * and per-frame entity rendering. It also provides features like rainbow
     * color interpolation and background texture rendering.
     */
    class GraphicsSystem : public InterfaceSystem {
    public:
        // Constructor: Initializes member variables.
        GraphicsSystem();

        // Destructor: Cleans up dynamically allocated resources and GPU objects.
        virtual ~GraphicsSystem();

        // Initializes OpenGL context, shaders, meshes, and background texture.
        virtual void Initialize() override;

        // Main per-frame update. Handles rendering and input.
        virtual void Update(float dt) override;

        // Renders all ECS entities with Transform + Sprite components.
        virtual void RenderEntities();

        // Receives and processes engine-wide messages (e.g., Quit).
        virtual void SendEngineMessage(Message* message) override;

        // Sets the ECS EntityManager for this GraphicsSystem.
        void SetEntityManager(EntityManager* em) { entityManager = em; }

        // Sets the active GLFW window pointer.
        void SetWindow(GLFWwindow* win) { window = win; }

    private:
        // Clears the frame buffer at the start of each frame.
        void BeginFrame();

        // Swaps buffers and polls events at the end of each frame.
        void EndFrame();

        // Handles keyboard input for mesh switching and color toggling.
        void ProcessInput();

        // Sets the current mesh color (rainbow or static) in the shader.
        void SetCurrentMeshColor();

        // Returns a mesh pointer corresponding to a given sprite name.
        Mesh* GetMeshForSprite(const std::string& spriteName);

        // Rendering context and systems
        GLFWwindow* window;           // Pointer to active GLFW window.
        Shader* shader;               // Main shader program.
        Mesh* triangleMesh;           // (Legacy) single triangle mesh.
        EntityManager* entityManager; // ECS entity manager.

        std::vector<Mesh*> meshes;         // All available meshes (triangle, quad, line, circle).
        std::vector<glm::vec3> meshColors; // Base colors for each mesh.

        int currentMeshIndex = 0;     // Index of currently selected mesh.

        // Interpolation-related variables
        float colorLerpTime = 0.0f;   // Timer for color interpolation.
        float colorLerpSpeed = 0.25f; // Speed of rainbow effect.
        bool interpolateColor = true; // Toggle between rainbow/static color
    };

} // namespace Framework
