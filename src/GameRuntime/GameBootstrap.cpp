#include "Precompiled.h"
#include "GameBootstrap.h"

#include "EngineScriptAPIProvider.h"
#include "GameScriptAPIProvider.h"
#include "ScriptAPIRegistry.h"

namespace Framework::GameBootstrap {

void Initialize()
{
    Framework::ScriptAPIRegistry::ClearProviders();
    Framework::ScriptAPIRegistry::RegisterEngineProvider(Framework::EngineScriptAPIProvider::Register);
    Framework::ScriptAPIRegistry::RegisterGameProvider(Framework::GameScriptAPIProvider::Register);

    LOG_INFO("GameBootstrap", "Script API providers initialized (engine + game)");
}

} // namespace Framework::GameBootstrap
