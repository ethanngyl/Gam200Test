/**
===============================================================================
 File:           DamageIndicatorSystem.h
 Author:         Josh Ong
 Email:          josh.o@digipen.edu 
 Date:           2025-11-05
 Contribution:   100%
 ------------------------------------------------------------------------------

  Brief:
  - Defines the DamageIndicatorSystem, which is an event-only system intended to
    provide visual feedback for combat events.

  Key features:
  - **IMessageHandler**: Inherits from IMessageHandler, indicating it only acts
    as a listener and does not require a traditional per-frame Update(dt).
  - **Decoupled Logic**: It is entirely decoupled from the system that generates
    damage (e.g., ProjectileSystem). This separation is the primary design goal.
  - **Event Hooks**: Declares the HandleMessage overrides for:
    1. EnemyDamagedMessage
    2. EnemyDeathMessage 
===============================================================================
 */

#pragma once
#include "Precompiled.h"
#include "Interface.h"      // For EngineSystem base class
#include "Event.h"    // For framework::IMessageHandler and message types

namespace Framework
{
    /**
     * @class DamageIndicatorSystem
     * @brief An event-driven system to handle damage and death events.
     * * This system subscribes to events and reacts by displaying damage numbers,
     * particle effects, or updating scores. It does not run a per-frame Update(dt).
     */
    class DamageIndicatorSystem : public IMessageHandler
    {
    public:
        DamageIndicatorSystem() = default;
        ~DamageIndicatorSystem() = default;

        /**
         * @brief Handles the EnemyDamagedMessage event.
         * (This is where you'd typically spawn a floating damage number entity).
         */
        void HandleMessage(const EnemyDamagedMessage& msg) override;

        /**
         * @brief Handles the EnemyDeathMessage event.
         * (This is where you'd typically update the score, play a sound, or spawn loot).
         */
        void HandleMessage(const EnemyDeathMessage& msg) override;

        // Note: This class only handles messages, so it does not need to inherit
        // from EngineSystem unless it also required an Update(dt) loop.
    };
}