#pragma once

namespace Framework::StateProfiles {

enum class Kind {
    StandardScripted,
    Level2Lua,
    Level3Hybrid,
    Restart,
    Quit,
    Unknown
};

Kind GetKind(int state);

} // namespace Framework::StateProfiles
