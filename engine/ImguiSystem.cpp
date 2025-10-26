/**
===============================================================================
 SIMPLE FIX: Just Add ## to Window Names

 No docking features needed - just unique IDs to prevent tab drag issues
===============================================================================
 */

#include "Precompiled.h"
#include "ImGuiSystem.h"
#include "EntitySpawner.h"

namespace Framework {

    ImGuiSystem::ImGuiSystem()
        : window(nullptr)
        , entityManager(nullptr)
        , entitySpawner(nullptr)
        , showDemo(false)
        , showEntityInspector(true)
        , showSpawner(true)
        , showDebug(true)
        , frameTime(0.0f)
        , entityCount(0)
    {
    }

    void ImGuiSystem::Shutdown()
    {
        std::cout << "[ImGui] Shutting down...\n";

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        std::cout << "[ImGui] Shutdown complete\n";
    }

    ImGuiSystem::~ImGuiSystem()
    {
        Shutdown();
    }

    void ImGuiSystem::Initialize()
    {
        if (!window) {
            std::cout << "[ImGui] Error: Window not set!\n";
            return;
        }

        glfwMakeContextCurrent(window);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;  // Disable settings file

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        std::cout << "[ImGui] Initialized successfully\n";
    }

    void ImGuiSystem::Update(float dt)
    {
        frameTime = dt;
        if (entityManager) {
            entityCount = static_cast<int>(entityManager->GetAllEntities().size());
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Entity Inspector", nullptr, &showEntityInspector);
                ImGui::MenuItem("Spawner", nullptr, &showSpawner);
                ImGui::MenuItem("Debug Info", nullptr, &showDebug);
                ImGui::MenuItem("ImGui Demo", nullptr, &showDemo);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Show windows
        if (showEntityInspector) ShowEntityInspector();
        if (showSpawner) ShowSpawnerWindow();
        if (showDebug) ShowDebugWindow();
        if (showDemo) ImGui::ShowDemoWindow(&showDemo);
    }

    void ImGuiSystem::Render()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void ImGuiSystem::SendEngineMessage(Message* msg)
    {
        (void)msg;
    }

    void ImGuiSystem::SetWindow(GLFWwindow* win)
    {
        window = win;
    }

    void ImGuiSystem::SetEntityManager(EntityManager* em)
    {
        entityManager = em;
    }

    void ImGuiSystem::SetEntitySpawner(EntitySpawner* spawner)
    {
        entitySpawner = spawner;
    }

    // ============================================================================
    // UI WINDOWS - WITH ## UNIQUE IDS
    // ============================================================================

    void ImGuiSystem::ShowEntityInspector()
    {
        if (!entityManager) return;

        ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);

        // ✅ FIX: Add ##UniqueID to make this window unique
        if (!ImGui::Begin("Entity Inspector##Inspector1", &showEntityInspector)) {
            ImGui::End();
            return;
        }

        ImGui::Text("Total Entities: %d", entityCount);
        ImGui::Separator();

        std::vector<Entity> entityCopy = entityManager->GetAllEntities();

        // Limit display
        const size_t maxShow = 20;
        if (entityCopy.size() > maxShow) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Showing %zu/%zu", maxShow, entityCopy.size());
            entityCopy.resize(maxShow);
        }

        Entity entityToDelete = { 0 };
        bool shouldDelete = false;

        for (size_t i = 0; i < entityCopy.size(); ++i) {
            Entity entity = entityCopy[i];

            // Super unique ID
            ImGui::PushID(static_cast<int>(entity.id * 10000 + i));

            char label[128];
            snprintf(label, sizeof(label), "Entity %u", entity.id);

            if (ImGui::CollapsingHeader(label)) {

                if (entityManager->HasComponent<Transform>(entity)) {
                    auto& transform = entityManager->GetComponent<Transform>(entity);
                    ImGui::Text("Transform:");
                    ImGui::DragFloat2("Position##Pos", &transform.position.x, 0.01f, -10.0f, 10.0f);
                    ImGui::DragFloat2("Scale##Scl", &transform.scale.x, 0.01f, 0.01f, 10.0f);
                }

                if (entityManager->HasComponent<Movement>(entity)) {
                    auto& movement = entityManager->GetComponent<Movement>(entity);
                    ImGui::Text("Movement:");
                    ImGui::DragFloat2("Direction##Dir", &movement.direction.x, 0.01f, -1.0f, 1.0f);
                    ImGui::DragFloat("Speed##Spd", &movement.moveSpeed, 0.01f, 0.0f, 2.0f);
                }

                if (entityManager->HasComponent<Sprite>(entity)) {
                    auto& sprite = entityManager->GetComponent<Sprite>(entity);
                    ImGui::Text("Sprite: %s", sprite.texturePath.c_str());
                }

                ImGui::Separator();

                // Unique button ID
                if (ImGui::Button("Delete##DelBtn")) {
                    entityToDelete = entity;
                    shouldDelete = true;
                }
            }

            ImGui::PopID();
        }

