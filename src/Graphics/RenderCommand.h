/*
===============================================================================
File:        RenderCommand.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Lightweight render command types and queues used by GraphicsSystemV2.

- RenderCommand: one logical draw call (mesh + material + texture + xform)
- RenderQueue:   gathers/Sorts/clears commands each frame (front-to-back)
- DebugRenderQueue: lines/circles/boxes for on-screen debug overlays

Notes:
- Sorting packs (layer, material, orderInLayer, depth) into a 64-bit key.
- Depth is quantized to 16 bits assuming a normalized range (0..100 units)
  to keep the key compact and branch-free during sort.
===============================================================================
*/
#pragma once
#include "ResourceHandle.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>

namespace Framework {

    /**
     * @brief Single render command representing one draw call
     */
    struct RenderCommand {
        // What to draw
        MeshHandle mesh;
        MaterialHandle material;
        TextureHandle  texture;
        // Transform
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        
        // Sorting keys
        int layer = 0;
        int orderInLayer = 0;
        float depth = 0.0f;  // Distance from camera for depth sorting
        
        // Per-instance data
        glm::vec4 tint = glm::vec4(1.0f);
        
        // State flags
        bool visible = true;

        // Generate sorting key for efficient sorting
        uint64_t GetSortKey() const {
            // Layer (8 bits) | Material (24 bits) | Order (16 bits) | Depth (16 bits)
            uint64_t key = 0;
            key |= (static_cast<uint64_t>(layer & 0xFF) << 56);
            key |= (static_cast<uint64_t>(material.GetID() & 0xFFFFFF) << 32);
            key |= (static_cast<uint64_t>(orderInLayer & 0xFFFF) << 16);
            
            // Convert depth to uint16 (assuming depth range 0-100)
            uint16_t depthBits = static_cast<uint16_t>(glm::clamp(depth * 655.35f, 0.0f, 65535.0f));
            key |= depthBits;
            
            return key;
        }
    };

    /**
     * @brief Render queue that accumulates and sorts render commands
     */
    class RenderQueue {
    public:
        /**
         * @brief Add a render command to the queue
         */
        void Submit(const RenderCommand& command) {
            if (command.visible) {
                commands.push_back(command);
            }
        }

        /**
         * @brief Sort commands for optimal rendering
         */
        void Sort() {
            std::sort(commands.begin(), commands.end(),
                [](const RenderCommand& a, const RenderCommand& b) {
                    return a.GetSortKey() < b.GetSortKey();
                });
        }

        /**
         * @brief Get all commands
         */
        const std::vector<RenderCommand>& GetCommands() const {
            return commands;
        }

        /**
         * @brief Clear all commands
         */
        void Clear() {
            commands.clear();
        }

        /**
         * @brief Get command count
         */
        size_t GetCount() const {
            return commands.size();
        }

    private:
        std::vector<RenderCommand> commands;
    };

    /**
     * @brief Debug render queue for debug visualizations
     * Rendered on top of everything else
     */
    class DebugRenderQueue {
    public:
        struct DebugLine {
            glm::vec3 start;            // World-space start
            glm::vec3 end;              // World-space end
            glm::vec4 color;            // RGBA (no texture)
        };

        struct DebugCircle {
            glm::vec3 center;           // World-space center (XY plane assumed)
            float radius;               // Circle radius in world units
            glm::vec4 color;            // RGBA
            int segments = 32;          // Tessellation for approximated circle
        };

        struct DebugBox {
            glm::vec3 center;           // World-space center
            glm::vec3 size;             // World-space extents (width, height, depth)
            glm::vec4 color;            // RGBA
        };

        /**
         * @brief Add debug line
         */
         // Add a line primitive
         // Example: // Draw a cyan normal from P to P+N
         // AddLine(P, P + N, {0,1,1,1});
        void AddLine(const glm::vec3& start, const glm::vec3& end, 
                     const glm::vec4& color = glm::vec4(1.0f)) {
            lines.push_back({start, end, color});
        }

        /**
         * @brief Add debug circle
         */
         // Add a circle primitive (useful for collision radii / vision cones)
         // Note: segments controls smoothness vs cost.
        void AddCircle(const glm::vec3& center, float radius,
                      const glm::vec4& color = glm::vec4(1.0f), int segments = 32) {
            circles.push_back({center, radius, color, segments});
        }

        /**
         * @brief Add debug box
         */
        // Add an axis-aligned box (use size={w,h,1} for 2D quads)
        void AddBox(const glm::vec3& center, const glm::vec3& size,
                   const glm::vec4& color = glm::vec4(1.0f)) {
            boxes.push_back({center, size, color});
        }

        /**
         * @brief Get all debug primitives
         */
         // Fast const accessors used by the debug pass
        const std::vector<DebugLine>& GetLines() const { return lines; }
        const std::vector<DebugCircle>& GetCircles() const { return circles; }
        const std::vector<DebugBox>& GetBoxes() const { return boxes; }

        /**
         * @brief Clear all debug primitives
         */
         // Clear all primitives at end-of-frame
        void Clear() {
            lines.clear();
            circles.clear();
            boxes.clear();
        }

    private:
        std::vector<DebugLine> lines;
        std::vector<DebugCircle> circles;
        std::vector<DebugBox> boxes;
    };

} // namespace Framework
