/**
===============================================================================
 File:           DamageIndicatorSystem.cpp
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-11-05
 Contribution:   100%
 ------------------------------------------------------------------------------

  Brief:
  - Implements the event handling logic for the DamageIndicatorSystem.
  - Currently includes diagnostic console output to confirm that messages
    (EnemyDamagedMessage and EnemyDeathMessage) are being correctly received
    and processed by the system.

  Key implementations:
  - **HandleMessage(EnemyDamagedMessage)**: Logs damage and remaining health.
  - **HandleMessage(EnemyDeathMessage)**: Logs enemy elimination.
===============================================================================
 */

#include "Precompiled.h"
#include "DamageIndicatorSystem.h" // Include the new header
#include "Event.h"
#include <iostream>

namespace Framework
{
    // ========================================================================
    // DAMAGE INDICATOR SYSTEM (Event Listener Implementation)
    // ========================================================================

    void DamageIndicatorSystem::HandleMessage(const EnemyDamagedMessage& msg)
    {
        (void)msg;

        // LOGIC: This proves the decoupling works! The event fired in ProjectileSystem
        // is safely handled here.
        // PERFORMANCE FIX: Simplified multi-line output (was causing 5-10ms frame spikes)
        // std::cout << "Enemy " << msg.enemyEntity.GetID() << " hit: -" << msg.damage
        //           << " HP (" << msg.health << " remaining)\n";
    }

    void DamageIndicatorSystem::HandleMessage(const EnemyDeathMessage& msg)
    {
        (void)msg;

        // LOGIC: Handles the death event after the entity's health hit zero.
        // PERFORMANCE FIX: Simplified multi-line output
        // std::cout << "Enemy " << msg.enemyEntity.GetID() << " eliminated by Player "
        //           << msg.playerEntity.GetID() << "\n";
    }
}