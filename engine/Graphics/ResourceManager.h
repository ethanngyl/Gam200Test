/**
===============================================================================
 File:           ResourceManager.h
 Author:         Graphics System Overhaul
 Date:           2025-10-07
 ------------------------------------------------------------------------------
 Brief:
 Centralized resource management system for all graphics resources.
 Handles loading, caching, and lifetime management of shaders, textures,
 meshes, and materials.

 Design notes:
 - Automatic resource caching (no duplicate loads)
 - Reference counting for automatic cleanup
 - Thread-safe resource access
 - Supports hot-reloading (future extension)
===============================================================================
*/
#pragma once
#include "ResourceHandle.h"
#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include "Mesh.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <mutex>

namespace Framework {

    /**
     * @brief Resource entry with reference counting
     */
    template<typename T>
    struct ResourceEntry {
        std::unique_ptr<T> resource;
        uint32_t refCount = 0;
        std::string path;  // For reloading

        ResourceEntry() = default;
        explicit ResourceEntry(std::unique_ptr<T> res, const std::string& p = "")
            : resource(std::move(res)), refCount(1), path(p) {}
    };

    /**
     * @brief Centralized graphics resource manager
     * Manages lifecycle of all graphics resources
     */
    class ResourceManager {
    public:
        ResourceManager() : nextShaderID(1), nextTextureID(1), 
                           nextMaterialID(1), nextMeshID(1) {}
        ~ResourceManager() { Clear(); }

        // Prevent copying
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        // --------------
        void LoadFiles();
        // --------------`

        // === SHADER MANAGEMENT ===
        
        /**
         * @brief Load or get cached shader
         * @param vertPath Vertex shader path
         * @param fragPath Fragment shader path
         * @param name Optional shader name (auto-generated if empty)
         * @return Handle to the shader
         */
        ShaderHandle LoadShader(const std::string& vertPath, 
                               const std::string& fragPath,
                               const std::string& name = "");

        /**
         * @brief Get shader by handle
         */
        Shader* GetShader(ShaderHandle handle);

        /**
         * @brief Release shader reference
         */
        void ReleaseShader(ShaderHandle handle);

        // === TEXTURE MANAGEMENT ===
        
        /**
         * @brief Load or get cached texture
         * @param path Texture file path
         * @return Handle to the texture
         */
        TextureHandle LoadTexture(const std::string& path);

        /**
         * @brief Create texture from memory
         */
        TextureHandle CreateTexture(const std::string& name, 
                                    int width, int height, 
                                    int channels, 
                                    const unsigned char* data);

        /**
         * @brief Get texture by handle
         */
        Texture* GetTexture(TextureHandle handle);

        /**
         * @brief Release texture reference
         */
        void ReleaseTexture(TextureHandle handle);

        // === MESH MANAGEMENT ===
        
        /**
         * @brief Create mesh from vertex data
         */
        MeshHandle CreateMesh(const std::string& name,
                             const std::vector<float>& vertices,
                             const std::vector<unsigned int>& indices = {},
                             GLenum drawMode = GL_TRIANGLES,
                             bool hasTexCoords = true);

        /**
         * @brief Get mesh by handle
         */
        Mesh* GetMesh(MeshHandle handle);

        /**
         * @brief Release mesh reference
         */
        void ReleaseMesh(MeshHandle handle);

        // === MATERIAL MANAGEMENT ===
        
        /**
         * @brief Create material
         */
        MaterialHandle CreateMaterial(const std::string& name, ShaderHandle shader);

        /**
         * @brief Get material by handle
         */
        Material* GetMaterial(MaterialHandle handle);

        /**
         * @brief Release material reference
         */
        void ReleaseMaterial(MaterialHandle handle);

        // === UTILITY ===
        
        /**
         * @brief Clear all resources
         */
        void Clear();

        /**
         * @brief Get resource statistics
         */
        struct Stats {
            size_t shaderCount = 0;
            size_t textureCount = 0;
            size_t meshCount = 0;
            size_t materialCount = 0;
        };
        Stats GetStats() const;

        bool HasTexture(TextureHandle h) const;
        TextureHandle EnsureTexture(const std::string& path);   // Load if missing, return handle
        bool          PathKnownAsTexture(const std::string& path) const;

    private:
        // Resource storage
        std::unordered_map<ShaderHandle, ResourceEntry<Shader>> shaders;
        std::unordered_map<TextureHandle, ResourceEntry<Texture>> textures;
        std::unordered_map<MeshHandle, ResourceEntry<Mesh>> meshes;
        std::unordered_map<MaterialHandle, ResourceEntry<Material>> materials;

        // Path-to-handle caches for deduplication
        std::unordered_map<std::string, ShaderHandle> shaderCache;
        std::unordered_map<std::string, TextureHandle> textureCache;
        std::unordered_map<std::string, MeshHandle> meshCache;

        // ID generators
        uint32_t nextShaderID;
        uint32_t nextTextureID;
        uint32_t nextMaterialID;
        uint32_t nextMeshID;

        // Thread safety
        mutable std::mutex resourceMutex;

        // Helper to generate cache key for shader
        std::string MakeShaderKey(const std::string& vertPath, const std::string& fragPath) const {
            return vertPath + "|" + fragPath;
        }
    };

} // namespace Framework
