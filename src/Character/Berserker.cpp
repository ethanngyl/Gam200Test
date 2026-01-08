/**
===============================================================================
 File:           Berserker.cpp
 Author:         Claude AI Assistant
 Date:           2025-12-25
 ------------------------------------------------------------------------------

 Implementation of Berserker character class
===============================================================================
*/

#include "Berserker.h"

namespace Framework
{
    Berserker::Berserker()
        : EntityClass(
            17,  // STR - Very high strength for maximum damage
            11,  // AGI - Moderate agility for aggressive playstyle
            9,   // VIT - Lower vitality (glass cannon approach)
            4,   // INT - Very low intelligence
            8,   // DEX - Lower dexterity (relies on raw power)
            14   // LUCK - Very high luck for crits and fortune
        )
    {
    }

    Berserker::Berserker(int str, int agi, int vit, int intel, int dex, int luck)
        : EntityClass(str, agi, vit, intel, dex, luck)
    {
    }

} // namespace Framework
