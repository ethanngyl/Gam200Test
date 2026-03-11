/**
===============================================================================
 File:           SkillComponent.h        
 Author:         Padilla Carl Jameson Z.
 Email:          c.padilla@digipen.edu
 Date:           2026-01-26
 Contribution:   100%
 ------------------------------------------------------------------------------

 SKILL SYSTEM FOUNDATION

 This file provides the base structure for the skill system.
 NO skills are hardcoded here - your team will add them later.

 HOW TO USE:
 1. Define your skills in SkillDatabase::InitializeSkills()
 2. Add new properties to SkillData struct as needed
 3. Add new enums (CharacterClass, SkillTargetType, etc.) as needed

 STRUCTURE:
 - CharacterClass     : Enum for player classes
 - SkillTargetType    : Enum for skill targeting modes
 - SkillEffectType    : Enum for skill effect categories
 - SkillData          : Struct containing all skill properties
 - SkillDatabase      : Singleton that stores all skill definitions
 - SkillComponent     : ECS component attached to player entity

 Copyright (C) 2026 DigiPen Institute of Technology.
 Reproduction or disclosure of this file or its contents
 without the prior written consent of DigiPen Institute of
 Technology is prohibited.
===============================================================================
*/

#pragma once
#include "Precompiled.h"
#include <iostream>

namespace Framework {

    // ========================================================================
    // ENUMS - Add more values as needed
    // ========================================================================

    /**
     * @brief Player character classes
     * Add new classes here as needed
     */
    enum class CharacterClass : uint8_t {
        // === ADD YOUR CLASSES HERE ===
        Swordmaster = 0,
        Magus = 1,
        Berserker = 2,
        // Add more: Archer, Healer, etc.

        Count,          // Keep this last - used for iteration
        None = 255
    };

    /**
     * @brief How a skill selects its target
     * Add new targeting modes as needed
     */
    enum class SkillTargetType : uint8_t {
        Self,           // Targets the caster
        SingleEnemy,    // Targets one enemy
        AllEnemies,     // Targets all enemies
        SingleAlly,     // Targets one ally
        AllAllies,      // Targets all allies
        Area,           // Area of effect at location
        // Add more as needed

        None
    };

    /**
     * @brief Category of skill effect
     * Add new effect types as needed
     */
    enum class SkillEffectType : uint8_t {
        Physical,       // Physical damage
        Magical,        // Magical damage
        Healing,        // Restore HP
        Buff,           // Positive status effect
        Debuff,         // Negative status effect
        Utility,        // Non-combat (movement, etc.)
        // Add more as needed

        None
    };

    // ========================================================================
    // SKILL DATA - Add new properties as needed
    // ========================================================================

    /**
     * @brief Contains all data for a single skill
     *
     * ADD NEW PROPERTIES HERE - existing code won't break as long as
     * you provide default values
     */
    struct SkillData {
        // === IDENTIFICATION ===
        int skillID = 0;                    // Unique ID
        std::string skillName = "";         // Display name
        std::string description = "";       // Tooltip text
        CharacterClass ownerClass = CharacterClass::None;

        // === PROGRESSION ===
        bool isUnlocked = false;
        int unlockOrder = 0;                // 0-3 = starting, 4+ = unlockable

        // === COMBAT STATS ===
        int apCost = 1;                     // Action points to use
        int cooldown = 0;                   // Turns before can reuse
        int damage = 0;                     // Base damage/heal value
        float damageMultiplier = 1.0f;      // Scaling factor

        // === TARGETING ===
        SkillTargetType targetType = SkillTargetType::SingleEnemy;
        int range = 1;                      // How far can target
        int areaSize = 0;                   // For AoE skills

        // === EFFECT ===
        SkillEffectType effectType = SkillEffectType::Physical;

        // === VISUALS (paths to assets) ===
        std::string iconPath = "";
        std::string animationName = "";
        std::string soundEffect = "";

        // === ADD MORE PROPERTIES BELOW ===
        // Example: int manaCost = 0;
        // Example: float critChance = 0.0f;
        // Example: std::vector<int> statusEffectIDs;

        // === CONSTRUCTORS ===
        SkillData() = default;

        // Minimal constructor
        SkillData(int id, const std::string& name, CharacterClass charClass, int order)
            : skillID(id)
            , skillName(name)
            , ownerClass(charClass)
            , unlockOrder(order)
        {
        }
    };

    // ========================================================================
    // SKILL DATABASE - Define all skills here
    // ========================================================================

    /**
     * @brief Singleton that holds all skill definitions
     */
    class SkillDatabase {
    public:
        static SkillDatabase& GetInstance() {
            static SkillDatabase instance;
            return instance;
        }

