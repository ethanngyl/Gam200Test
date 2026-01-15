/*
===============================================================================
File:        ResourceManager.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 100% (remaining)
-------------------------------------------------------------------------------
Brief:
Centralized loader/cache for Shaders, Textures, Meshes, and Materials.
Prevents duplicate loads via hash caches and reference counts. Includes a
directory scanner (assets/) that auto-loads textures and pairs vertex/fragment
shaders by matching file stem names (e.g., foo.vert + foo.frag).

Key Points:
- Thread-safe via a single mutex guarding all resource maps/caches.
- Ref-counting for release semantics; resources are freed when count hits 0.
- Cache keys:
  � Shaders: combined vert+frag path key
  � Textures: normalized path string
  � Meshes: logical name key (e.g., "quad")
- Utility stats and a master Clear() for shutdown.

Safety:
- Early-outs on missing folders/files and on failed GL/resource creations.
- Uniform logging for load/release paths.
===============================================================================
*/
#include "Precompiled.h"
#include "ResourceManager.h"
#include <iostream>
#include <filesystem>

namespace Framework {

    // =========================================================================
    // Bulk scan under ./assets to auto-load textures and shader pairs
    // =========================================================================
    void ResourceManager::LoadFiles()
    {
        namespace fs = std::filesystem;

        const fs::path root = "./assets";
        if (!fs::exists(root)) {
            std::cerr << "ResourceManager::LoadFiles: assets folder not found: " << root << "\n";
            return;
        }

        // 1) Collect files by extension
        std::unordered_set<std::string> textureExts = { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };

        // For shader pairing by stem (e.g., foo.vert + foo.frag)
        std::unordered_map<std::string, fs::path> vertByStem;
        std::unordered_map<std::string, fs::path> fragByStem;

        // Normalize helper: return a portable relative string key for caches & logs
        auto normalize = [](fs::path p) -> std::string {
            p = fs::weakly_canonical(p);
            // make it relative to the working dir if possible
            std::error_code ec;
            fs::path rel = fs::relative(p, fs::current_path(), ec);
            return (ec ? p : rel).generic_string(); // forward slashes
            };

        // 2) First pass: index files
        for (const auto& entry : fs::recursive_directory_iterator(root)) {
            if (!entry.is_regular_file()) continue;

            const fs::path path = entry.path();
            const std::string ext = path.extension().string();
            const std::string stem = path.stem().string();

            // Collect shaders for pairing
            if (ext == ".vert" || ext == ".vs" || ext == ".vsh" || ext == ".glslv") {
                vertByStem[stem] = path;
                continue;
            }
            if (ext == ".frag" || ext == ".fs" || ext == ".fsh" || ext == ".glslf") {
                fragByStem[stem] = path;
                continue;
            }

            // Queue textures by extension
            if (textureExts.count(ext)) {
                const std::string key = normalize(path);
                // 3) Load textures immediately (cache will dedupe)
                TextureHandle th = LoadTexture(key);
                if (!th.IsValid()) {
                    std::cerr << "ResourceManager::LoadFiles: failed to load texture: " << key << "\n";
                }
                continue;
            }

        }

        // 3) Pair & load shaders by stem (only load pairs that exist)
        for (const auto& [stem, vpath] : vertByStem) {
            auto fit = fragByStem.find(stem);
            if (fit == fragByStem.end()) continue; // no matching fragment shader; skip

            const std::string vkey = normalize(vpath);
            const std::string fkey = normalize(fit->second);

            // Use the stem as a friendly shader "name" in logs/caches
            ShaderHandle sh = LoadShader(vkey, fkey, stem);
            if (!sh.IsValid()) {
                std::cerr << "ResourceManager::LoadFiles: failed to load shader pair: "
                    << vkey << " + " << fkey << " (name=" << stem << ")\n";
            }
        }

        // 4) Summary
        auto stats = GetStats();
        std::cout << "ResourceManager::LoadFiles: scanned '" << root.generic_string() << "'\n"
            << "  Shaders:  " << stats.shaderCount << "\n"
            << "  Textures: " << stats.textureCount << "\n"
            << "  Meshes:   " << stats.meshCount << "\n"
            << "  Materials:" << stats.materialCount << "\n";
    }

   // =========================================================================
   // Shader Management
   // =========================================================================

