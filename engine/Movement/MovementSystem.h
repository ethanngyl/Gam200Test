/**
 ===============================================================================
 File:           MovementSystem.h
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-09-24
 Contribution:   100%
 ------------------------------------------------------------------------------
 Header file for the MovementSystem class.
 
  Design notes:
  This file declares the MovementSystem, which follows an Entity-Component-System
  (ECS) design. Its sole purpose is to move entities that have both a Transform
  and a Movement component.
 
  It depends on two other systems: the EntityManager (to get entity data) and
  the InputSystem (to read keyboard input). These dependencies are provided
  externally via setter methods to keep the system decoupled. The Update()
  method takes a delta time (`dt`) parameter to ensure movement is smooth and
  independent of the frame rate.
===============================================================================
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
    class MovementSystem : public EngineSystem
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