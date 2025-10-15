/**
===============================================================================
 File:           ResourceHandle.h
 Author:         Graphics System Overhaul
 Date:           2025-10-07
 ------------------------------------------------------------------------------
 Brief:
 Type-safe resource handle system for managing graphics resources without
 exposing raw pointers. Provides compile-time type safety and prevents
 resource handle misuse.

 Design notes:
 - Uses template-based type tagging to prevent mixing handle types
 - Invalid handle is represented by ID 0
 - Lightweight (just a uint32_t wrapper)
 - Can be easily serialized/stored
===============================================================================
*/
#pragma once
#include <cstdint>
#include <functional>

namespace Framework {

    /**
     * @brief Type-safe handle for graphics resources
     * @tparam Tag Unique type to differentiate handle types
     * 
     * Usage:
     * struct ShaderTag {};
     * using ShaderHandle = ResourceHandle<ShaderTag>;
     */
    template<typename Tag>
    class ResourceHandle {
    public:
        using HandleType = uint32_t;
        static constexpr HandleType INVALID_ID = 0;

        // Default constructor creates invalid handle
        constexpr ResourceHandle() : id(INVALID_ID) {}
        
        // Explicit construction from ID
        explicit constexpr ResourceHandle(HandleType id) : id(id) {}

        // Check if handle is valid
        constexpr bool IsValid() const { return id != INVALID_ID; }
        
        // Get raw ID (for internal use)
        constexpr HandleType GetID() const { return id; }

        // Comparison operators
        constexpr bool operator==(const ResourceHandle& other) const { return id == other.id; }
        constexpr bool operator!=(const ResourceHandle& other) const { return id != other.id; }
        constexpr bool operator<(const ResourceHandle& other) const { return id < other.id; }

    private:
        HandleType id;
    };

    // Define handle types for different resource types
    struct ShaderTag {};
    struct TextureTag {};
    struct MaterialTag {};
    struct MeshTag {};

    using ShaderHandle = ResourceHandle<ShaderTag>;
    using TextureHandle = ResourceHandle<TextureTag>;
    using MaterialHandle = ResourceHandle<MaterialTag>;
    using MeshHandle = ResourceHandle<MeshTag>;

    // Invalid handle constants
    inline constexpr ShaderHandle INVALID_SHADER_HANDLE{};
    inline constexpr TextureHandle INVALID_TEXTURE_HANDLE{};
    inline constexpr MaterialHandle INVALID_MATERIAL_HANDLE{};
    inline constexpr MeshHandle INVALID_MESH_HANDLE{};

} // namespace Framework

// Hash support for using handles in unordered containers
namespace std {
    template<typename Tag>
    struct hash<Framework::ResourceHandle<Tag>> {
        size_t operator()(const Framework::ResourceHandle<Tag>& handle) const {
            return hash<typename Framework::ResourceHandle<Tag>::HandleType>()(handle.GetID());
        }
    };
}
