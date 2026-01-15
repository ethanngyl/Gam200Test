/**
===============================================================================
 File:           SkillSystem.cpp
 Author:
 Date:           2025-01-12
 ------------------------------------------------------------------------------
 Implementation of SkillSystem
===============================================================================
*/

#include "Precompiled.h"
#include "SkillSystem.h"

namespace Framework {

    void SkillSystem::Initialize() {
        std::cout << "========================================" << std::endl;
        std::cout << "SKILLSYSTEM INITIALIZED!" << std::endl;
        std::cout << "HOTKEYS:" << std::endl;
        std::cout << "  [Z/X/C] - Select Class (Swordmaster/Magus/Berserker)" << std::endl;
        std::cout << "  [V]     - Unlock next skill" << std::endl;
        std::cout << "  [B/N/M/J] - Replace slot 1/2/3/4 with newest skill" << std::endl;
        std::cout << "  [P]     - Print loadout" << std::endl;
        std::cout << "========================================" << std::endl;

        LOG_INFO("SKILL", "SkillSystem Initialized");
    }

    void SkillSystem::Update(float dt) {
        (void)dt;

        // DEBUG: Check if Update is running
        static bool firstRun = true;
        if (firstRun) {
            std::cout << "[SKILL] Update() called for first time!" << std::endl;
            std::cout << "[SKILL] inputSystem pointer = " << inputSystem << std::endl;
            firstRun = false;
        }

        if (!inputSystem) {
            static bool warnedOnce = false;
            if (!warnedOnce) {
                std::cout << "[SKILL] ERROR: inputSystem is NULL!" << std::endl;
                warnedOnce = true;
            }
            return;
        }

        // ================================================================
        // CLASS SELECTION (Z, X, C)
        // ================================================================
        if (inputSystem->IsKeyPressed(KEY_Z)) {
            std::cout << "[SKILL] Z pressed - Selecting Swordmaster" << std::endl;
            SelectClass(CharacterClass::Swordmaster);
        }
        if (inputSystem->IsKeyPressed(KEY_X)) {
            std::cout << "[SKILL] X pressed - Selecting Magus" << std::endl;
            SelectClass(CharacterClass::Magus);
        }
        if (inputSystem->IsKeyPressed(KEY_C)) {
            std::cout << "[SKILL] C pressed - Selecting Berserker" << std::endl;
            SelectClass(CharacterClass::Berserker);
        }

        // ================================================================
        // UNLOCK NEXT SKILL (V)
        // ================================================================
        if (inputSystem->IsKeyPressed(KEY_V)) {
            std::cout << "[SKILL] V pressed - Unlock next skill" << std::endl;
            if (!hasSelectedClass) {
                std::cout << "[SKILL] Select a class first! Press Z, X, or C." << std::endl;
            }
            else {
                std::cout << "[SKILL] >>> SIMULATING LEVEL COMPLETE <<<" << std::endl;
                UnlockNextSkill(playerSkills);
            }
        }

        // ================================================================
        // REPLACE SKILLS (B, N, M, J)
        // ================================================================
        if (inputSystem->IsKeyPressed(KEY_B)) {
            std::cout << "[SKILL] B pressed - Replace Slot 1" << std::endl;
            ReplaceSlotWithNewest(0);
        }
        if (inputSystem->IsKeyPressed(KEY_N)) {
            std::cout << "[SKILL] N pressed - Replace Slot 2" << std::endl;
            ReplaceSlotWithNewest(1);
        }
        if (inputSystem->IsKeyPressed(KEY_M)) {
            std::cout << "[SKILL] M pressed - Replace Slot 3" << std::endl;
            ReplaceSlotWithNewest(2);
        }
        if (inputSystem->IsKeyPressed(KEY_J)) {
            std::cout << "[SKILL] J pressed - Replace Slot 4" << std::endl;
            ReplaceSlotWithNewest(3);
        }

        // ================================================================
        // PRINT LOADOUT (P)
        // ================================================================
        if (inputSystem->IsKeyPressed(KEY_P)) {
            std::cout << "[SKILL] P pressed - Print loadout" << std::endl;
            if (!hasSelectedClass) {
                std::cout << "[SKILL] Select a class first! Press Z, X, or C." << std::endl;
            }
            else {
                playerSkills.PrintLoadout();
            }
        }
    }

    void SkillSystem::SendEngineMessage(Message* msg) {
        (void)msg;
    }

    // ====================================================================
    // MAIN FUNCTIONS
    // ====================================================================

