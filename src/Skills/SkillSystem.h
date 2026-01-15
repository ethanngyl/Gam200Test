/**
===============================================================================
 File:           SkillSystem.h
 Author:
 Date:           2025-01-12
 ------------------------------------------------------------------------------

 System for managing skill unlocks and skill replacement.

 Key Functions:
 - GiveStartingSkills(): Give player their class's starting 4 skills
 - UnlockNextSkill(): Unlock the next skill in progression (level complete)
 - ReplaceSkill(): Replace an equipped skill with a newly unlocked skill

 HOTKEY TESTING (remove when integrating real UI):
 ----------------------------------------
 [Z] - Select Class: Swordmaster
 [X] - Select Class: Magus
 [C] - Select Class: Berserker

 [V] - Unlock next skill (simulates level complete)

 [B] - Replace Slot 1 with newest unlocked skill
 [N] - Replace Slot 2 with newest unlocked skill
 [M] - Replace Slot 3 with newest unlocked skill
 [J] - Replace Slot 4 with newest unlocked skill

 [P] - Print current skill loadout
 ----------------------------------------
===============================================================================
*/

#pragma once

// DON'T include Precompiled.h here - it causes circular dependency
// Include only what we need
#include "SkillComponent.h"
#include "Interface.h"
#include "Log.h"

namespace Framework {

    // Forward declarations - these are defined elsewhere
    class InputSystem;
    class EntityManager;

    class SkillSystem : public EngineSystem {
    public:
        SkillSystem() = default;
        ~SkillSystem() = default;

        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* msg) override;

        // ====================================================================
        // DEPENDENCY SETTERS
        // ====================================================================
        void SetEntityManager(EntityManager* em) {
            entityManager = em;
        }

        void SetInputSystem(InputSystem* input) {
            inputSystem = input;
        }

        // ====================================================================
        // GET PLAYER SKILLS (for external access)
        // ====================================================================
        SkillComponent* GetPlayerSkills() {
            return &playerSkills;
        }

        // ====================================================================
        // MAIN FUNCTIONS (static for use anywhere)
        // ====================================================================
        static void GiveStartingSkills(SkillComponent& skillComp, CharacterClass charClass);
        static void UnlockNextSkill(SkillComponent& skillComp);
        static bool ReplaceSkill(SkillComponent& skillComp, int slotIndex, int newSkillID);
        static void SwapSkillSlots(SkillComponent& skillComp, int fromSlot, int toSlot);

        // ====================================================================
        // HELPER FUNCTIONS
        // ====================================================================
        static std::vector<SkillData> GetClassSkills(CharacterClass charClass);
        static const char* GetClassName(CharacterClass charClass);

    private:
        EntityManager* entityManager = nullptr;
        InputSystem* inputSystem = nullptr;

        // Player skill data (for testing - later attach to player entity)
        SkillComponent playerSkills;
        bool hasSelectedClass = false;

        // ====================================================================
        // HOTKEY HELPER FUNCTIONS
        // ====================================================================
        void SelectClass(CharacterClass charClass);
        void ReplaceSlotWithNewest(int slotIndex);
    };

} // namespace Framework