   // Load (or fetch from cache) a shader program built from vert+frag files
    ShaderHandle ResourceManager::LoadShader(const std::string& vertPath,
                                             const std::string& fragPath,
                                             const std::string& name) {
        std::lock_guard<std::mutex> lock(resourceMutex);

        // Cache key combines both file paths; prevents duplicate programs
        std::string cacheKey = MakeShaderKey(vertPath, fragPath);
        auto cacheIt = shaderCache.find(cacheKey);
        if (cacheIt != shaderCache.end()) {
            // Increment reference count
            shaders[cacheIt->second].refCount++;
            return cacheIt->second;
        }

        // Slow path: create program object
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

    // Raw pointer access (non-owning) for a shader by handle
    Shader* ResourceManager::GetShader(ShaderHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = shaders.find(handle);
        return (it != shaders.end()) ? it->second.resource.get() : nullptr;
    }

    // Decrement ref-count and free shader when it reaches zero
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

    // =========================================================================
    // Texture Management
    // =========================================================================

    // Load (or get) a texture by path. Logical names without an extension are ignored.
    TextureHandle ResourceManager::LoadTexture(const std::string& path) {
        std::lock_guard<std::mutex> lock(resourceMutex);

        // Skip non-file identifiers (e.g., sprite names)
        if (path.find('.') == std::string::npos) {
            // e.g., "wireframequad", "circle", etc.
            return INVALID_TEXTURE_HANDLE;
        }

        // Cache hit
        auto cacheIt = textureCache.find(path);
        if (cacheIt != textureCache.end()) {
            textures[cacheIt->second].refCount++;
            return cacheIt->second;
        }

        // Load new GL texture
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

    // Create a texture resource placeholder (for future CreateFromMemory)
    TextureHandle ResourceManager::CreateTexture(const std::string& name,
                                                 int width, int height,
                                                 int channels,
                                                 const unsigned char* data) {
        (void)data, channels, height, width; // silence unused variable warning

        std::lock_guard<std::mutex> lock(resourceMutex);

        // Create texture (implementation would need to be added to Texture class)
        auto texture = std::make_unique<Texture>();
        
        TextureHandle handle(nextTextureID++);
        ResourceEntry<Texture> entry(std::move(texture), name);
        textures[handle] = std::move(entry);

        std::cout << "ResourceManager: Created texture '" << name << "'\n";
        return handle;
    }

    // Raw pointer access (non-owning) for a texture
    Texture* ResourceManager::GetTexture(TextureHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = textures.find(handle);
        return (it != textures.end()) ? it->second.resource.get() : nullptr;
    }

    // Decrement ref-count and free texture when it reaches zero
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

    // =========================================================================
    // Mesh Management
    // =========================================================================

    // Create (or fetch) a mesh by logical name. Non-indexed or indexed path supported.
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

        // Build VAO/VBO/EBO and attribute layout
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

    // Raw pointer access (non-owning) for a mesh
    Mesh* ResourceManager::GetMesh(MeshHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = meshes.find(handle);
        return (it != meshes.end()) ? it->second.resource.get() : nullptr;
    }

    // Decrement ref-count and free mesh when it reaches zero
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

    // =========================================================================
    // Material Management
    // =========================================================================

    // Create a material object that references a shader
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

    // Raw pointer access (non-owning) for a material
    Material* ResourceManager::GetMaterial(MaterialHandle handle) {
        std::lock_guard<std::mutex> lock(resourceMutex);
        auto it = materials.find(handle);
        return (it != materials.end()) ? it->second.resource.get() : nullptr;
    }

    // Decrement ref-count and free material when it reaches zero
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

    // =========================================================================
    // Utility / Stats
    // =========================================================================

    // Drop all resource maps and caches (used on shutdown)
    void ResourceManager::Clear() {
        std::lock_guard<std::mutex> lock(resourceMutex);

        // Force deallocation using swap trick to prevent memory leaks
        // Don't call clear() first - swap directly with empty maps
        std::unordered_map<MaterialHandle, ResourceEntry<Material>>().swap(materials);
        std::unordered_map<MeshHandle, ResourceEntry<Mesh>>().swap(meshes);
        std::unordered_map<TextureHandle, ResourceEntry<Texture>>().swap(textures);
        std::unordered_map<ShaderHandle, ResourceEntry<Shader>>().swap(shaders);

        std::unordered_map<std::string, ShaderHandle>().swap(shaderCache);
        std::unordered_map<std::string, TextureHandle>().swap(textureCache);
        std::unordered_map<std::string, MeshHandle>().swap(meshCache);

        std::cout << "ResourceManager: Cleared all resources\n";
    }

    // Snapshot of current resource counts
    ResourceManager::Stats ResourceManager::GetStats() const {
        std::lock_guard<std::mutex> lock(resourceMutex);
        Stats stats;
        stats.shaderCount = shaders.size();
        stats.textureCount = textures.size();
        stats.meshCount = meshes.size();
        stats.materialCount = materials.size();
        return stats;
    }

    // Check if a texture handle is valid and present
    bool ResourceManager::HasTexture(TextureHandle h) const {
        std::lock_guard<std::mutex> lock(resourceMutex);
        return textures.find(h) != textures.end() && textures.at(h).resource != nullptr;
    }

    // ============================================================================
    // Author:        Tan Wei Leong
    // Email:         weileong.tan@digipen.edu
    // Date:          2025-11-06
    // Contribution:  100%
    // -----------------------------------------------------------------------------
    // Function: EnsureTexture
    // Description:
    //   Loads or retrieves a texture handle by path from the ResourceManager cache.
    //   This function provides a simplified interface for texture access where the
    //   caller only needs to specify the file path. Internally, it reuses the
    //   caching logic in LoadTexture() to prevent duplicate loads.
    //
    //   Key Notes:
    //     - Thread-safe through ResourceManager's internal locking
    //     - Guarantees a valid texture handle if the texture exists
    //     - Commonly used by GraphicsSystemV2 and EntitySpawner
    //
    //   Example Usage:
    //     TextureHandle tex = resourceManager.EnsureTexture("assets/player.png");
    //
    //   Related:
    //     - ResourceManager::LoadTexture()
    //     - GraphicsSystemV2::GetTextureForSpriteName()
    // ============================================================================
    TextureHandle ResourceManager::EnsureTexture(const std::string& path)
    {
        // LoadTexture already dedupes via cache; this just makes intent explicit.
        return LoadTexture(path);
    }

    bool ResourceManager::PathKnownAsTexture(const std::string& path) const {
        std::lock_guard<std::mutex> lock(resourceMutex);
        return textureCache.find(path) != textureCache.end();
    }

    // ============================================================================
    // Author:        Tan Wei Leong
    // Email:         weileong.tan@digipen.edu
    // Date:          2025-11-06
    // Contribution:  100%
    // -----------------------------------------------------------------------------
    // Function: GetMeshHandle
    // Description:
    //   Retrieves a mesh handle by name or file path. The function first checks
    //   the mesh cache for a quick lookup, then falls back to a full search across
    //   registered meshes if not found. Ensures consistent access to mesh resources
    //   without reloading them from disk.
    //
    //   Key Notes:
    //     - Thread-safe lookup using std::mutex
    //     - Returns INVALID_MESH_HANDLE if not found
    //     - Used by GraphicsSystemV2 for assigning default meshes
    //
    //   Example Usage:
    //     MeshHandle quadMesh = resourceManager.GetMeshHandle("quad");
    // ============================================================================
    MeshHandle ResourceManager::GetMeshHandle(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(resourceMutex);

        // Check cache first (fastest)
        auto it = meshCache.find(name);
        if (it != meshCache.end())
            return it->second;

        // Search through all registered meshes by their path/name
        for (const auto& [handle, entry] : meshes)
        {
            if (entry.path == name)
                return handle;
        }

        std::cerr << "WARNING: Mesh '" << name << "' not found\n";
        return INVALID_MESH_HANDLE;
    }


   // ============================================================================
   // Author:        Tan Wei Leong
   // Email:         weileong.tan@digipen.edu
   // Date:          2025-11-06
   // Contribution:  100%
   // -----------------------------------------------------------------------------
   // Function: GetMaterialHandle
   // Description:
   //   Retrieves a material handle based on its assigned name. Iterates through
   //   all loaded materials and returns the first match. This function enables
   //   dynamic material retrieval for entities and systems that assign materials
   //   at runtime.
   //
   //   Key Notes:
   //     - Thread-safe through scoped mutex lock
   //     - Returns INVALID_MATERIAL_HANDLE if material not found
   //     - Commonly used by EntitySpawner and GraphicsSystemV2
   //
   //   Example Usage:
   //     MaterialHandle defaultMat = resourceManager.GetMaterialHandle("quad_mat");
   // ============================================================================
    MaterialHandle ResourceManager::GetMaterialHandle(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(resourceMutex);

        // Search through all materials by their assigned name field
        for (const auto& [handle, entry] : materials)
        {
            if (entry.resource && entry.resource->name == name)
                return handle;
        }

        std::cerr << "WARNING: Material '" << name << "' not found\n";
        return INVALID_MATERIAL_HANDLE;
    }

} // namespace Framework