/**
===============================================================================
 File:           Swordmaster.h
 Author:         Claude AI Assistant
 Date:           2025-12-25
 ------------------------------------------------------------------------------

 Design notes:
 Swordmaster character class - high STR and VIT for melee combat
===============================================================================
*/

#pragma once
#include "EntityClass.h"

namespace Framework
{
    /**
     * @class Swordmaster
     * @brief Melee warrior class with high strength and vitality
     *
     * Specializes in close-range physical combat with strong defense.
     * Excels in STR and VIT, moderate DEX, lower INT and AGI.
     */
    class Swordmaster : public EntityClass
    {
    public:
        /**
         * @brief Default constructor with Swordmaster stat distribution
         */
        Swordmaster();

        /**
         * @brief Custom constructor for specific stat values
         * @param str Strength value
         * @param agi Agility value
         * @param vit Vitality value
         * @param intel Intelligence value
         * @param dex Dexterity value
         * @param luck Luck value
         */
        Swordmaster(int str, int agi, int vit, int intel, int dex, int luck);

        /**
         * @brief Get class name
         * @return "Swordmaster"
         */
        const char* GetClassName() const override { return "Swordmaster"; }
    };

} // namespace Framework