    void SkillSystem::GiveStartingSkills(SkillComponent& skillComp, CharacterClass charClass) {
        skillComp.playerClass = charClass;

        const char* className = GetClassName(charClass);
        std::cout << "[SKILL] Giving " << className << " starting skills..." << std::endl;

        auto skills = GetClassSkills(charClass);

        // Unlock first 4 skills only
        for (int i = 0; i < 4 && i < static_cast<int>(skills.size()); ++i) {
            skillComp.UnlockSkill(skills[i].skillID, skills[i].skillName, charClass);
        }

        skillComp.PrintLoadout();
    }

    void SkillSystem::UnlockNextSkill(SkillComponent& skillComp) {
        auto allSkills = GetClassSkills(skillComp.playerClass);

        for (const auto& skillData : allSkills) {
            if (!skillComp.HasSkill(skillData.skillID)) {
                std::cout << "========================================" << std::endl;
                std::cout << "LEVEL COMPLETE! New skill unlocked!" << std::endl;
                std::cout << "========================================" << std::endl;

                skillComp.unlockedSkills.push_back(SkillData(skillData.skillID, skillData.skillName, skillData.ownerClass, true));

                std::cout << "Unlocked: " << skillData.skillName << " (ID: " << skillData.skillID << ")" << std::endl;
                std::cout << "Use B/N/M/J to REPLACE an equipped skill!" << std::endl;
                std::cout << "========================================" << std::endl;

                skillComp.PrintLoadout();
                return;
            }
        }

        std::cout << "[SKILL] All skills for this class already unlocked!" << std::endl;
    }

    bool SkillSystem::ReplaceSkill(SkillComponent& skillComp, int slotIndex, int newSkillID) {
        if (slotIndex < 0 || slotIndex >= 4) {
            std::cout << "[SKILL] Invalid slot index: " << slotIndex << std::endl;
            return false;
        }

        int unlockedIndex = -1;
        for (size_t i = 0; i < skillComp.unlockedSkills.size(); ++i) {
            if (skillComp.unlockedSkills[i].skillID == newSkillID) {
                unlockedIndex = static_cast<int>(i);
                break;
            }
        }

        if (unlockedIndex == -1) {
            std::cout << "[SKILL] Cannot replace with skill " << newSkillID << " - not unlocked!" << std::endl;
            return false;
        }

        const char* oldSkillName = "[EMPTY]";
        if (skillComp.equippedSlots[slotIndex] != -1) {
            oldSkillName = skillComp.unlockedSkills[skillComp.equippedSlots[slotIndex]].skillName.c_str();
        }

        const char* newSkillName = skillComp.unlockedSkills[unlockedIndex].skillName.c_str();

        std::cout << "========================================" << std::endl;
        std::cout << "REPLACING SKILL IN SLOT " << (slotIndex + 1) << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  OLD: " << oldSkillName << std::endl;
        std::cout << "  NEW: " << newSkillName << " (ID: " << newSkillID << ")" << std::endl;
        std::cout << "========================================" << std::endl;

        skillComp.equippedSlots[slotIndex] = unlockedIndex;

        skillComp.PrintLoadout();
        return true;
    }

    void SkillSystem::SwapSkillSlots(SkillComponent& skillComp, int fromSlot, int toSlot) {
        if (fromSlot < 0 || fromSlot >= 4 || toSlot < 0 || toSlot >= 4) {
            LOG_ERROR("SKILL", "Invalid slot indices for swap");
            return;
        }

        const char* fromSkillName = "[EMPTY]";
        const char* toSkillName = "[EMPTY]";

        if (skillComp.equippedSlots[fromSlot] != -1) {
            fromSkillName = skillComp.unlockedSkills[skillComp.equippedSlots[fromSlot]].skillName.c_str();
        }
        if (skillComp.equippedSlots[toSlot] != -1) {
            toSkillName = skillComp.unlockedSkills[skillComp.equippedSlots[toSlot]].skillName.c_str();
        }

        LOG_INFO("SKILL", "========================================");
        LOG_INFO("SKILL", "SWAPPING SKILL POSITIONS");
        LOG_INFO("SKILL", "========================================");
        LOG_INFO("SKILL", "BEFORE:");
        LOG_INFO("SKILL", "  Slot %d: %s", fromSlot + 1, fromSkillName);
        LOG_INFO("SKILL", "  Slot %d: %s", toSlot + 1, toSkillName);

        int temp = skillComp.equippedSlots[fromSlot];
        skillComp.equippedSlots[fromSlot] = skillComp.equippedSlots[toSlot];
        skillComp.equippedSlots[toSlot] = temp;

        LOG_INFO("SKILL", "\nAFTER:");
        LOG_INFO("SKILL", "  Slot %d: %s", fromSlot + 1, toSkillName);
        LOG_INFO("SKILL", "  Slot %d: %s", toSlot + 1, fromSkillName);
        LOG_INFO("SKILL", "========================================\n");

        skillComp.PrintLoadout();
    }

