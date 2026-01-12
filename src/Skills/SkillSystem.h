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
 [1] - Select Class: Swordmaster
 [2] - Select Class: Magus
 [3] - Select Class: Berserker

 [U] - Unlock next skill (simulates level complete)

 [5] - Replace Slot 1 with newest unlocked skill
 [6] - Replace Slot 2 with newest unlocked skill
 [7] - Replace Slot 3 with newest unlocked skill
 [8] - Replace Slot 4 with newest unlocked skill

 [P] - Print current skill loadout
 ----------------------------------------
===============================================================================
*/

#pragma once
#include "Precompiled.h"
#include "SkillComponent.h"


namespace Framework {

    class SkillSystem : public EngineSystem {
    public:
        SkillSystem() = default;
        ~SkillSystem() = default;

        void Initialize() override {
            LOG_INFO("SKILL", "SkillSystem Initialized");
            LOG_INFO("SKILL", "========================================");
            LOG_INFO("SKILL", "HOTKEYS:");
            LOG_INFO("SKILL", "  [1/2/3] - Select Class (Swordmaster/Magus/Berserker)");
            LOG_INFO("SKILL", "  [U]     - Unlock next skill");
            LOG_INFO("SKILL", "  [5/6/7/8] - Replace slot 1/2/3/4 with newest skill");
            LOG_INFO("SKILL", "  [P]     - Print loadout");
            LOG_INFO("SKILL", "========================================");
        }

        void Update(float dt) override {
            (void)dt;

            if (!inputSystem) return;

            // ================================================================
            // CLASS SELECTION (1, 2, 3)
            // ================================================================
            if (inputSystem->IsKeyPressed(KEY_1)) {
                SelectClass(CharacterClass::Swordmaster);
            }
            if (inputSystem->IsKeyPressed(KEY_2)) {
                SelectClass(CharacterClass::Magus);
            }
            if (inputSystem->IsKeyPressed(KEY_3)) {
                SelectClass(CharacterClass::Berserker);
            }

            // ================================================================
            // UNLOCK NEXT SKILL (U)
            // ================================================================
            if (inputSystem->IsKeyPressed(KEY_U)) {
                if (!hasSelectedClass) {
                    LOG_WARN("SKILL", "Select a class first! Press 1, 2, or 3.");
                }
                else {
                    LOG_INFO("SKILL", "");
                    LOG_INFO("SKILL", ">>> SIMULATING LEVEL COMPLETE <<<");
                    UnlockNextSkill(playerSkills);
                }
            }

            // ================================================================
            // REPLACE SKILLS (5, 6, 7, 8)
            // ================================================================
            if (inputSystem->IsKeyPressed(KEY_5)) {
                ReplaceSlotWithNewest(0);
            }
            if (inputSystem->IsKeyPressed(KEY_6)) {
                ReplaceSlotWithNewest(1);
            }
            if (inputSystem->IsKeyPressed(KEY_7)) {
                ReplaceSlotWithNewest(2);
            }
            if (inputSystem->IsKeyPressed(KEY_8)) {
                ReplaceSlotWithNewest(3);
            }

            // ================================================================
            // PRINT LOADOUT (P)
            // ================================================================
            if (inputSystem->IsKeyPressed(KEY_P)) {
                if (!hasSelectedClass) {
                    LOG_WARN("SKILL", "Select a class first! Press 1, 2, or 3.");
                }
                else {
                    playerSkills.PrintLoadout();
                }
            }
        }

        void SendEngineMessage(Message* msg) override {
            (void)msg;
        }

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

        static void GiveStartingSkills(SkillComponent& skillComp, CharacterClass charClass) {
            skillComp.playerClass = charClass;

            const char* className = GetClassName(charClass);
            LOG_INFO("SKILL", "Giving %s starting skills...", className);

            auto skills = GetClassSkills(charClass);

            // Unlock first 4 skills only
            for (int i = 0; i < 4 && i < static_cast<int>(skills.size()); ++i) {
                skillComp.UnlockSkill(skills[i].skillID, skills[i].skillName, charClass);
            }

            skillComp.PrintLoadout();
        }