        // Get all skills for a class
        const std::vector<SkillData>& GetClassSkills(CharacterClass charClass) const {
            int index = static_cast<int>(charClass);
            if (index >= 0 && index < static_cast<int>(CharacterClass::Count)) {
                return classSkills[index];
            }
            static std::vector<SkillData> empty;
            return empty;
        }

        // Get skill by ID (searches all classes)
        const SkillData* GetSkillByID(int skillID) const {
            for (int c = 0; c < static_cast<int>(CharacterClass::Count); ++c) {
                for (const auto& skill : classSkills[c]) {
                    if (skill.skillID == skillID) {
                        return &skill;
                    }
                }
            }
            return nullptr;
        }

        // Get class name as string
        static const char* GetClassName(CharacterClass charClass) {
            switch (charClass) {
            case CharacterClass::Swordmaster: return "Swordmaster";
            case CharacterClass::Magus: return "Magus";
            case CharacterClass::Berserker: return "Berserker";
                // Add more cases as you add classes
            default: return "Unknown";
            }
        }

        // Get number of skills for a class
        int GetSkillCount(CharacterClass charClass) const {
            return static_cast<int>(GetClassSkills(charClass).size());
        }

    private:
        SkillDatabase() {
            InitializeSkills();
        }

        /**
         * ================================================================
         * YOUR TEAM DEFINES ALL SKILLS HERE
         * ================================================================
         *
         * HOW TO ADD A SKILL:
         *
         *   SkillData skill;
         *   skill.skillID = 1;              // Unique ID
         *   skill.skillName = "Fireball";   // Display name
         *   skill.description = "...";      // Tooltip
         *   skill.ownerClass = CharacterClass::Magus;
         *   skill.unlockOrder = 0;          // 0-3 = starting, 4-7 = unlockable
         *   skill.apCost = 2;
         *   skill.damage = 15;
         *   skill.targetType = SkillTargetType::SingleEnemy;
         *   skill.effectType = SkillEffectType::Magical;
         *   classSkills[static_cast<int>(CharacterClass::Magus)].push_back(skill);
         *
         * ================================================================
         */
        void InitializeSkills() {
            classSkills.resize(static_cast<int>(CharacterClass::Count));

            // =============================================================
            // SWORDMASTER SKILLS (IDs 1-8)
            // =============================================================
            auto& swordmaster = classSkills[static_cast<int>(CharacterClass::Swordmaster)];

            // Starting skills (unlockOrder 0-3)
            swordmaster.push_back(SkillData(1, "Swordmaster Skill 1", CharacterClass::Swordmaster, 0));
            swordmaster.push_back(SkillData(2, "Swordmaster Skill 2", CharacterClass::Swordmaster, 1));
            swordmaster.push_back(SkillData(3, "Swordmaster Skill 3", CharacterClass::Swordmaster, 2));
            swordmaster.push_back(SkillData(4, "Swordmaster Skill 4", CharacterClass::Swordmaster, 3));

            // Unlockable skills (unlockOrder 4-7)
            swordmaster.push_back(SkillData(5, "Swordmaster Skill 5", CharacterClass::Swordmaster, 4));
            swordmaster.push_back(SkillData(6, "Swordmaster Skill 6", CharacterClass::Swordmaster, 5));
            swordmaster.push_back(SkillData(7, "Swordmaster Skill 7", CharacterClass::Swordmaster, 6));
            swordmaster.push_back(SkillData(8, "Swordmaster Skill 8", CharacterClass::Swordmaster, 7));

            // =============================================================
            // MAGUS SKILLS (IDs 9-16)
            // =============================================================
            auto& magus = classSkills[static_cast<int>(CharacterClass::Magus)];

            // Starting skills
            magus.push_back(SkillData(9, "Magus Skill 1", CharacterClass::Magus, 0));
            magus.push_back(SkillData(10, "Magus Skill 2", CharacterClass::Magus, 1));
            magus.push_back(SkillData(11, "Magus Skill 3", CharacterClass::Magus, 2));
            magus.push_back(SkillData(12, "Magus Skill 4", CharacterClass::Magus, 3));

            // Unlockable skills
            magus.push_back(SkillData(13, "Magus Skill 5", CharacterClass::Magus, 4));
            magus.push_back(SkillData(14, "Magus Skill 6", CharacterClass::Magus, 5));
            magus.push_back(SkillData(15, "Magus Skill 7", CharacterClass::Magus, 6));
            magus.push_back(SkillData(16, "Magus Skill 8", CharacterClass::Magus, 7));

            // =============================================================
            // BERSERKER SKILLS (IDs 17-24)
            // =============================================================
            auto& berserker = classSkills[static_cast<int>(CharacterClass::Berserker)];

            // Starting skills
            berserker.push_back(SkillData(17, "Berserker Skill 1", CharacterClass::Berserker, 0));
            berserker.push_back(SkillData(18, "Berserker Skill 2", CharacterClass::Berserker, 1));
            berserker.push_back(SkillData(19, "Berserker Skill 3", CharacterClass::Berserker, 2));
            berserker.push_back(SkillData(20, "Berserker Skill 4", CharacterClass::Berserker, 3));

            // Unlockable skills
            berserker.push_back(SkillData(21, "Berserker Skill 5", CharacterClass::Berserker, 4));
            berserker.push_back(SkillData(22, "Berserker Skill 6", CharacterClass::Berserker, 5));
            berserker.push_back(SkillData(23, "Berserker Skill 7", CharacterClass::Berserker, 6));
            berserker.push_back(SkillData(24, "Berserker Skill 8", CharacterClass::Berserker, 7));

            // =============================================================
            // ADD MORE CLASS SKILLS BELOW
            // =============================================================
        }

