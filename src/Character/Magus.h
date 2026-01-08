/**
===============================================================================
 File:           Magus.h
 Author:         Claude AI Assistant
 Date:           2025-12-25
 ------------------------------------------------------------------------------

 Design notes:
 Magus character class - high INT for magic casting
===============================================================================
*/

#pragma once
#include "EntityClass.h"

namespace Framework
{
    /**
     * @class Magus
     * @brief Magic-focused caster class with high intelligence
     *
     * Specializes in spell casting and magical abilities.
     * Excels in INT, moderate DEX and LUCK, lower STR and VIT.
     */
    class Magus : public EntityClass
    {
    public:
        /**
         * @brief Default constructor with Magus stat distribution
         */
        Magus();

        /**
         * @brief Custom constructor for specific stat values
         * @param str Strength value
         * @param agi Agility value
         * @param vit Vitality value
         * @param intel Intelligence value
         * @param dex Dexterity value
         * @param luck Luck value
         */
        Magus(int str, int agi, int vit, int intel, int dex, int luck);

        /**
         * @brief Get class name
         * @return "Magus"
         */
        const char* GetClassName() const override { return "Magus"; }
    };

} // namespace Framework