        static void UnlockNextSkill(SkillComponent& skillComp) {
            auto allSkills = GetClassSkills(skillComp.playerClass);

            for (const auto& skillData : allSkills) {
                if (!skillComp.HasSkill(skillData.skillID)) {
                    LOG_INFO("SKILL", "========================================");
                    LOG_INFO("SKILL", "LEVEL COMPLETE! New skill unlocked!");
                    LOG_INFO("SKILL", "========================================");

                    skillComp.unlockedSkills.push_back(SkillData(skillData.skillID, skillData.skillName, skillData.ownerClass, true));

                    LOG_INFO("SKILL", "Unlocked: %s (ID: %d)", skillData.skillName.c_str(), skillData.skillID);
                    LOG_INFO("SKILL", "Use 5/6/7/8 to REPLACE an equipped skill!");
                    LOG_INFO("SKILL", "========================================");

                    skillComp.PrintLoadout();
                    return;
                }
            }

            LOG_WARN("SKILL", "All skills for this class already unlocked!");
        }

        static bool ReplaceSkill(SkillComponent& skillComp, int slotIndex, int newSkillID) {
            if (slotIndex < 0 || slotIndex >= 4) {
                LOG_ERROR("SKILL", "Invalid slot index: %d", slotIndex);
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
                LOG_ERROR("SKILL", "Cannot replace with skill %d - not unlocked!", newSkillID);
                return false;
            }

            const char* oldSkillName = "[EMPTY]";
            if (skillComp.equippedSlots[slotIndex] != -1) {
                oldSkillName = skillComp.unlockedSkills[skillComp.equippedSlots[slotIndex]].skillName.c_str();
            }

            const char* newSkillName = skillComp.unlockedSkills[unlockedIndex].skillName.c_str();

            LOG_INFO("SKILL", "========================================");
            LOG_INFO("SKILL", "REPLACING SKILL IN SLOT %d", slotIndex + 1);
            LOG_INFO("SKILL", "========================================");
            LOG_INFO("SKILL", "  OLD: %s", oldSkillName);
            LOG_INFO("SKILL", "  NEW: %s (ID: %d)", newSkillName, newSkillID);
            LOG_INFO("SKILL", "========================================\n");

            skillComp.equippedSlots[slotIndex] = unlockedIndex;

            skillComp.PrintLoadout();
            return true;
        }

        static void SwapSkillSlots(SkillComponent& skillComp, int fromSlot, int toSlot) {
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

        static std::vector<SkillData> GetClassSkills(CharacterClass charClass) {
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

        static const char* GetClassName(CharacterClass charClass) {
            switch (charClass) {
            case CharacterClass::Swordmaster: return "Swordmaster";
            case CharacterClass::Magus: return "Magus";
            case CharacterClass::Berserker: return "Berserker";
            default: return "Unknown";
            }
        }

    private:
        EntityManager* entityManager = nullptr;
        InputSystem* inputSystem = nullptr;

        // Player skill data (for testing - later attach to player entity)
        SkillComponent playerSkills;
        bool hasSelectedClass = false;

        // ====================================================================
        // HOTKEY HELPER FUNCTIONS
        // ====================================================================

        void SelectClass(CharacterClass charClass) {
            if (hasSelectedClass) {
                LOG_WARN("SKILL", "Class already selected! Cannot change.");
                return;
            }

            LOG_INFO("SKILL", "");
            LOG_INFO("SKILL", "########################################");
            LOG_INFO("SKILL", "# CLASS SELECTED: %s", GetClassName(charClass));
            LOG_INFO("SKILL", "########################################");
            LOG_INFO("SKILL", "");

            GiveStartingSkills(playerSkills, charClass);
            hasSelectedClass = true;
        }

        void ReplaceSlotWithNewest(int slotIndex) {
            if (!hasSelectedClass) {
                LOG_WARN("SKILL", "Select a class first! Press 1, 2, or 3.");
                return;
            }

            if (playerSkills.unlockedSkills.empty()) {
                LOG_ERROR("SKILL", "No skills unlocked!");
                return;
            }

            int newestIndex = static_cast<int>(playerSkills.unlockedSkills.size()) - 1;
            int newestSkillID = playerSkills.unlockedSkills[newestIndex].skillID;

            LOG_INFO("SKILL", "");
            LOG_INFO("SKILL", ">>> REPLACING SLOT %d <<<", slotIndex + 1);

            ReplaceSkill(playerSkills, slotIndex, newestSkillID);
        }
    };

} // namespace Framework