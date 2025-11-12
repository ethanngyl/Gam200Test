/**
===============================================================================
 File:           Material.h
 Author:         Graphics System Overhaul
 Date:           2025-10-07
 ------------------------------------------------------------------------------
 Brief:
 Material system that encapsulates shader, textures, and rendering properties.
 Provides a high-level abstraction for defining how objects should be rendered.

 Design notes:
 - Combines shader + textures + rendering parameters
 - Supports multiple textures per material
 - Allows custom shader parameters
 - Immutable after creation (use MaterialInstance for variations)
===============================================================================
*/
#pragma once
#include "ResourceHandle.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <variant>

namespace Framework {

    /**
     * @brief Rendering blend modes
     */
    enum class BlendMode {
        Opaque,      // No blending
        AlphaBlend,  // Standard alpha blending
        Additive,    // Additive blending
        Multiply     // Multiplicative blending
    };

    /**
     * @brief Material parameter variant
     * Can store different types of shader parameters
     */
    using MaterialParameter = std::variant<
        float,
        glm::vec2,
        glm::vec3,
        glm::vec4,
        int,
        TextureHandle
    >;

    /**
     * @brief Material definition
     * Defines how an object should be rendered
     */
    struct Material {
        std::string name;
        ShaderHandle shader;
        
        // Texture slots
        TextureHandle albedoTexture;
        TextureHandle normalTexture;
        TextureHandle specularTexture;
        
        // Rendering properties
        BlendMode blendMode = BlendMode::Opaque;
        bool depthTest = true;
        bool depthWrite = true;
        bool cullBackFace = true;
        
        // Color tint
        glm::vec4 tint = glm::vec4(1.0f);

        // Sprite sheet UV bounds
        float u0 = 0.0f;
        float v0 = 0.0f;
        float u1 = 1.0f;
        float v1 = 1.0f;
        
        // Custom parameters
        std::unordered_map<std::string, MaterialParameter> parameters;

        Material() = default;
        explicit Material(const std::string& name) : name(name) {}
    };

    /**
     * @brief Material instance with per-object overrides
     * Allows variations without duplicating the base material
     */
    struct MaterialInstance {
        MaterialHandle baseMaterial;
        
        // Override tint
        glm::vec4 tintOverride = glm::vec4(1.0f);
        bool useTintOverride = false;
        
        // Override parameters
        std::unordered_map<std::string, MaterialParameter> parameterOverrides;

        MaterialInstance() = default;
        explicit MaterialInstance(MaterialHandle base) : baseMaterial(base) {}
    };

} // namespace Framework