    // ====================================================================
    // HELPER FUNCTIONS
    // ====================================================================

    std::vector<SkillData> SkillSystem::GetClassSkills(CharacterClass charClass) {
        std::vector<SkillData> skills;

        switch (charClass) {
        case CharacterClass::Swordmaster:
            skills.push_back(SkillData(1, "Swordmaster Skill 1", CharacterClass::Swordmaster));
            skills.push_back(SkillData(2, "Swordmaster Skill 2", CharacterClass::Swordmaster));
            skills.push_back(SkillData(3, "Swordmaster Skill 3", CharacterClass::Swordmaster));
            skills.push_back(SkillData(4, "Swordmaster Skill 4", CharacterClass::Swordmaster));
            skills.push_back(SkillData(5, "Swordmaster Skill 5", CharacterClass::Swordmaster));
            skills.push_back(SkillData(6, "Swordmaster Skill 6", CharacterClass::Swordmaster));
            skills.push_back(SkillData(7, "Swordmaster Skill 7", CharacterClass::Swordmaster));
            skills.push_back(SkillData(8, "Swordmaster Skill 8", CharacterClass::Swordmaster));
            break;

        case CharacterClass::Magus:
            skills.push_back(SkillData(9, "Magus Skill 1", CharacterClass::Magus));
            skills.push_back(SkillData(10, "Magus Skill 2", CharacterClass::Magus));
            skills.push_back(SkillData(11, "Magus Skill 3", CharacterClass::Magus));
            skills.push_back(SkillData(12, "Magus Skill 4", CharacterClass::Magus));
            skills.push_back(SkillData(13, "Magus Skill 5", CharacterClass::Magus));
            skills.push_back(SkillData(14, "Magus Skill 6", CharacterClass::Magus));
            skills.push_back(SkillData(15, "Magus Skill 7", CharacterClass::Magus));
            skills.push_back(SkillData(16, "Magus Skill 8", CharacterClass::Magus));
            break;

        case CharacterClass::Berserker:
            skills.push_back(SkillData(17, "Berserker Skill 1", CharacterClass::Berserker));
            skills.push_back(SkillData(18, "Berserker Skill 2", CharacterClass::Berserker));
            skills.push_back(SkillData(19, "Berserker Skill 3", CharacterClass::Berserker));
            skills.push_back(SkillData(20, "Berserker Skill 4", CharacterClass::Berserker));
            skills.push_back(SkillData(21, "Berserker Skill 5", CharacterClass::Berserker));
            skills.push_back(SkillData(22, "Berserker Skill 6", CharacterClass::Berserker));
            skills.push_back(SkillData(23, "Berserker Skill 7", CharacterClass::Berserker));
            skills.push_back(SkillData(24, "Berserker Skill 8", CharacterClass::Berserker));
            break;

        default:
            LOG_ERROR("SKILL", "Invalid character class!");
            break;
        }

        return skills;
    }

    const char* SkillSystem::GetClassName(CharacterClass charClass) {
        switch (charClass) {
        case CharacterClass::Swordmaster: return "Swordmaster";
        case CharacterClass::Magus: return "Magus";
        case CharacterClass::Berserker: return "Berserker";
        default: return "Unknown";
        }
    }

    // ====================================================================
    // PRIVATE HOTKEY HELPER FUNCTIONS
    // ====================================================================

    void SkillSystem::SelectClass(CharacterClass charClass) {
        if (hasSelectedClass) {
            std::cout << "[SKILL] Class already selected! Cannot change." << std::endl;
            return;
        }

        std::cout << std::endl;
        std::cout << "########################################" << std::endl;
        std::cout << "# CLASS SELECTED: " << GetClassName(charClass) << std::endl;
        std::cout << "########################################" << std::endl;
        std::cout << std::endl;

        GiveStartingSkills(playerSkills, charClass);
        hasSelectedClass = true;
    }

    void SkillSystem::ReplaceSlotWithNewest(int slotIndex) {
        if (!hasSelectedClass) {
            std::cout << "[SKILL] Select a class first! Press Z, X, or C." << std::endl;
            return;
        }

        if (playerSkills.unlockedSkills.empty()) {
            std::cout << "[SKILL] No skills unlocked!" << std::endl;
            return;
        }

        int newestIndex = static_cast<int>(playerSkills.unlockedSkills.size()) - 1;
        int newestSkillID = playerSkills.unlockedSkills[newestIndex].skillID;

        std::cout << std::endl;
        std::cout << ">>> REPLACING SLOT " << (slotIndex + 1) << " <<<" << std::endl;

        ReplaceSkill(playerSkills, slotIndex, newestSkillID);
    }

} // namespace Framework