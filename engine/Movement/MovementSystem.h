/**
 * @file MovementSystem.h
 * @author ETHAN NG YONG LE (n.ethanyongle@digipen.edu)
 * @brief Movement system for keyboard-controlled entity motion
 * @date 2025-09-30
 *
 * Processes WASD input and updates Transform positions for entities
 * with Movement components. Provides frame-rate independent motion
 * using delta time.
 */
#pragma once
#include "Interface.h"
#include "ECSEntityManager.h"

namespace Framework
{
    class InputSystem;//Forward declaration
    /**
     * @class MovementSystem
     * @brief Handles entity movement based on keyboard input
     *
     * Updates the Transform component of entities that have Movement components
     * by processing WASD keyboard input from the InputSystem. Movement is
     * frame-rate independent using delta time.
     *
     * Dependencies:
     * - Requires EntityManager to query and modify entity components
     * - Requires InputSystem to read keyboard state
     *
     * @note Both SetEntityManager() and SetInputSystem() must be called
     *       before Update() will function properly
     */
    class MovementSystem : public InterfaceSystem
    {
    public:
        /**
         * @brief Constructs the MovementSystem
         */
        MovementSystem();

        /**
         * @brief Destructor
         */
        virtual ~MovementSystem();

        /**
         * @brief Initializes the movement system
         */
        virtual void Initialize() override;

        /**
         * @brief Updates entity positions based on input
         * @param dt Delta time since last frame in seconds
         *
         * Processes movement for all entities with Transform and Movement components.
         * Reads WASD input, normalizes diagonal movement, and clamps to screen bounds.
         */
        virtual void Update(float dt) override;

        /**
         * @brief Handles engine messages
         * @param message Message to process
         */
        virtual void SendEngineMessage(Message* message) override;

        /**
         * @brief Sets the input system for reading keyboard state
         * @param input Pointer to the InputSystem instance
         */
        void SetInputSystem(InputSystem* input) { inputSystem = input; }

        /**
         * @brief Sets the entity manager for accessing entity components
         * @param em Pointer to the EntityManager instance
         */
        void SetEntityManager(EntityManager* em) { entityManager = em; }

    private:
        EntityManager* entityManager; //Entity component database
        InputSystem* inputSystem; // Keyboard input provider
    };
}