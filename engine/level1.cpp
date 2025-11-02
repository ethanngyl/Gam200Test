#include "Precompiled.h"        
#include "Core.h"               
#include "EntitySpawner.h"      
#include "Vector2D.h"    
#include "GSM/GameStateList.h"
#include "GSM/GameStateManager.h"
#include "PlayerManager.h"
#include "ConfigReader/ConfigReader.h"

extern Framework::CoreEngine* engine;
using Framework::Vector2D;
using Framework::EntitySpawner;
using Framework::PlayerControllerSystem;

void level1_Load()
{
    LOG_INFO("LEVEL1", "=== Level1 Load ===");
}

void level1_Initialize()
{
    LOG_INFO("LEVEL1", "=== Level1 Initialize ===");

    if (!engine) {
        LOG_ERROR("LEVEL1", "Engine is null!");
        return;
    }

    EntitySpawner* spawner = engine->GetSpawner();
    if (!spawner) {
        LOG_ERROR("LEVEL1", "Spawner is null!");
        return;
    }

    PlayerControllerSystem* playerControll = engine->GetPlayerController();
    if (!playerControll) {
        LOG_ERROR("LEVEL1", "playerControll is null!");
        return;
    }

    // ========================================================================
    // Spawn Player - Save the returned entity ID
    // ========================================================================
    Framework::Entity playerEntity = spawner->SpawnPlayer(Vector2D(0.0f, -0.5f));

    //------------------------------------------------------------------
    //  SPRITE ANIMATION SETUP FOR PLAYER (config-driven)
    //------------------------------------------------------------------
    auto* em = engine->GetEntityManager();
    auto* gfx = engine->GetGraphicsSystem();

    if (em && gfx) {
        // ============================================================================
        // Load animation setting
        // ============================================================================
        ConfigReader::LoadConfig("assets/anim_Bird.txt");

        // === Give player sprite animation ===
        auto& anim = em->AddComponent<Framework::SpriteAnimation>(playerEntity);

        // Sprite path
        std::string spritePath = ConfigReader::GetString("sprite", "");
        anim.spriteSheet = gfx->GetResourceManager().LoadTexture(spritePath);
        Framework::Texture* tex = gfx->GetResourceManager().GetTexture(anim.spriteSheet);

        // Sheet layout
        anim.rows = ConfigReader::GetInt("rows", 1);
        anim.columns = ConfigReader::GetInt("columns", 1);
        anim.frameCount = ConfigReader::GetInt("frameCount", 1);
        anim.frameTime = ConfigReader::GetFloat("frameTime", 0.0f);
        anim.loop = ConfigReader::GetBool("loop", true);
        anim.uvShrinkPx = ConfigReader::GetFloat("uvShrinkPx", 0.0f);

        // Derived frame size
        anim.frameWidth = tex->GetWidth() / anim.columns;
        anim.frameHeight = tex->GetHeight() / anim.rows;
        anim.playing = true;
        anim.currentFrame = 0;

        // Renderable
        auto& rend = em->AddComponent<Framework::Renderable>(playerEntity);

        // Transform
        auto& xform = em->AddComponent<Framework::Transform>(playerEntity);
        xform.upperLimit = 2.0f;
        xform.lowerLimit = 0.5f;
    }

    // ========================================================================
    // Tell the PlayerController who the player entity is
    // ========================================================================
    playerControll->SetPlayerEntity(playerEntity);

    // Optional: Configure shooting parameters
    playerControll->SetShootCooldown(0.2f);     
    playerControll->SetProjectileSpeed(0.5f);

    LOG_INFO("LEVEL1", "PlayerController configured with entity ID: %u", playerEntity);

    // Generate enemy waves
    spawner->SpawnEnemyWave(5, 0.8f);

    // Generate some obstacles
    spawner->SpawnObstacle(Vector2D(-0.5f, 0.0f), Vector2D(0.3f, 0.3f));
    spawner->SpawnObstacle(Vector2D(0.5f, 0.0f), Vector2D(0.3f, 0.3f));

    // Generate a circular pattern
    spawner->SpawnCircle("circle", 8, Vector2D(0.0f, 0.0f), 0.6f);

    LOG_INFO("LEVEL1", "All entities spawned successfully");
}

void level1_Update()
{
    if (engine && engine->GetInputSystem() &&
        engine->GetInputSystem()->IsKeyPressed(Framework::KEY_5))
    {
         LOG_INFO("LEVEL1", "ESC pressed - returning to menu");
        next = mainMenu;
    }
}

void level1_Draw()
{
    // Only responsible for rendering related matters
    // Do not create entities here
    LOG_INFO("MENU", "=== level1 Draw ===");
}

void level1_Free()
{
    LOG_INFO("LEVEL1", "=== Level1 Free ===");

    // Clean All Entities
    if (engine && engine->GetEntityManager()) {
        auto entities = engine->GetEntityManager()->GetAllEntities();
        for (auto entity : entities) {
            engine->GetEntityManager()->DestroyEntity(entity);
        }
    }
}

void level1_Unload()
{
    LOG_INFO("LEVEL1", "=== Level1 Unload ===");
}