/**
===============================================================================
 File:           ResourceManager.cpp
 Author:         Graphics System Overhaul
 Date:           2025-10-07
 ------------------------------------------------------------------------------
 Brief:
 Implementation of the ResourceManager class for centralized resource
 lifecycle management.
===============================================================================
*/
#include "Precompiled.h"
#include "ResourceManager.h"
#include <iostream>

namespace Framework {

    // === SHADER MANAGEMENT ===

    ShaderHandle ResourceManager::LoadShader(const std::string& vertPath,
                                             const std::string& fragPath,
                                             const std::string& name) {
        std::lock_guard<std::mutex> lock(resourceMutex);

        // Check cache first
        std::string cacheKey = MakeShaderKey(vertPath, fragPath);
        auto cacheIt = shaderCache.find(cacheKey);
        if (cacheIt != shaderCache.end()) {
            // Increment reference count
            shaders[cacheIt->second].refCount++;
            return cacheIt->second;
        }

        // Load new shader
        try {
            auto shader = std::make_unique<Shader>(vertPath, fragPath);
            ShaderHandle handle(nextShaderID++);

            ResourceEntry<Shader> entry(std::move(shader), cacheKey);
            shaders[handle] = std::move(entry);
            shaderCache[cacheKey] = handle;

            std::cout << "ResourceManager: Loaded shader '" 
                      << (name.empty() ? cacheKey : name) << "'\n";
            return handle;
        }
        catch (const std::exception& e) {
            std::cerr << "ResourceManager: Failed to load shader: " << e.what() << "\n";
            return INVALID_SHADER_HANDLE;
        }
    }

    Shader* ResourceManager::GetShader(ShaderHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = shaders.find(handle);
        return (it != shaders.end()) ? it->second.resource.get() : nullptr;
    }

    void ResourceManager::ReleaseShader(ShaderHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = shaders.find(handle);
        if (it != shaders.end()) {
            if (--it->second.refCount == 0) {
                // Remove from cache
                shaderCache.erase(it->second.path);
                shaders.erase(it);
                std::cout << "ResourceManager: Released shader\n";
            }
        }
    }

    // === TEXTURE MANAGEMENT ===

    TextureHandle ResourceManager::LoadTexture(const std::string& path) {
        std::lock_guard<std::mutex> lock(resourceMutex);

        // Check cache
        auto cacheIt = textureCache.find(path);
        if (cacheIt != textureCache.end()) {
            textures[cacheIt->second].refCount++;
            return cacheIt->second;
        }

        // Load new texture
        auto texture = std::make_unique<Texture>();
        if (!texture->LoadFromFile(path)) {
            std::cerr << "ResourceManager: Failed to load texture: " << path << "\n";
            return INVALID_TEXTURE_HANDLE;
        }

        TextureHandle handle(nextTextureID++);
        ResourceEntry<Texture> entry(std::move(texture), path);
        textures[handle] = std::move(entry);
        textureCache[path] = handle;

        std::cout << "ResourceManager: Loaded texture '" << path << "'\n";
        return handle;
    }

    TextureHandle ResourceManager::CreateTexture(const std::string& name,
                                                 int width, int height,
                                                 int channels,
                                                 const unsigned char* data) {
        (void)data, channels, height, width; // silence unused variable warning

        std::lock_guard<std::mutex> lock(resourceMutex);

        // Create texture (implementation would need to be added to Texture class)
        auto texture = std::make_unique<Texture>();
        // Note: You'd need to add a CreateFromMemory method to Texture class
        
        TextureHandle handle(nextTextureID++);
        ResourceEntry<Texture> entry(std::move(texture), name);
        textures[handle] = std::move(entry);

        std::cout << "ResourceManager: Created texture '" << name << "'\n";
        return handle;
    }

    Texture* ResourceManager::GetTexture(TextureHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = textures.find(handle);
        return (it != textures.end()) ? it->second.resource.get() : nullptr;
    }

