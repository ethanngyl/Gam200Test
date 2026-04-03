#include "Precompiled.h"
#include "StateProfiles.h"

#include "GameStateList.h"

namespace {
    struct StateProfileEntry {
        int state;
        Framework::StateProfiles::Kind kind;
    };

    constexpr StateProfileEntry kStateProfiles[] = {
        { mainMenu, Framework::StateProfiles::Kind::StandardScripted },
        { settingsMenu, Framework::StateProfiles::Kind::StandardScripted },
        { Level_select, Framework::StateProfiles::Kind::StandardScripted },
        { LEVEL_2, Framework::StateProfiles::Kind::Level2Lua },
        { LEVEL_3, Framework::StateProfiles::Kind::Level3Hybrid },
        { LEVEL_END, Framework::StateProfiles::Kind::StandardScripted },
        { TUTORIAL, Framework::StateProfiles::Kind::StandardScripted },
        { CONTROL, Framework::StateProfiles::Kind::StandardScripted },
        { CONTROL2, Framework::StateProfiles::Kind::StandardScripted },
        { SKILL_SETS, Framework::StateProfiles::Kind::StandardScripted },
        { WIN_SCREEN, Framework::StateProfiles::Kind::StandardScripted },
        { LOSE_SCREEN, Framework::StateProfiles::Kind::StandardScripted },
        { DEMO_BRIDGE, Framework::StateProfiles::Kind::StandardScripted },
        { GS_QUIT, Framework::StateProfiles::Kind::Quit },
        { GS_RESTART, Framework::StateProfiles::Kind::Restart }
    };
}

namespace Framework::StateProfiles {

Kind GetKind(int state)
{
    for (const auto& entry : kStateProfiles) {
        if (entry.state == state) {
            return entry.kind;
        }
    }
    return Kind::Unknown;
}

} // namespace Framework::StateProfiles
