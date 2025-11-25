/**
===============================================================================
 File:           Component.cpp
 Author:         Code Assistant
 Date:           2025-11-25
 ------------------------------------------------------------------------------

  Implementation of component destructors for proper resource cleanup.

  This file contains destructors for components that hold resource handles
  (textures, materials, meshes) to ensure proper reference counting and
  memory management.
===============================================================================
 */

#include "Component.h"
#include "Core.h"
#include "GraphicsSystemV2.h"

namespace Framework
{
    // ========================================================================
    // SpriteAnimation Destructor
    // ========================================================================

    SpriteAnimation::~SpriteAnimation() {
        // Release the sprite sheet texture when this component is destroyed
        if (spriteSheet.IsValid() && CORE && CORE->GetGraphicsSystem()) {
            auto* gfx = static_cast<GraphicsSystemV2*>(CORE->GetGraphicsSystem());
            auto& resourceManager = gfx->GetResourceManager();
            resourceManager.ReleaseTexture(spriteSheet);
        }
    }

} // namespace Framework
