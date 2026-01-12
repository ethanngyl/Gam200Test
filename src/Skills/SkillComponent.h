/**
===============================================================================
 File:           SkillComponent.h
 Author:        
 Date:           2025-01-12
 ------------------------------------------------------------------------------

 ECS Component for managing player skills with class-based progression.

 - Each class has 8 skills (4 starting + 4 unlockable)
 - Player can only equip 4 skills at once
 - Old skills can be REPLACED with newly unlocked skills
 - All unlocked skills remain in the pool permanently
===============================================================================
*/

#pragma once
#include "Precompiled.h"

namespace Framework {

    /**
     * @brief Character class types (matches your EntityClass derivatives)
     */
    enum class CharacterClass : uint8_t {
        Swordmaster = 0,
        Magus = 1,
        Berserker = 2,
        None = 255
    };

    /**
     * @brief Base skill data structure
     */
    struct SkillData {
        int skillID = 0;
        std::string skillName = "";
        CharacterClass ownerClass;
        bool isUnlocked = false;

        SkillData() : ownerClass(CharacterClass::None) {}

        SkillData(int id, const std::string& name, CharacterClass charClass, bool unlocked = false)
            : skillID(id), skillName(name), ownerClass(charClass), isUnlocked(unlocked) {
        }
    };

    /**
     * @brief ECS Component for player skill management
     */
    struct SkillComponent {
        CharacterClass playerClass = CharacterClass::None;
        std::vector<SkillData> unlockedSkills;
        int equippedSlots[4] = { -1, -1, -1, -1 };

        bool HasSkill(int skillID) const {
            for (const auto& skill : unlockedSkills) {
                if (skill.skillID == skillID) {
                    return true;
                }
            }
            return false;
        }

        void UnlockSkill(int skillID, const std::string& skillName, CharacterClass charClass) {
            if (HasSkill(skillID)) {
                LOG_WARN("SKILL", "Skill %d (%s) already unlocked!", skillID, skillName.c_str());
                return;
            }

            unlockedSkills.push_back(SkillData(skillID, skillName, charClass, true));
            LOG_INFO("SKILL", "Unlocked new skill: %s (ID: %d, Class: %d)",
                skillName.c_str(), skillID, static_cast<int>(charClass));

            // Auto-equip to first empty slot
            for (int i = 0; i < 4; ++i) {
                if (equippedSlots[i] == -1) {
                    equippedSlots[i] = static_cast<int>(unlockedSkills.size()) - 1;
                    LOG_INFO("SKILL", "Auto-equipped %s to slot %d", skillName.c_str(), i + 1);
                    break;
                }
            }
        }

        bool EquipSkill(int slotIndex, int unlockedIndex) {
            if (slotIndex < 0 || slotIndex >= 4) {
                LOG_ERROR("SKILL", "Invalid slot index: %d", slotIndex);
                return false;
            }

            if (unlockedIndex < 0 || unlockedIndex >= static_cast<int>(unlockedSkills.size())) {
                LOG_ERROR("SKILL", "Invalid unlocked skill index: %d", unlockedIndex);
                return false;
            }

            equippedSlots[slotIndex] = unlockedIndex;
            LOG_INFO("SKILL", "Equipped %s to slot %d",
                unlockedSkills[unlockedIndex].skillName.c_str(), slotIndex + 1);
            return true;
        }

        const SkillData* GetEquippedSkill(int slotIndex) const {
            if (slotIndex < 0 || slotIndex >= 4) return nullptr;
            if (equippedSlots[slotIndex] == -1) return nullptr;

            int index = equippedSlots[slotIndex];
            if (index >= 0 && index < static_cast<int>(unlockedSkills.size())) {
                return &unlockedSkills[index];
            }
            return nullptr;
        }

        void PrintLoadout() const {
            const char* className = "Unknown";
            switch (playerClass) {
            case CharacterClass::Swordmaster: className = "Swordmaster"; break;
            case CharacterClass::Magus: className = "Magus"; break;
            case CharacterClass::Berserker: className = "Berserker"; break;
            default: className = "None"; break;
            }

            LOG_INFO("SKILL", "========================================");
            LOG_INFO("SKILL", "PLAYER SKILL LOADOUT - %s", className);
            LOG_INFO("SKILL", "========================================");
            LOG_INFO("SKILL", "Equipped Skills:");
            for (int i = 0; i < 4; ++i) {
                if (equippedSlots[i] != -1) {
                    const auto& skill = unlockedSkills[equippedSlots[i]];
                    LOG_INFO("SKILL", "  Slot %d: %s (ID: %d)",
                        i + 1, skill.skillName.c_str(), skill.skillID);
                }
                else {
                    LOG_INFO("SKILL", "  Slot %d: [EMPTY]", i + 1);
                }
            }

            LOG_INFO("SKILL", "\nUnlocked Skills (%zu total):", unlockedSkills.size());
            for (size_t i = 0; i < unlockedSkills.size(); ++i) {
                const auto& skill = unlockedSkills[i];
                LOG_INFO("SKILL", "  %zu. %s (ID: %d)",
                    i + 1, skill.skillName.c_str(), skill.skillID);
            }

            LOG_INFO("SKILL", "========================================");
        }
    };

} // namespace Framework