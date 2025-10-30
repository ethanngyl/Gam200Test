/**
===============================================================================
 File:           MainMenu.cpp 
 Description:    Main menu state implementation
===============================================================================
 */

#include "Precompiled.h"
#include "MainMenu.h"
#include "Input/Input.h"
#include "GSM/GameStateList.h"
#include "GSM/GameStateManager.h"
#include "ImguiSystem.h"


// ============================================================================
// MAIN MENU LIFECYCLE FUNCTIONS
// ============================================================================
extern Framework::CoreEngine* engine;

void mainMenu_Load()
{
    LOG_INFO("MENU", "=== Main Menu Load ===");
}

void mainMenu_Initialize()
{
    LOG_INFO("MENU", "=== Main Menu Initialize ===");
    if (engine && engine->GetImGuiSystem()) {
        engine->GetImGuiSystem()->Disable();
        LOG_INFO("MENU", "ImGui disabled in menu");
    }

    if (engine) {
        engine->SetPlaying(false);
        LOG_INFO("MENU", "Menu in EDITOR mode");
    }
}

void mainMenu_Update()
{
    LOG_INFO("MENU", "=== Main Menu Update ===");

    if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_SPACE))
    {
        LOG_INFO("TEST", "Space pressed!");
        next = LEVEL_1;
    }
}

void mainMenu_Draw()
{
    LOG_INFO("MENU", "=== Main Menu Draw ===");
}

void mainMenu_Free()
{
    LOG_INFO("MENU", "=== Main Menu Free ===");

}

void mainMenu_Unload()
{
    LOG_INFO("MENU", "=== Main Menu Unload ===");
}