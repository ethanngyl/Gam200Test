/**
===============================================================================
 File:           SkillSystem.cpp
 Author:
 Date:           2025-01-12
 ------------------------------------------------------------------------------

 SKILL SYSTEM IMPLEMENTATION

 To add skills: Edit SkillDatabase::InitializeSkills() in SkillComponent.h

===============================================================================
*/

#include "Precompiled.h"
#include "SkillSystem.h"

namespace Framework {

    // ========================================================================
    // ENGINE INTERFACE
    // ========================================================================

    void SkillSystem::Initialize() {
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "SKILL SYSTEM INITIALIZED" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Test Hotkeys:" << std::endl;
        std::cout << "  [Z/X/C]   - Select Class" << std::endl;
        std::cout << "  [V]       - Unlock next skill" << std::endl;
        std::cout << "  [B/N/M/J] - Replace slot 1/2/3/4" << std::endl;
        std::cout << "  [P]       - Print loadout" << std::endl;
        std::cout << "========================================" << std::endl;
    }

    void SkillSystem::Update(float dt) {
        (void)dt;
        if (!inputSystem) return;

        // === TEST HOTKEYS (remove when UI is ready) ===

        if (inputSystem->IsKeyPressed(KEY_Z)) SelectClass(CharacterClass::Swordmaster);
        if (inputSystem->IsKeyPressed(KEY_X)) SelectClass(CharacterClass::Magus);
        if (inputSystem->IsKeyPressed(KEY_C)) SelectClass(CharacterClass::Berserker);

        if (inputSystem->IsKeyPressed(KEY_V)) {
            if (!hasSelectedClass) {
                std::cout << "[SKILL] Select a class first (Z/X/C)" << std::endl;
            }
            else {
                UnlockNextSkill(playerSkills);
            }
        }

        if (inputSystem->IsKeyPressed(KEY_B)) ReplaceSlotWithNewest(0);
        if (inputSystem->IsKeyPressed(KEY_N)) ReplaceSlotWithNewest(1);
        if (inputSystem->IsKeyPressed(KEY_M)) ReplaceSlotWithNewest(2);
        if (inputSystem->IsKeyPressed(KEY_J)) ReplaceSlotWithNewest(3);

        if (inputSystem->IsKeyPressed(KEY_P)) {
            if (!hasSelectedClass) {
                std::cout << "[SKILL] Select a class first (Z/X/C)" << std::endl;
            }
            else {
                playerSkills.PrintLoadout();
            }
        }
    }

    void SkillSystem::SendEngineMessage(Message* msg) {
        (void)msg;
    }

    // ========================================================================
    // PUBLIC API
    // ========================================================================

    void SkillSystem::GiveStartingSkills(SkillComponent& skillComp, CharacterClass charClass) {
        skillComp.playerClass = charClass;

        const auto& allSkills = SkillDatabase::GetInstance().GetClassSkills(charClass);

        std::cout << std::endl;
        std::cout << "Initializing " << SkillDatabase::GetClassName(charClass) << "..." << std::endl;

        // Unlock skills with unlockOrder 0-3 (starting skills)
        for (const auto& skill : allSkills) {
            if (skill.unlockOrder >= 0 && skill.unlockOrder < 4) {
                skillComp.UnlockSkill(skill);
            }
        }

        if (skillComp.unlockedSkills.empty()) {
            std::cout << "[SKILL] WARNING: No starting skills defined for this class!" << std::endl;
            std::cout << "[SKILL] Add skills in SkillDatabase::InitializeSkills()" << std::endl;
        }

        skillComp.PrintLoadout();
    }

    bool SkillSystem::UnlockNextSkill(SkillComponent& skillComp) {
        const auto& allSkills = SkillDatabase::GetInstance().GetClassSkills(skillComp.playerClass);

        // Find next skill that isn't unlocked
        for (const auto& skill : allSkills) {
            if (!skillComp.HasSkill(skill.skillID)) {
                std::cout << std::endl;
                std::cout << ">>> NEW SKILL UNLOCKED! <<<" << std::endl;
                skillComp.UnlockSkill(skill);
                skillComp.PrintLoadout();
                return true;
            }
        }

        std::cout << "[SKILL] All skills already unlocked!" << std::endl;
        return false;
    }

    bool SkillSystem::ReplaceSkillByID(SkillComponent& skillComp, int slotIndex, int skillID) {
        if (slotIndex < 0 || slotIndex >= 4) {
            std::cout << "[SKILL] Invalid slot: " << slotIndex << std::endl;
            return false;
        }

        // Find skill in unlocked list
        int foundIndex = -1;
        for (size_t i = 0; i < skillComp.unlockedSkills.size(); ++i) {
            if (skillComp.unlockedSkills[i].skillID == skillID) {
                foundIndex = static_cast<int>(i);
                break;
            }
        }

        if (foundIndex == -1) {
            std::cout << "[SKILL] Skill ID " << skillID << " not unlocked!" << std::endl;
            return false;
        }

        // Get names for display
        std::string oldName = "[EMPTY]";
        if (const SkillData* old = skillComp.GetEquippedSkill(slotIndex)) {
            oldName = old->skillName;
        }
        const std::string& newName = skillComp.unlockedSkills[foundIndex].skillName;

        skillComp.equippedSlots[slotIndex] = foundIndex;

        std::cout << std::endl;
        std::cout << "Slot " << (slotIndex + 1) << ": " << oldName << " -> " << newName << std::endl;
        skillComp.PrintLoadout();
        return true;
    }

    void SkillSystem::SwapSkillSlots(SkillComponent& skillComp, int fromSlot, int toSlot) {
        if (fromSlot < 0 || fromSlot >= 4 || toSlot < 0 || toSlot >= 4) {
            std::cout << "[SKILL] Invalid slot indices" << std::endl;
            return;
        }

        int temp = skillComp.equippedSlots[fromSlot];
        skillComp.equippedSlots[fromSlot] = skillComp.equippedSlots[toSlot];
        skillComp.equippedSlots[toSlot] = temp;

        std::cout << "Swapped Slot " << (fromSlot + 1) << " <-> Slot " << (toSlot + 1) << std::endl;
        skillComp.PrintLoadout();
    }

    // ========================================================================
    // PRIVATE HELPERS (for test hotkeys)
    // ========================================================================

    void SkillSystem::SelectClass(CharacterClass charClass) {
        if (hasSelectedClass) {
            std::cout << "[SKILL] Class already selected!" << std::endl;
            return;
        }

        std::cout << std::endl;
        std::cout << "########################################" << std::endl;
        std::cout << "# CLASS: " << SkillDatabase::GetClassName(charClass) << std::endl;
        std::cout << "########################################" << std::endl;

        GiveStartingSkills(playerSkills, charClass);
        hasSelectedClass = true;
    }

    void SkillSystem::ReplaceSlotWithNewest(int slotIndex) {
        if (!hasSelectedClass) {
            std::cout << "[SKILL] Select a class first (Z/X/C)" << std::endl;
            return;
        }

        if (playerSkills.unlockedSkills.empty()) {
            std::cout << "[SKILL] No skills to equip!" << std::endl;
            return;
        }

        int newestID = playerSkills.unlockedSkills.back().skillID;
        ReplaceSkillByID(playerSkills, slotIndex, newestID);
    }

} // namespace Framework