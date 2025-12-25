/**
===============================================================================
 File:           Magus.cpp
 Author:         Claude AI Assistant
 Date:           2025-12-25
 ------------------------------------------------------------------------------

 Implementation of Magus character class
===============================================================================
*/

#include "Magus.h"

namespace Framework
{
    Magus::Magus()
        : EntityClass(
            5,   // STR - Low strength (physical weakness)
            9,   // AGI - Moderate agility
            7,   // VIT - Low vitality (fragile)
            16,  // INT - Very high intelligence for magic power
            11,  // DEX - Moderate-high dexterity for spell accuracy
            12   // LUCK - High luck for critical spells
        )
    {
    }

    Magus::Magus(int str, int agi, int vit, int intel, int dex, int luck)
        : EntityClass(str, agi, vit, intel, dex, luck)
    {
    }

} // namespace Framework
