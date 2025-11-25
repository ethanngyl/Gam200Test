/*
===============================================================================
File:        RenderComponents.cpp
Author:      Code Assistant
Date:        2025-11-25
-------------------------------------------------------------------------------
Brief:
Implementation of render component destructors for proper resource cleanup.

Ensures that textures, materials, and meshes are properly released when
Renderable/MeshRenderer components are destroyed to prevent memory leaks.
===============================================================================
*/

#include "RenderComponents.h"
#include "Core.h"
#include "GraphicsSystemV2.h"

namespace Framework {

    // ========================================================================
    // Renderable Destructor
    // ========================================================================

    Renderable::~Renderable() {
        // Release resources when this component is destroyed
        if (CORE && CORE->GetGraphicsSystem()) {
            auto* gfx = static_cast<GraphicsSystemV2*>(CORE->GetGraphicsSystem());
            auto& resourceManager = gfx->GetResourceManager();

            // Release texture if valid
            if (texture.IsValid()) {
                resourceManager.ReleaseTexture(texture);
            }

            // Release mesh if valid
            if (mesh.IsValid()) {
                resourceManager.ReleaseMesh(mesh);
            }

            // Release material if valid
            if (material.IsValid()) {
                resourceManager.ReleaseMaterial(material);
            }
        }
    }

} // namespace Framework