        ImGui::End();

        if (shouldDelete && entityToDelete.id != 0) {
            entityManager->DestroyEntity(entityToDelete);
        }
    }

    void ImGuiSystem::ShowSpawnerWindow()
    {
        if (!entitySpawner) return;

        ImGui::SetNextWindowSize(ImVec2(250, 450), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(370, 30), ImGuiCond_FirstUseEver);

        // ✅ FIX: Add ##UniqueID
        if (!ImGui::Begin("Entity Spawner##Spawner1", &showSpawner)) {
            ImGui::End();
            return;
        }

        ImGui::Text("Spawn Entities");
        ImGui::Separator();

        static float spawnX = 0.0f;
        static float spawnY = 0.0f;
        ImGui::DragFloat("Spawn X##SpX", &spawnX, 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("Spawn Y##SpY", &spawnY, 0.01f, -1.0f, 1.0f);

        ImGui::Separator();

        //FIX: All buttons have unique ##IDs
        if (ImGui::Button("Spawn Player##Btn1", ImVec2(-1, 0))) {
            entitySpawner->SpawnPlayer(Vector2D(spawnX, spawnY));
        }

        if (ImGui::Button("Spawn Enemy##Btn2", ImVec2(-1, 0))) {
            entitySpawner->SpawnEnemy(Vector2D(spawnX, spawnY));
        }

        if (ImGui::Button("Spawn Projectile##Btn3", ImVec2(-1, 0))) {
            entitySpawner->SpawnProjectile(
                Vector2D(spawnX, spawnY),
                Vector2D(0.0f, 1.0f),
                0.5f
            );
        }

        if (ImGui::Button("Spawn Obstacle##Btn4", ImVec2(-1, 0))) {
            entitySpawner->SpawnObstacle(
                Vector2D(spawnX, spawnY),
                Vector2D(0.3f, 0.3f)
            );
        }

        ImGui::Separator();

        static int waveCount = 5;
        ImGui::SliderInt("Wave Size##Wave", &waveCount, 1, 20);
        if (ImGui::Button("Spawn Enemy Wave##Btn5", ImVec2(-1, 0))) {
            entitySpawner->SpawnEnemyWave(waveCount, 0.8f);
        }

        ImGui::Separator();

        if (ImGui::Button("Spawn Circle Pattern##Btn6", ImVec2(-1, 0))) {
            entitySpawner->SpawnCircle("circle", 12, Vector2D(0, 0), 0.8f);
        }

        if (ImGui::Button("Spawn Grid Pattern##Btn7", ImVec2(-1, 0))) {
            entitySpawner->SpawnGrid("wireframequad", 4, 4, Vector2D(-0.6f, -0.4f), Vector2D(0.1f, 0.1f));
        }

        ImGui::End();
    }

    void ImGuiSystem::ShowDebugWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(250, 250), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(630, 30), ImGuiCond_FirstUseEver);

        // ✅ FIX: Add ##UniqueID
        if (!ImGui::Begin("Debug Info##Debug1", &showDebug)) {
            ImGui::End();
            return;
        }

        ImGui::Text("%.3f ms/frame (%.1f FPS)",
            frameTime * 1000.0f,
            1.0f / (frameTime + 0.001f));

        ImGui::Text("Entities: %d", entityCount);

        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::BulletText("WASD: Move");
        ImGui::BulletText("SPACE: Shoot Up");
        ImGui::BulletText("SHIFT: Shoot Down");
        ImGui::BulletText("E: Spawn Enemy");
        ImGui::BulletText("O: Spawn Obstacle");
        ImGui::BulletText("R: Spawn Pickup");
        ImGui::BulletText("Q/ESC: Quit");

        ImGui::End();
    }

} // namespace Framework