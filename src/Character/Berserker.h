/**
===============================================================================
 File:           Berserker.h
 Author:         Claude AI Assistant
 Date:           2025-12-25
 ------------------------------------------------------------------------------

 Design notes:
 Berserker character class - high STR and LUCK for high-risk combat
===============================================================================
*/

#pragma once
#include "EntityClass.h"

namespace Framework
{
    /**
     * @class Berserker
     * @brief Aggressive melee class with high damage and luck
     *
     * Specializes in high-risk, high-reward combat style.
     * Excels in STR and LUCK, moderate AGI, lower VIT and INT.
     */
    class Berserker : public EntityClass
    {
    public:
        /**
         * @brief Default constructor with Berserker stat distribution
         */
        Berserker();

        /**
         * @brief Custom constructor for specific stat values
         * @param str Strength value
         * @param agi Agility value
         * @param vit Vitality value
         * @param intel Intelligence value
         * @param dex Dexterity value
         * @param luck Luck value
         */
        Berserker(int str, int agi, int vit, int intel, int dex, int luck);

        /**
         * @brief Get class name
         * @return "Berserker"
         */
        const char* GetClassName() const override { return "Berserker"; }
    };

} // namespace Framework