    void ResourceManager::ReleaseTexture(TextureHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = textures.find(handle);
        if (it != textures.end()) {
            if (--it->second.refCount == 0) {
                textureCache.erase(it->second.path);
                textures.erase(it);
                std::cout << "ResourceManager: Released texture\n";
            }
        }
    }

    // === MESH MANAGEMENT ===

    MeshHandle ResourceManager::CreateMesh(const std::string& name,
                                          const std::vector<float>& vertices,
                                          const std::vector<unsigned int>& indices,
                                          GLenum drawMode,
                                          bool hasTexCoords) {
        std::lock_guard<std::mutex> lock(resourceMutex);

        // Check if already exists
        auto cacheIt = meshCache.find(name);
        if (cacheIt != meshCache.end()) {
            meshes[cacheIt->second].refCount++;
            return cacheIt->second;
        }

        // Create new mesh
        std::unique_ptr<Mesh> mesh;
        
        if (indices.empty()) {
            // Non-indexed mesh
            mesh = std::make_unique<Mesh>(vertices, drawMode, hasTexCoords);
        } else {
            // Indexed mesh
            mesh = std::make_unique<Mesh>();
            std::vector<int> attribSizes = hasTexCoords ? 
                std::vector<int>{3, 3, 2} : std::vector<int>{3, 3};
            mesh->Initialize(vertices, indices, attribSizes);
        }

        MeshHandle handle(nextMeshID++);
        ResourceEntry<Mesh> entry(std::move(mesh), name);
        meshes[handle] = std::move(entry);
        meshCache[name] = handle;

        std::cout << "ResourceManager: Created mesh '" << name << "'\n";
        return handle;
    }

    Mesh* ResourceManager::GetMesh(MeshHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = meshes.find(handle);
        return (it != meshes.end()) ? it->second.resource.get() : nullptr;
    }

    void ResourceManager::ReleaseMesh(MeshHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = meshes.find(handle);
        if (it != meshes.end()) {
            if (--it->second.refCount == 0) {
                meshCache.erase(it->second.path);
                meshes.erase(it);
                std::cout << "ResourceManager: Released mesh\n";
            }
        }
    }

    // === MATERIAL MANAGEMENT ===

    MaterialHandle ResourceManager::CreateMaterial(const std::string& name, ShaderHandle shader) {
        std::lock_guard<std::mutex> lock(resourceMutex);

        auto material = std::make_unique<Material>(name);
        material->shader = shader;

        MaterialHandle handle(nextMaterialID++);
        ResourceEntry<Material> entry(std::move(material), name);
        materials[handle] = std::move(entry);

        std::cout << "ResourceManager: Created material '" << name << "'\n";
        return handle;
    }

    Material* ResourceManager::GetMaterial(MaterialHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = materials.find(handle);
        return (it != materials.end()) ? it->second.resource.get() : nullptr;
    }

    void ResourceManager::ReleaseMaterial(MaterialHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = materials.find(handle);
        if (it != materials.end()) {
            if (--it->second.refCount == 0) {
                materials.erase(it);
                std::cout << "ResourceManager: Released material\n";
            }
        }
    }

    // === UTILITY ===

    void ResourceManager::Clear() {
        std::lock_guard<std::mutex> lock(resourceMutex);
        
        materials.clear();
        meshes.clear();
        textures.clear();
        shaders.clear();
        
        shaderCache.clear();
        textureCache.clear();
        meshCache.clear();
        
        std::cout << "ResourceManager: Cleared all resources\n";
    }

    ResourceManager::Stats ResourceManager::GetStats() const {
        std::lock_guard<std::mutex> lock(resourceMutex);
        Stats stats;
        stats.shaderCount = shaders.size();
        stats.textureCount = textures.size();
        stats.meshCount = meshes.size();
        stats.materialCount = materials.size();
        return stats;
    }

} // namespace Framework
