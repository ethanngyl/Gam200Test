/**
===============================================================================
 File:           EntityClass.h
 Author:         Claude AI Assistant
 Date:           2025-12-25
 ------------------------------------------------------------------------------

 Design notes:
 Base class for character types with RPG-style stat attributes.
 Provides foundation for derived classes (Swordmaster, Magus, Berserker, etc.)
===============================================================================
*/

#pragma once

namespace Framework
{
    /**
     * @class EntityClass
     * @brief Base class for character types with RPG stat attributes
     *
     * Defines core character statistics that all derived character classes inherit.
     * Stats are implemented as integers representing character strengths/weaknesses.
     */
    class EntityClass
    {
    protected:
        int STR;   ///< Strength - Physical power and melee damage
        int AGI;   ///< Agility - Speed, evasion, and initiative
        int VIT;   ///< Vitality - Health points and physical defense
        int INT;   ///< Intelligence - Magic power and mana capacity
        int DEX;   ///< Dexterity - Accuracy and critical hit chance
        int LUCK;  ///< Luck - Item drops, critical chance, and fortune

    public:
        /**
         * @brief Default constructor - initializes all stats to 0
         */
        EntityClass();

        /**
         * @brief Parameterized constructor
         * @param str Strength value
         * @param agi Agility value
         * @param vit Vitality value
         * @param intel Intelligence value
         * @param dex Dexterity value
         * @param luck Luck value
         */
        EntityClass(int str, int agi, int vit, int intel, int dex, int luck);

        /**
         * @brief Virtual destructor for proper cleanup of derived classes
         */
        virtual ~EntityClass() = default;

        // Getters
        int GetSTR() const { return STR; }
        int GetAGI() const { return AGI; }
        int GetVIT() const { return VIT; }
        int GetINT() const { return INT; }
        int GetDEX() const { return DEX; }
        int GetLUCK() const { return LUCK; }

        // Setters
        void SetSTR(int value) { STR = value; }
        void SetAGI(int value) { AGI = value; }
        void SetVIT(int value) { VIT = value; }
        void SetINT(int value) { INT = value; }
        void SetDEX(int value) { DEX = value; }
        void SetLUCK(int value) { LUCK = value; }

        /**
         * @brief Get character class name (override in derived classes)
         * @return String representing the class name
         */
        virtual const char* GetClassName() const { return "EntityClass"; }

        /**
         * @brief Calculate total stat sum
         * @return Sum of all stat values
         */
        int GetTotalStats() const;
    };

} // namespace Framework
