/**
===============================================================================
 File:           SkillSystem.h
 Author:
 Date:           2025-01-12
 ------------------------------------------------------------------------------

 SKILL SYSTEM - Manages player skill progression

 PUBLIC API:
 - GiveStartingSkills()  : Give player their class's first 4 skills
 - UnlockNextSkill()     : Unlock the next skill (call on level complete)
 - ReplaceSkillByID()    : Swap an equipped skill with another unlocked skill
 - SwapSkillSlots()      : Swap positions of two equipped skills

 USAGE:
 1. Call SetInputSystem() and SetEntityManager() during engine init
 2. Player selects class -> call GiveStartingSkills()
 3. Player completes level -> call UnlockNextSkill()
 4. Player wants to change loadout -> call ReplaceSkillByID()

===============================================================================
*/

#pragma once

#include "SkillComponent.h"
#include "Interface.h"

namespace Framework {

    class InputSystem;
    class EntityManager;

    class SkillSystem : public EngineSystem {
    public:
        SkillSystem() = default;
        ~SkillSystem() = default;

        // === ENGINE INTERFACE ===
        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* msg) override;

        // === DEPENDENCIES ===
        void SetEntityManager(EntityManager* em) { entityManager = em; }
        void SetInputSystem(InputSystem* input) { inputSystem = input; }

        // === ACCESSORS ===
        SkillComponent* GetPlayerSkills() { return &playerSkills; }
        bool HasSelectedClass() const { return hasSelectedClass; }

        // =================================================================
        // PUBLIC API - Use these functions in your game code
        // =================================================================

        /**
         * @brief Give player their starting skills (first 4 with unlockOrder 0-3)
         * @param skillComp The player's skill component
         * @param charClass The class to initialize
         */
        static void GiveStartingSkills(SkillComponent& skillComp, CharacterClass charClass);

        /**
         * @brief Unlock the next skill in progression order
         * @param skillComp The player's skill component
         * @return true if a skill was unlocked, false if all skills already unlocked
         */
        static bool UnlockNextSkill(SkillComponent& skillComp);

        /**
         * @brief Replace an equipped slot with a different unlocked skill
         * @param skillComp The player's skill component
         * @param slotIndex Which slot to replace (0-3)
         * @param skillID The skill ID to equip
         * @return true if successful
         */
        static bool ReplaceSkillByID(SkillComponent& skillComp, int slotIndex, int skillID);

        /**
         * @brief Swap two equipped skill slots
         * @param skillComp The player's skill component
         * @param fromSlot First slot (0-3)
         * @param toSlot Second slot (0-3)
         */
        static void SwapSkillSlots(SkillComponent& skillComp, int fromSlot, int toSlot);

    private:
        EntityManager* entityManager = nullptr;
        InputSystem* inputSystem = nullptr;

        SkillComponent playerSkills;
        bool hasSelectedClass = false;

        // Test hotkey helpers
        void SelectClass(CharacterClass charClass);
        void ReplaceSlotWithNewest(int slotIndex);
    };

} // namespace Framework