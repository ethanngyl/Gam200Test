/*
===============================================================================
File:        Camera.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Camera component supporting orthographic (2D) and perspective (3D) projections.
Maintains view and projection matrices based on position, rotation, zoom,
and projection parameters.

Details:
- Two constructors: orthographic and perspective presets.
- Setter methods update view/projection matrices immediately to keep state valid.
- Orthographic zoom implemented by scaling frustum extents.
- Rotation order for view matrix: Z ? Y ? X (applied to identity, then translate).
- Utility factory methods to create 2D cameras with world or pixel-perfect bounds.

Safety:
- Clamps zoom to a small positive minimum to avoid degenerate projections.
- Projection updates whenever dependent parameters change (aspect, fov, bounds).
===============================================================================
*/

#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Framework {

    // =========================================================================
    // Constructors
    // =========================================================================

    // Orthographic constructor: bounds are in world units (y-up assumed by caller).
    Camera::Camera(float left, float right, float bottom, float top,
        float nearPlane, float farPlane)
        : position(0.0f),
        rotation(0.0f),
        zoom(1.0f),
        projectionType(ProjectionType::Orthographic),
        orthoLeft(left), orthoRight(right), orthoBottom(bottom), orthoTop(top),
        fov(45.0f), aspectRatio(1.0f),
        nearPlane(nearPlane), farPlane(farPlane)
    {
        UpdateViewMatrix();
        UpdateProjectionMatrix();
    }

    // Perspective constructor: typical 3D camera with fov/aspect/near/far.
    Camera::Camera(float fov, float aspectRatio, float nearPlane, float farPlane)
        : position(0.0f, 0.0f, 5.0f),
        rotation(0.0f),
        zoom(1.0f),
        projectionType(ProjectionType::Perspective),
        orthoLeft(-1.0f), orthoRight(1.0f), orthoBottom(-1.0f), orthoTop(1.0f),
        fov(fov), aspectRatio(aspectRatio),
        nearPlane(nearPlane), farPlane(farPlane)
    {
        UpdateViewMatrix();
        UpdateProjectionMatrix();
    }

    // =========================================================================
    // Setters / Mutators
    // =========================================================================

    // Set camera position in world space and rebuild the view matrix.
    void Camera::SetPosition(const glm::vec3& pos) {
        position = pos;
        UpdateViewMatrix();
    }

    // Set Euler rotation in degrees (pitch=x, yaw=y, roll=z) and rebuild view.
    void Camera::SetRotation(const glm::vec3& rot) {
        rotation = rot;
        UpdateViewMatrix();
    }

    // Set zoom factor; orthographic frustum is scaled by 1/zoom.
    void Camera::SetZoom(float z) {
        zoom = std::max(0.1f, z);   // prevent zero/negative zoom that breaks ortho
        UpdateProjectionMatrix();
    }

    // Update aspect ratio (used by perspective projection).
    void Camera::SetAspectRatio(float aspect) {
        aspectRatio = aspect;
        UpdateProjectionMatrix();
    }

    // Switch to orthographic mode with new bounds and near/far planes.
    void Camera::SetOrthographic(float left, float right, float bottom, float top,
        float near, float far) {
        projectionType = ProjectionType::Orthographic;
        orthoLeft = left;
        orthoRight = right;
        orthoBottom = bottom;
        orthoTop = top;
        nearPlane = near;
        farPlane = far;
        UpdateProjectionMatrix();
    }

    // Switch to perspective mode with new fov/aspect/near/far.
    void Camera::SetPerspective(float fovDegrees, float aspect, float near, float far) {
        projectionType = ProjectionType::Perspective;
        fov = fovDegrees;
        aspectRatio = aspect;
        nearPlane = near;
        farPlane = far;
        UpdateProjectionMatrix();
    }

    // Translate by an offset in world space and rebuild the view matrix.
    void Camera::Translate(const glm::vec3& offset) {
        position += offset;
        UpdateViewMatrix();
    }

    // Incremental rotation in degrees and rebuild the view matrix.
    void Camera::Rotate(const glm::vec3& angles) {
        rotation += angles;
        UpdateViewMatrix();
    }

    // Point the camera at a world-space target with an up vector.
    // This directly writes the view matrix (position is honored; rotation is not derived).
    void Camera::LookAt(const glm::vec3& target, const glm::vec3& up) {
        viewMatrix = glm::lookAt(position, target, up);
        // If you need rotation to reflect LookAt, extract orientation from viewMatrix separately.
    }

    // =========================================================================
    // Private helpers
    // =========================================================================

    // Rebuild the view matrix from position and Euler rotation.
    void Camera::UpdateViewMatrix() {
        // Start from identity
        viewMatrix = glm::mat4(1.0f);

        // Apply rotations in Z, then Y, then X (roll ? yaw ? pitch).
        // Note: View matrix uses the inverse transform of the camera pose; here
        // we build a camera transform then apply the inverse by translating with -position.
        viewMatrix = glm::rotate(viewMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        viewMatrix = glm::rotate(viewMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        viewMatrix = glm::rotate(viewMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));

        // Apply translation (negative to move the world opposite of camera motion).
        viewMatrix = glm::translate(viewMatrix, -position);
    }

    // Rebuild the projection matrix based on the active projection type.
    void Camera::UpdateProjectionMatrix() {
        if (projectionType == ProjectionType::Orthographic) {
            // Apply zoom by scaling the ortho bounds inward/outward.
            const float zl = orthoLeft / zoom;
            const float zr = orthoRight / zoom;
            const float zb = orthoBottom / zoom;
            const float zt = orthoTop / zoom;

            projectionMatrix = glm::ortho(zl, zr, zb, zt, nearPlane, farPlane);
        }
        else {
            // Standard perspective projection using fov (degrees) and aspect.
            projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        }
    }

    glm::vec2 Camera::GetOrthoHalfExtents() const
    {
        if (projectionType != ProjectionType::Orthographic) {
            return glm::vec2(0.0f);
        }

        // Current visible width/height in world units after zoom
        const float width = (orthoRight - orthoLeft) / zoom;
        const float height = (orthoTop - orthoBottom) / zoom;

        return glm::vec2(width * 0.5f, height * 0.5f);
    }

    // =========================================================================
    // Camera Factory
    // =========================================================================

    // Create a 2D orthographic camera in world units with center at (0,0).
    Camera CameraFactory::Create2D(float width, float height) {
        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;
        return Camera(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
    }

    // Create a pixel-perfect 2D orthographic camera using viewport size (in pixels).
    // Useful for UI layers where 1 unit = 1 pixel.
    Camera CameraFactory::Create2DPixelPerfect(int viewportWidth, int viewportHeight) {
        const float halfWidth = viewportWidth * 0.5f;
        const float halfHeight = viewportHeight * 0.5f;
        return Camera(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
    }

} // namespace Framework
