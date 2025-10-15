/**
===============================================================================
 File:           Camera.cpp
 Author:         Graphics System Overhaul
 Date:           2025-10-07
 ------------------------------------------------------------------------------
 Brief:
 Implementation of the Camera class for view and projection management.
===============================================================================
*/
#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm> 

namespace Framework {

    // === CONSTRUCTORS ===

    Camera::Camera(float left, float right, float bottom, float top,
        float nearPlane, float farPlane)
        : position(0.0f), rotation(0.0f), zoom(1.0f),
        projectionType(ProjectionType::Orthographic),
        orthoLeft(left), orthoRight(right), orthoBottom(bottom), orthoTop(top),
        fov(45.0f), aspectRatio(1.0f),
        nearPlane(nearPlane), farPlane(farPlane) {
        UpdateViewMatrix();
        UpdateProjectionMatrix();
    }

    Camera::Camera(float fov, float aspectRatio, float nearPlane, float farPlane)
        : position(0.0f, 0.0f, 5.0f), rotation(0.0f), zoom(1.0f),
        projectionType(ProjectionType::Perspective),
        orthoLeft(-1.0f), orthoRight(1.0f), orthoBottom(-1.0f), orthoTop(1.0f),
        fov(fov), aspectRatio(aspectRatio),
        nearPlane(nearPlane), farPlane(farPlane) {
        UpdateViewMatrix();
        UpdateProjectionMatrix();
    }

    // === SETTERS ===

    void Camera::SetPosition(const glm::vec3& pos) {
        position = pos;
        UpdateViewMatrix();
    }

    void Camera::SetRotation(const glm::vec3& rot) {
        rotation = rot;
        UpdateViewMatrix();
    }

    void Camera::SetZoom(float z) {
        zoom = glm::max(0.1f, z);  // Prevent negative/zero zoom
        UpdateProjectionMatrix();
    }

    void Camera::SetAspectRatio(float aspect) {
        aspectRatio = aspect;
        UpdateProjectionMatrix();
    }

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

    void Camera::SetPerspective(float fovDegrees, float aspect, float near, float far) {
        projectionType = ProjectionType::Perspective;
        fov = fovDegrees;
        aspectRatio = aspect;
        nearPlane = near;
        farPlane = far;
        UpdateProjectionMatrix();
    }

    void Camera::Translate(const glm::vec3& offset) {
        position += offset;
        UpdateViewMatrix();
    }

    void Camera::Rotate(const glm::vec3& angles) {
        rotation += angles;
        UpdateViewMatrix();
    }

    void Camera::LookAt(const glm::vec3& target, const glm::vec3& up) {
        viewMatrix = glm::lookAt(position, target, up);

        // Extract rotation from view matrix (optional, for consistency)
        // This is a simplified version - full extraction is more complex
    }

    // === PRIVATE METHODS ===

    void Camera::UpdateViewMatrix() {
        // Create view matrix from position and rotation
        viewMatrix = glm::mat4(1.0f);

        // Apply rotation (in reverse order: Z, Y, X)
        viewMatrix = glm::rotate(viewMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        viewMatrix = glm::rotate(viewMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        viewMatrix = glm::rotate(viewMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));

        // Apply translation
        viewMatrix = glm::translate(viewMatrix, -position);
    }

    void Camera::UpdateProjectionMatrix() {
        if (projectionType == ProjectionType::Orthographic) {
            // Apply zoom to orthographic bounds
            float zoomLeft = orthoLeft / zoom;
            float zoomRight = orthoRight / zoom;
            float zoomBottom = orthoBottom / zoom;
            float zoomTop = orthoTop / zoom;

            projectionMatrix = glm::ortho(zoomLeft, zoomRight, zoomBottom, zoomTop,
                nearPlane, farPlane);
        }
        else {
            projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio,
                nearPlane, farPlane);
        }
    }

    // === CAMERA FACTORY ===

    Camera CameraFactory::Create2D(float width, float height) {
        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;
        return Camera(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
    }

    Camera CameraFactory::Create2DPixelPerfect(int viewportWidth, int viewportHeight) {
        float halfWidth = viewportWidth * 0.5f;
        float halfHeight = viewportHeight * 0.5f;
        return Camera(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
    }

    //Camera CameraFactory::Create3D(float fov, float aspectRatio) {
    //    return Camera(fov, aspectRatio, 0.1f, 100.0f);
    //}

} // namespace Framework