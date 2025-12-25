/**
===============================================================================
 File:           EntityClass.cpp
 Author:         Claude AI Assistant
 Date:           2025-12-25
 ------------------------------------------------------------------------------

 Implementation of EntityClass base class
===============================================================================
*/

#include "EntityClass.h"

namespace Framework
{
    EntityClass::EntityClass()
        : STR(0), AGI(0), VIT(0), INT(0), DEX(0), LUCK(0)
    {
    }

    EntityClass::EntityClass(int str, int agi, int vit, int intel, int dex, int luck)
        : STR(str), AGI(agi), VIT(vit), INT(intel), DEX(dex), LUCK(luck)
    {
    }

    int EntityClass::GetTotalStats() const
    {
        return STR + AGI + VIT + INT + DEX + LUCK;
    }

} // namespace Framework
