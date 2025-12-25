/**
===============================================================================
 File:           Swordmaster.cpp
 Author:         Claude AI Assistant
 Date:           2025-12-25
 ------------------------------------------------------------------------------

 Implementation of Swordmaster character class
===============================================================================
*/

#include "Swordmaster.h"

namespace Framework
{
    Swordmaster::Swordmaster()
        : EntityClass(
            15,  // STR - High strength for melee damage
            8,   // AGI - Moderate agility
            14,  // VIT - High vitality for defense and HP
            5,   // INT - Low intelligence (physical fighter)
            10,  // DEX - Moderate dexterity for accuracy
            8    // LUCK - Moderate luck
        )
    {
    }

    Swordmaster::Swordmaster(int str, int agi, int vit, int intel, int dex, int luck)
        : EntityClass(str, agi, vit, intel, dex, luck)
    {
    }

} // namespace Framework
