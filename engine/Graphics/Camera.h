/**
===============================================================================
 File:           Camera.h
 Author:         Graphics System Overhaul
 Date:           2025-10-07
 ------------------------------------------------------------------------------
 Brief:
 Camera system for managing view and projection transformations.
 Supports orthographic and perspective projections with flexible
 viewport management.

 Design notes:
 - Supports both 2D (orthographic) and 3D (perspective) rendering
 - Automatic aspect ratio handling
 - View frustum calculation for culling (future)
 - Multiple camera support for split-screen or minimaps
===============================================================================
*/
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Framework {

    /**
     * @brief Camera projection types
     */
    enum class ProjectionType {
        Orthographic,
        Perspective
    };

    /**
     * @brief Camera class for view and projection management
     */
    class Camera {
    public:
        /**
         * @brief Construct orthographic camera
         * @param left Left bound
         * @param right Right bound
         * @param bottom Bottom bound
         * @param top Top bound
         * @param nearPlane Near clipping plane
         * @param farPlane Far clipping plane
         */
        Camera(float left, float right, float bottom, float top,
               float nearPlane = -1.0f, float farPlane = 1.0f);

        /**
         * @brief Construct perspective camera
         * @param fov Field of view in degrees
         * @param aspectRatio Width/height ratio
         * @param nearPlane Near clipping plane
         * @param farPlane Far clipping plane
         */
        Camera(float fov, float aspectRatio, float nearPlane, float farPlane);

        // === GETTERS ===
        
        const glm::mat4& GetViewMatrix() const { return viewMatrix; }
        const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }
        glm::mat4 GetViewProjectionMatrix() const { return projectionMatrix * viewMatrix; }

        const glm::vec3& GetPosition() const { return position; }
        const glm::vec3& GetRotation() const { return rotation; }
        float GetZoom() const { return zoom; }

        // === SETTERS ===
        
        void SetPosition(const glm::vec3& pos);
        void SetRotation(const glm::vec3& rot);
        void SetZoom(float z);

        /**
         * @brief Update aspect ratio (for window resize)
         */
        void SetAspectRatio(float aspectRatio);

        /**
         * @brief Set orthographic bounds
         */
        void SetOrthographic(float left, float right, float bottom, float top,
                            float nearPlane = -1.0f, float farPlane = 1.0f);

        /**
         * @brief Set perspective parameters
         */
        void SetPerspective(float fov, float aspectRatio, float nearPlane, float farPlane);

        /**
         * @brief Move camera by offset
         */
        void Translate(const glm::vec3& offset);

        /**
         * @brief Rotate camera by angles (degrees)
         */
        void Rotate(const glm::vec3& angles);

        /**
         * @brief Look at target position
         */
        void LookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0));

    private:
        // Camera transform
        glm::vec3 position;
        glm::vec3 rotation;  // Euler angles in degrees
        float zoom;

        // Matrices
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;

        // Projection parameters
        ProjectionType projectionType;
        
        // Orthographic
        float orthoLeft, orthoRight, orthoBottom, orthoTop;
        
        // Perspective
        float fov;  // Field of view in degrees
        float aspectRatio;
        
        // Common
        float nearPlane, farPlane;

        // Update matrices
        void UpdateViewMatrix();
        void UpdateProjectionMatrix();
    };

    /**
     * @brief Helper to create common camera types
     */
    class CameraFactory {
    public:
        /**
         * @brief Create 2D camera with Y-up coordinates
         * @param width Viewport width in world units
         * @param height Viewport height in world units
         */
        static Camera Create2D(float width, float height);

        /**
         * @brief Create 2D camera matching viewport size
         * @param viewportWidth Viewport width in pixels
         * @param viewportHeight Viewport height in pixels
         */
        static Camera Create2DPixelPerfect(int viewportWidth, int viewportHeight);

        /**
         * @brief Create 3D perspective camera
         * @param fov Field of view in degrees
         * @param aspectRatio Width/height ratio
         */
        static Camera Create3D(float fov, float aspectRatio);
    };

} // namespace Framework