        std::vector<std::vector<SkillData>> classSkills;
    };

    // ========================================================================
    // SKILL COMPONENT - Attached to player entity
    // ========================================================================

    /**
     * @brief ECS Component that tracks player's unlocked and equipped skills
     */
    struct SkillComponent {
        CharacterClass playerClass = CharacterClass::None;
        std::vector<SkillData> unlockedSkills;
        int equippedSlots[4] = { -1, -1, -1, -1 };  // -1 = empty slot

        // === QUERIES ===

        bool HasSkill(int skillID) const {
            for (const auto& skill : unlockedSkills) {
                if (skill.skillID == skillID) return true;
            }
            return false;
        }

        const SkillData* GetSkillByID(int skillID) const {
            for (const auto& skill : unlockedSkills) {
                if (skill.skillID == skillID) return &skill;
            }
            return nullptr;
        }

        const SkillData* GetEquippedSkill(int slotIndex) const {
            if (slotIndex < 0 || slotIndex >= 4) return nullptr;
            if (equippedSlots[slotIndex] == -1) return nullptr;
            int idx = equippedSlots[slotIndex];
            if (idx >= 0 && idx < static_cast<int>(unlockedSkills.size())) {
                return &unlockedSkills[idx];
            }
            return nullptr;
        }

        int GetUnlockedCount() const {
            return static_cast<int>(unlockedSkills.size());
        }

        // === MANAGEMENT ===

        void UnlockSkill(const SkillData& skillData) {
            if (HasSkill(skillData.skillID)) {
                std::cout << "[SKILL] " << skillData.skillName << " already unlocked!" << std::endl;
                return;
            }

            SkillData newSkill = skillData;
            newSkill.isUnlocked = true;
            unlockedSkills.push_back(newSkill);

            std::cout << "[SKILL] Unlocked: " << skillData.skillName << std::endl;

            // Auto-equip to first empty slot
            for (int i = 0; i < 4; ++i) {
                if (equippedSlots[i] == -1) {
                    equippedSlots[i] = static_cast<int>(unlockedSkills.size()) - 1;
                    std::cout << "        -> Equipped to Slot " << (i + 1) << std::endl;
                    break;
                }
            }
        }

        bool EquipToSlot(int slotIndex, int unlockedIndex) {
            if (slotIndex < 0 || slotIndex >= 4) return false;
            if (unlockedIndex < 0 || unlockedIndex >= static_cast<int>(unlockedSkills.size())) return false;
            equippedSlots[slotIndex] = unlockedIndex;
            return true;
        }

        void ClearSlot(int slotIndex) {
            if (slotIndex >= 0 && slotIndex < 4) {
                equippedSlots[slotIndex] = -1;
            }
        }

        // === DEBUG ===

        void PrintLoadout() const {
            std::cout << std::endl;
            std::cout << "========================================" << std::endl;
            std::cout << "LOADOUT - " << SkillDatabase::GetClassName(playerClass) << std::endl;
            std::cout << "========================================" << std::endl;

            std::cout << "EQUIPPED:" << std::endl;
            for (int i = 0; i < 4; ++i) {
                std::cout << "  Slot " << (i + 1) << ": ";
                const SkillData* skill = GetEquippedSkill(i);
                if (skill) {
                    std::cout << skill->skillName;
                    if (skill->apCost > 0) std::cout << " (AP:" << skill->apCost << ")";
                    if (skill->damage > 0) std::cout << " (DMG:" << skill->damage << ")";
                }
                else {
                    std::cout << "[EMPTY]";
                }
                std::cout << std::endl;
            }

            std::cout << std::endl;
            std::cout << "UNLOCKED (" << unlockedSkills.size() << "):" << std::endl;
            for (size_t i = 0; i < unlockedSkills.size(); ++i) {
                std::cout << "  " << (i + 1) << ". " << unlockedSkills[i].skillName << std::endl;
            }
            std::cout << "========================================" << std::endl;
        }
    };

} // namespace Framework