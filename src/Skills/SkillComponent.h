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
#include <iostream>

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
                std::cout << "[SKILL] Skill " << skillID << " (" << skillName << ") already unlocked!" << std::endl;
                return;
            }

            unlockedSkills.push_back(SkillData(skillID, skillName, charClass, true));
            std::cout << "[SKILL] Unlocked new skill: " << skillName << " (ID: " << skillID << ")" << std::endl;

            // Auto-equip to first empty slot
            for (int i = 0; i < 4; ++i) {
                if (equippedSlots[i] == -1) {
                    equippedSlots[i] = static_cast<int>(unlockedSkills.size()) - 1;
                    std::cout << "[SKILL] Auto-equipped " << skillName << " to slot " << (i + 1) << std::endl;
                    break;
                }
            }
        }

        bool EquipSkill(int slotIndex, int unlockedIndex) {
            if (slotIndex < 0 || slotIndex >= 4) {
                std::cout << "[SKILL] Invalid slot index: " << slotIndex << std::endl;
                return false;
            }

            if (unlockedIndex < 0 || unlockedIndex >= static_cast<int>(unlockedSkills.size())) {
                std::cout << "[SKILL] Invalid unlocked skill index: " << unlockedIndex << std::endl;
                return false;
            }

            equippedSlots[slotIndex] = unlockedIndex;
            std::cout << "[SKILL] Equipped " << unlockedSkills[unlockedIndex].skillName << " to slot " << (slotIndex + 1) << std::endl;
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

            std::cout << std::endl;
            std::cout << "========================================" << std::endl;
            std::cout << "PLAYER SKILL LOADOUT - " << className << std::endl;
            std::cout << "========================================" << std::endl;
            std::cout << "Equipped Skills:" << std::endl;
            for (int i = 0; i < 4; ++i) {
                if (equippedSlots[i] != -1) {
                    const auto& skill = unlockedSkills[equippedSlots[i]];
                    std::cout << "  Slot " << (i + 1) << ": " << skill.skillName << " (ID: " << skill.skillID << ")" << std::endl;
                }
                else {
                    std::cout << "  Slot " << (i + 1) << ": [EMPTY]" << std::endl;
                }
            }

            std::cout << std::endl;
            std::cout << "Unlocked Skills (" << unlockedSkills.size() << " total):" << std::endl;
            for (size_t i = 0; i < unlockedSkills.size(); ++i) {
                const auto& skill = unlockedSkills[i];
                std::cout << "  " << (i + 1) << ". " << skill.skillName << " (ID: " << skill.skillID << ")" << std::endl;
            }

            std::cout << "========================================" << std::endl;
            std::cout << std::endl;
        }
    };

} // namespace Framework