/**
===============================================================================
 File:           ECSComponent.h
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Contribution:   100%
 ------------------------------------------------------------------------------
 [A brief, one-sentence description of the file's primary purpose.]
 
  Design notes:
  Provides the foundational component base classes and type identification
 * system for the Entity Component System. Components are pure data containers
 * that systems operate on.
===============================================================================
 */

#pragma once
#include <cstdint>

namespace Framework
{
    using ComponentTypeID = uint32_t;
    /**
     * @class ComponentBase
     * @brief Abstract base class for all components
     *
     * Provides a common polymorphic base for type-erased component storage.
     * All specific component types must inherit from Component<T> which
     * derives from this base.
     */
    class ComponentBase
    {
    public:
        virtual ~ComponentBase() = default;
    };

    /**
     * @class Component
     * @brief CRTP base class for typed components
     * @tparam T The derived component type
     *
     * Provides automatic type ID generation for each component type.
     * Uses the Curiously Recurring Template Pattern (CRTP) to give
     * each component type a unique, stable ID at compile time.
     *
     * @note Type IDs are assigned sequentially starting from 1
     * @note Type IDs are stable within a single program execution
     *
     * Example usage:
     * @code
     * struct Transform : public Component<Transform> {
     *     Vector2D position;
     * };
     *
     * // Each type gets a unique ID:
     * ComponentTypeID id = Transform::GetTypeID();
     * @endcode
     */
    template<typename T>
    class Component : public ComponentBase
    {
    public:
        /**
         * @brief Gets the unique type ID for this component type
         * @return Unique component type identifier
         *
         * Returns the same ID for all instances of type T.
         * Thread-safe due to static initialization guarantees.
         */
        static ComponentTypeID GetTypeID()
        {
            static ComponentTypeID typeID = GetNextTypeID();
            return typeID;
        }

    private:
        /**
         * @brief Generates the next available component type ID
         * @return New unique component type ID
         *
         * Called once per component type during static initialization.
         * IDs start at 1 and increment sequentially.
         */
        static ComponentTypeID GetNextTypeID()
        {
            static ComponentTypeID nextID = 0;
            return ++nextID;
        }
    };
}