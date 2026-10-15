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
        // === PLAYER CLASSES ===
        Swordmaster = 0,
        Magus = 1,
        Berserker = 2,

        // === ENEMY CLASSES ===
        EnemyKnight = 3,
        EnemyMage = 4,
        EnemyTank = 5,
        EnemyKnightCommander = 6,

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
        int hpCost = 0;                     // Health points to use
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

        // === SKILL BEHAVIOR ===
        std::string skillType = "melee";     // "melee", "projectile", "buff_ally", "summon", "rallying_cry", "taunt"
        bool multiAttack = false;            // Can attack multiple times if AP allows
        bool requiresLineOfSight = false;    // Projectile needs clear LOS
        int preferredDistance = 0;           // For ranged AI positioning
        float projectileSpeed = 0.0f;        // Speed for projectile skills

        // === STATUS EFFECTS ===
        std::string statusEffect = "";       // Status to apply ("stun", "guard", "knightsOath", etc.)
        int statusDuration = 0;              // How many turns the effect lasts (-1 = permanent)

        // === SUMMON ===
        std::string summonConfig = "";       // Config type for summoned enemy
        int summonDeathThreshold = 0;        // Min deaths before summon available

        // === CONDITION ===
        std::string condition = "";          // "ally_low_health", etc.
        float conditionThreshold = 0.0f;     // Threshold for condition check

        // === TARGETING ===
        bool targetSelfIfNoAlly = false;     // For buff skills: target self if no ally found

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
            case CharacterClass::EnemyKnight: return "EnemyKnight";
            case CharacterClass::EnemyMage: return "EnemyMage";
            case CharacterClass::EnemyTank: return "EnemyTank";
            case CharacterClass::EnemyKnightCommander: return "EnemyKnightCommander";
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

            // Starting skills (unlockOrder 0-3)
            {
                SkillData s(17, "Slam", CharacterClass::Berserker, 0);
                s.skillType = "melee"; s.damage = 2; s.apCost = 1; s.range = 2;
                s.description = "Two tiles forward, 2 damage. Costs 1 AP and 1 HP";
                s.effectType = SkillEffectType::Physical;
                berserker.push_back(s);
            }
            {
                SkillData s(18, "Siphon Charge", CharacterClass::Berserker, 1);
                s.skillType = "buff_ally"; s.apCost = 2;
                s.description = "Consumes 2 AP to restore 2 HP immediately";
                s.statusEffect = "siphonCharge"; s.statusDuration = 0;
                s.effectType = SkillEffectType::Buff;
                berserker.push_back(s);
            }
            {
                SkillData s(19, "Futile Resistance", CharacterClass::Berserker, 2);
                s.skillType = "buff_ally"; s.apCost = 2;
                s.description = "Enemies with <40% HP deal 1 less damage for 2 turns. Costs 1 HP";
                s.statusEffect = "futileResistance"; s.statusDuration = 2;
                s.effectType = SkillEffectType::Buff;
                berserker.push_back(s);
            }
            {
                SkillData s(20, "Dark Omens", CharacterClass::Berserker, 3);
                s.skillType = "buff_ally"; s.apCost = 3;
                s.description = "If lethal damage taken, HP set to 1. Next turn: 2 free skills, then die. Once per level";
                s.statusEffect = "darkOmens"; s.statusDuration = 1;
                s.effectType = SkillEffectType::Buff;
                berserker.push_back(s);
            }

            // Unlockable skills (unlockOrder 4-7)
            {
                SkillData s(21, "Cannibalism", CharacterClass::Berserker, 4);
                s.skillType = "buff_ally"; s.apCost = 2;
                s.description = "Select ally: ally loses 1 HP, you heal 2 HP";
                s.targetType = SkillTargetType::SingleAlly;
                s.effectType = SkillEffectType::Healing;
                berserker.push_back(s);
            }
            {
                SkillData s(22, "Groundshatter", CharacterClass::Berserker, 5);
                s.skillType = "melee"; s.damage = 3; s.apCost = 3; s.areaSize = 5;
                s.description = "5x5 AoE, damages all characters for 3. Costs 3 HP. Self stunned next turn";
                s.statusEffect = "stun"; s.statusDuration = 1;
                s.effectType = SkillEffectType::Physical;
                berserker.push_back(s);
            }
            {
                SkillData s(23, "Bloody Warcry", CharacterClass::Berserker, 6);
                s.skillType = "buff_ally"; s.apCost = 2;
                s.description = "Next turn: all characters first move free, next skill +1 dmg -1 HP. Costs 2 HP";
                s.statusEffect = "bloodyWarcry"; s.statusDuration = 2;
                s.targetType = SkillTargetType::AllAllies;
                s.effectType = SkillEffectType::Buff;
                berserker.push_back(s);
            }
            {
                SkillData s(24, "Bladed Whirlwind", CharacterClass::Berserker, 7);
                s.skillType = "projectile"; s.damage = 1; s.apCost = 1; s.range = 7;
                s.description = "Projectile in facing direction, 1 damage. Enemy takes 1 additional damage at end of turn";
                s.requiresLineOfSight = true; s.projectileSpeed = 3.0f;
                s.statusEffect = "bladedWhirlwindDot"; s.statusDuration = 1;
                s.effectType = SkillEffectType::Physical;
                berserker.push_back(s);
            }

            // =============================================================
            // ENEMY KNIGHT SKILLS (IDs 100-101)
            // =============================================================
            auto& eKnight = classSkills[static_cast<int>(CharacterClass::EnemyKnight)];
            {
                SkillData s(100, "Strike", CharacterClass::EnemyKnight, 0);
                s.skillType = "melee"; s.damage = 1; s.apCost = 1; s.range = 1;
                s.multiAttack = true; s.effectType = SkillEffectType::Physical;
                eKnight.push_back(s);
            }
            {
                SkillData s(101, "Summon Reinforcements", CharacterClass::EnemyKnight, 1);
                s.skillType = "summon"; s.apCost = 2; s.cooldown = 2;
                s.summonConfig = "knight"; s.summonDeathThreshold = 2;
                s.effectType = SkillEffectType::Utility;
                eKnight.push_back(s);
            }

            // =============================================================
            // ENEMY MAGE SKILLS (IDs 110-111)
            // =============================================================
            auto& eMage = classSkills[static_cast<int>(CharacterClass::EnemyMage)];
            {
                SkillData s(110, "Arcane Bolt", CharacterClass::EnemyMage, 0);
                s.skillType = "projectile"; s.damage = 1; s.apCost = 2; s.range = 7;
                s.multiAttack = true; s.requiresLineOfSight = true;
                s.preferredDistance = 3; s.projectileSpeed = 3.0f;
                s.effectType = SkillEffectType::Magical;
                eMage.push_back(s);
            }
            {
                SkillData s(111, "Barrier", CharacterClass::EnemyMage, 1);
                s.skillType = "buff_ally"; s.apCost = 2; s.cooldown = 2;
                s.statusEffect = "guard"; s.statusDuration = -1;
                s.targetSelfIfNoAlly = true;
                s.effectType = SkillEffectType::Buff;
                eMage.push_back(s);
            }

            // =============================================================
            // ENEMY TANK SKILLS (IDs 120-121)
            // =============================================================
            auto& eTank = classSkills[static_cast<int>(CharacterClass::EnemyTank)];
            {
                SkillData s(120, "Shield Bash", CharacterClass::EnemyTank, 0);
                s.skillType = "melee"; s.damage = 2; s.apCost = 3; s.range = 1;
                s.statusEffect = "stun"; s.statusDuration = 1;
                s.effectType = SkillEffectType::Physical;
                eTank.push_back(s);
            }
            {
                SkillData s(121, "Taunt", CharacterClass::EnemyTank, 1);
                s.skillType = "taunt"; s.apCost = 2;
                s.statusEffect = "knightsOath"; s.statusDuration = -1;
                s.condition = "ally_low_health"; s.conditionThreshold = 0.5f;
                s.effectType = SkillEffectType::Utility;
                eTank.push_back(s);
            }

            // =============================================================
            // ENEMY KNIGHT COMMANDER SKILLS (IDs 130-131)
            // =============================================================
            auto& eCommander = classSkills[static_cast<int>(CharacterClass::EnemyKnightCommander)];
            {
                SkillData s(130, "Strike", CharacterClass::EnemyKnightCommander, 0);
                s.skillType = "melee"; s.damage = 1; s.apCost = 1; s.range = 1;
                s.multiAttack = true; s.effectType = SkillEffectType::Physical;
                eCommander.push_back(s);
            }
            {
                SkillData s(131, "Rallying Cry", CharacterClass::EnemyKnightCommander, 1);
                s.skillType = "rallying_cry"; s.apCost = 2; s.cooldown = 3;
                s.effectType = SkillEffectType::Buff;
                eCommander.push_back(s);
            }

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