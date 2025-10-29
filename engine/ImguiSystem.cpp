/**
===============================================================================
 SIMPLE FIX: Just Add ## to Window Names

 No docking features needed - just unique IDs to prevent tab drag issues
===============================================================================
 */

#include "Precompiled.h"
#include "ImGuiSystem.h"
#include "EntitySpawner.h"
#include "AudioSystem.h"
namespace Framework {

    ImGuiSystem::ImGuiSystem()
        : window(nullptr)
        , entityManager(nullptr)
        , entitySpawner(nullptr)
        , audioSystem(nullptr)
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
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; //Enable Docking
        io.IniFilename = nullptr;  // Disable settings file

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        std::cout << "[ImGui] Initialized successfully\n";
    }

	// jiahao
    bool ImGuiSystem::OpenLevelFromTxt(const std::string& file, bool clearAll) {

		std::ifstream readFile(file);

        if (!readFile.is_open()) {
            std::cerr << "[ImGuiError] Could not open file for reading: " << file << "\n";
			return false;
        }

        if (!entityManager) {
            std::cerr << "[ImGuiError] ImGuiSystem missing managers for loading.\n";
            return false;
        }

        if (clearAll) {
            entityManager->ClearAllEntities();
        }

        Framework::Entity Entity;
        bool hasEntity = false;

        std::string line;
        auto lineNumber = 0;
        while (std::getline(readFile, line)) {

			++lineNumber;

            if (line.empty() || line[0] == '#') {
                continue;
            }
			std::istringstream iss(line);
            std::string word;

            if (!(iss >> word)) {
                std::cerr << "[ImGuiError] missing entity at line " << lineNumber << "\n";
                continue;
            }

            if (word == "entity") {
                Entity = entityManager->CreateEntity();
                hasEntity = true;
                continue;
            }

            if (!hasEntity) {
                continue;
            }

            if (word == "Transform") {
                float px, py, sx, sy;
                if (iss >> px >> py >> sx >> sy) {
                    entityManager->AddComponent<Framework::Transform>(Entity);
					auto& transform = entityManager->GetComponent<Framework::Transform>(Entity);
                    transform.position = Vector2D(px, py);
                    transform.scale = Vector2D(sx, sy);
                }
                else {
                    std::cerr << "[ImGuiError] parsing Transform at line " << lineNumber << "\n";
                    continue;
                }
            }

            if (word == "Sprite") {
                std::string name;
                if (iss >> name) {
					entityManager->AddComponent<Framework::Sprite>(Entity);
					auto& sprite = entityManager->GetComponent<Framework::Sprite>(Entity);
					sprite.texturePath = name;
                }
                continue;
            }

            if (word == "Movement") {
                float speed, dx, dy;
                if (iss >> speed >> dx >> dy)
                {
					entityManager->AddComponent<Framework::Movement>(Entity);
                    auto& movement = entityManager->GetComponent<Framework::Movement>(Entity);
                    movement.moveSpeed = speed;
                    movement.direction = Vector2D(dx, dy);
                }
                else {
                    std::cerr << "[ImGuiError] parsing Movement at line " << lineNumber << "\n";
                    continue;
                }
            }

            if (word == "BoxCollider") {
                float sx, sy, ox, oy;
                int trigger = 0;

                if (iss >> sx >> sy >> ox >> oy >> trigger) {
                    entityManager->AddComponent<Framework::BoxCollider>(Entity);
                    auto& boxCollider = entityManager->GetComponent<Framework::BoxCollider>(Entity);
                    boxCollider.size = Vector2D(sx, sy);
                    boxCollider.offset = Vector2D(ox, oy);
                    boxCollider.isTrigger = (trigger != 0);
                }
                else {
                    std::cerr << "[ImGuiError] parsing BoxCollider at line " << lineNumber << "\n";
					continue;
                }
            }

            if (word == "CircleCollider") {
                float radius, ox, oy;
                if (iss >> radius >> ox >> oy) {
					entityManager->AddComponent<Framework::CircleCollider>(Entity);
                    auto& circleCollider = entityManager->GetComponent<Framework::CircleCollider>(Entity);
                    circleCollider.radius = radius;
                    circleCollider.offset = Vector2D(ox, oy);
                }
                else {
                    std::cerr << "[ImGuiError] parsing CircleCollider at line " << lineNumber << "\n";
					continue;
                }
            }

            if (word == "TriangleCollider") {
				float v0x, v0y, v1x, v1y, v2x, v2y;
                if (iss >> v0x >> v0y >> v1x >> v1y >> v2x >> v2y)
                {
                    entityManager->AddComponent<Framework::TriangleCollider>(Entity);
                    auto& triangleCollider = entityManager->GetComponent<Framework::TriangleCollider>(Entity);
                    triangleCollider.v0 = Vector2D(v0x, v0y);
                    triangleCollider.v1 = Vector2D(v1x, v1y);
                    triangleCollider.v2 = Vector2D(v2x, v2y);
                }
                else {
                    std::cerr << "[ImGuiError] parsing TriangleCollider at line " << lineNumber << "\n";
                    continue;
                }

            }
            /*if (type == "player") {
                float x = 0.0f, y = 0.0f;
                if (iss >> x >> y) {
					entitySpawner->SpawnPlayer(Vector2D(x, y));
                }
                else {
					std::cerr << "[ImGuiError] parsing player at line " << lineNumber << "\n";
                }
            }
            else if (type == "enemy") {
                float x = 0.0f, y = 0.0f, spd = 0.05f;
                if (iss >> x >> y ) {
                    if (iss) iss >> spd;
					entitySpawner->SpawnEnemy(Vector2D(x, y), spd);
                }
                else {
					std::cerr << "[ImGuiError] parsing enemy at line " << lineNumber << "\n";
                }
            }
            else if (type == "obstacle") {
                float x = 0.0f, y = 0.0f, sx = 0.5f, sy = 0.5f;

                if (iss >> x >> y >> sx >> sy){
                    entitySpawner->SpawnObstacle(Vector2D(x, y), Vector2D(sx, sy));
                }

                else {
					std::cerr << "[ImGuiError] parsing obstacle at line " << lineNumber << "\n";
                  
                }
            }
            else if (type == "sprite") {
                std::string spriteName;
                float x = 0.0f, y = 0.0f, sx = 1.0f, sy = 1.0f;
                if (iss >> spriteName >> x >> y >> sx >> sy) {
                    entitySpawner->SpawnSprite(spriteName, Vector2D(x, y), Vector2D(sx, sy));
                }
                else {
                    std::cerr << "[ImGuiError] parsing sprite at line " << lineNumber << "\n";
                }
            }
            else {
				std::cerr << "[ImGuiError] Unknown entity type '" << type << "' at line " << lineNumber << "\n";
            }*/
        }
        return true;
    }

    // jiahao
    bool ImGuiSystem::SaveLevelToTxt(const std::string& file) {

        std::ofstream writeFile(file);
        if (!writeFile.is_open()) {
            std::cerr << "[ImGuiError] Could not open file for writing: " << file << "\n";
            return false;
        }

        writeFile << "# Saved from ImGui system\n";

        auto entities = entityManager->GetAllEntities();
        for (const auto& entity : entities) {
            const bool hasAny = entityManager->HasComponent<Transform>(entity)
                || entityManager->HasComponent<Sprite>(entity)
                || entityManager->HasComponent<CircleCollider>(entity)
                || entityManager->HasComponent<TriangleCollider>(entity)
				|| entityManager->HasComponent<BoxCollider>(entity);

            if (!hasAny) {
                continue;
			}

            writeFile << "entity\n";

            if (entityManager->HasComponent<Transform>(entity)) {
				auto& transform = entityManager->GetComponent<Transform>(entity);
                writeFile << "Transform " << transform.position.x << " " << transform.position.y << " "
					<< transform.scale.x << " " << transform.scale.y << "\n";
            }

            if (entityManager->HasComponent<Sprite>(entity)) {
				auto& sprite = entityManager->GetComponent<Sprite>(entity);
                if (!sprite.texturePath.empty()) {
                    writeFile << "Sprite " << sprite.texturePath << "\n";
                }
				
            }

            if (entityManager->HasComponent<Movement>(entity)) {
				auto& movement = entityManager->GetComponent<Movement>(entity);
				writeFile << "Movement " << movement.moveSpeed << " " << movement.direction.x << " " << movement.direction.y << "\n";

            }

            if (entityManager->HasComponent<BoxCollider>(entity)) {
				auto& boxCollider = entityManager->GetComponent<BoxCollider>(entity);
				const int trigger = boxCollider.isTrigger ? 1 : 0;
				writeFile << "BoxCollider " << boxCollider.size.x << " " << boxCollider.size.y << " " << boxCollider.offset.x << " " << boxCollider.offset.y << " " << trigger << "\n";

            }

            if (entityManager->HasComponent<CircleCollider>(entity)) {
				auto& circleCollider = entityManager->GetComponent<CircleCollider>(entity);
				writeFile << "CircleCollider " << circleCollider.radius << " " << circleCollider.offset.x << " " << circleCollider.offset.y << "\n";
            }

            if (entityManager->HasComponent<TriangleCollider>(entity)) {
				auto& triangleCollider = entityManager->GetComponent<TriangleCollider>(entity);
				writeFile << "TriangleCollider " << triangleCollider.v0.x << " " << triangleCollider.v0.y << " "
					      << triangleCollider.v1.x << " " << triangleCollider.v1.y << " "
					      << triangleCollider.v2.x << " " << triangleCollider.v2.y << "\n";
            }
            /*if (entityManager->HasComponent<Transform>(entity)) {
                auto& transform = entityManager->GetComponent<Transform>(entity);
                if (entityManager->HasComponent<CircleCollider>(entity)) {
                    writeFile << "player " << transform.position.x << " " << transform.position.y << "\n";
                }
                else if (entityManager->HasComponent<TriangleCollider>(entity)) {
                    writeFile << "enemy " << transform.position.x << " " << transform.position.y << "\n";
                }
                else if (entityManager->HasComponent<BoxCollider>(entity)) {
                    writeFile << "obstacle " << transform.position.x << " " << transform.position.y << " "
                              << transform.scale.x << " " << transform.scale.y << "\n";
                }
                else if (entityManager->HasComponent<Sprite>(entity)) {
                    auto& sprite = entityManager->GetComponent<Sprite>(entity);
                    writeFile << "sprite " << sprite.texturePath << " "
                              << transform.position.x << " " << transform.position.y << " "
                              << transform.scale.x << " " << transform.scale.y << "\n";
                }
            }*/
			writeFile << "\n";
        }
        writeFile.close();
        return true;
	}

    void ImGuiSystem::Update(float dt)
    {
        frameTime = dt;
        if (entityManager) {
            entityCount = static_cast<int>(entityManager->GetAllEntities().size());
        }

        static bool wantOpenModal = false;
        static bool wantSaveAsModal = false;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);



        // Menu bar
        if (ImGui::BeginMainMenuBar()) {
            //File bar - jiahao
            if (ImGui::BeginMenu("File")) {
                if (CORE->IsPlaying()) {
                    ImGui::BeginDisabled();
                }
                if (ImGui::MenuItem("Open")) {
                    bool isOpen = OpenLevelFromTxt("assets/level1.txt", true);
                    if (!isOpen) {
                        std::cerr << "[ImGuiError] Failed to open level.txt\n";
					}
                    else {
						currentLevelPath = "assets/level1.txt";
                    }
                }

                if (ImGui::MenuItem("Open...")) {
                    if (currentLevelPath.empty()) {
						currentLevelPath = "assets/level1.txt";
                    }
					openPath = currentLevelPath;
                    //ImGui::OpenPopup("Open Level...");
                    wantOpenModal = true;
                }

                if (ImGui::MenuItem("Save")) {
					const std::string path = currentLevelPath.empty() ? "assets/level1.txt" : currentLevelPath;
                    bool isSave = SaveLevelToTxt(path);
                    if (!isSave) {
                        std::cerr << "[ImGuiError] Failed to save level.txt\n";
                    }
                }

                if (ImGui::MenuItem("Save as ...")) {
                    if (currentLevelPath.empty()) {
						currentLevelPath = "assets/level1.txt";
                    }
					openPath = currentLevelPath;
                    //ImGui::OpenPopup("Save Level As...");
					wantSaveAsModal = true;
                }

                if (ImGui::MenuItem("Exit")) {
                    Message quitMsg(Status::Quit);
                    CORE->BroadcastMessage(&quitMsg);
                }

                if (CORE->IsPlaying()) {
                    ImGui::EndDisabled();
                }
                ImGui::EndMenu();
            }

            //windows bar
            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Entity Inspector", nullptr, &showEntityInspector);
                ImGui::MenuItem("Spawner", nullptr, &showSpawner);
                ImGui::MenuItem("Debug Info", nullptr, &showDebug);
                ImGui::MenuItem("ImGui Demo", nullptr, &showDemo);
                ImGui::EndMenu();
            }

            //play/stop editor bar - jiahao
            if (ImGui::BeginMenu("Editor")) {

                if (!CORE->IsPlaying()) {
                    if (ImGui::MenuItem("Play")) {
                        if (!SaveLevelToTxt(defaultLevelPath)) {
							std::cerr << "[ImGuiError] Could not create default setting"
                                      <<defaultLevelPath << "\n";
                        }
                        else {
							CORE->SetPlaying(true);
                        }
                    }
                }

                else {
                    if (ImGui::MenuItem("Stop")) {
						CORE->SetPlaying(false);
						entityManager->ClearAllEntities();
                        OpenLevelFromTxt(defaultLevelPath, true);
                    }
                }
 
                ImGui::EndMenu();
			}

            ImGui::EndMainMenuBar();

        
        }

        if (wantOpenModal) {
            ImGui::OpenPopup("Open Level...");
            wantOpenModal = false;
		}

        if (wantSaveAsModal) {
            ImGui::OpenPopup("Save Level As...");
            wantSaveAsModal = false;
		}


        if (ImGui::BeginPopupModal("Open Level...", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char openBuffer[256] = "";
            static bool openError = false;
			static std::string openErrorMsg = "";

            if (ImGui::IsWindowAppearing()) {
                std::snprintf(openBuffer, sizeof(openBuffer), "%s", openPath.c_str());
                openError = false;
                openErrorMsg.clear();
            }
            ImGui::InputText("Path", openBuffer, sizeof(openBuffer));

            if (openError) { 
                ImGui::Spacing();
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", openErrorMsg.c_str());
                
            }

            if (ImGui::Button("Open")) {
                openPath = openBuffer;
                bool isOpen = OpenLevelFromTxt(openPath, true);
                if (isOpen) {
                    currentLevelPath = openPath;
                    ImGui::CloseCurrentPopup();
                }
                else {
					openError = true;
                    openErrorMsg = "Invalid path or file format: " + std::string(openBuffer);
                }
                
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }


        if (ImGui::BeginPopupModal("Save Level As...", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            
            static char saveBuffer[256] = "";
			static bool saveError = false;  
			static std::string saveErrorMsg = "";

            if (ImGui::IsWindowAppearing())
            {
                std::snprintf(saveBuffer, sizeof(saveBuffer), "%s", openPath.c_str());
				saveError = false;
                saveErrorMsg.clear();
            }

            ImGui::InputText("Path", saveBuffer, sizeof(saveBuffer) );

            if (saveError) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", saveErrorMsg.c_str());
            }

            if (ImGui::Button("Save")) {

                openPath = saveBuffer;

                bool isSave = SaveLevelToTxt(openPath);
                if (isSave) {
                    currentLevelPath = openPath;
					ImGui::CloseCurrentPopup();
                }
                else {
					saveError = true;
					saveErrorMsg = "Could not save to path: " + std::string(saveBuffer);
                }
                
            }

			ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
			ImGui::EndPopup();
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


    void ImGuiSystem::SetAudioSystem(AudioSystem* audio)
    {
        audioSystem = audio;
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

        // FIX: Add ##UniqueID
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
            entitySpawner->SpawnGrid("wireframequad", 16, 20, Vector2D(-0.6f, -0.4f), Vector2D(0.1f, 0.1f));
        }

        if (ImGui::Button("Trigger Audio##Btn8", ImVec2(-1, 0))) {
            if (audioSystem) {
                std::cout << "[DEBUG] AudioSystem exists\n";
                audioSystem->PlaySound("leaves", false);
                std::cout << "[DEBUG] PlaySound called\n";
            }
            else {
                std::cout << "[DEBUG] ERROR: AudioSystem is NULL!\n";
            }
        }

        ImGui::End();
    }

    void ImGuiSystem::ShowDebugWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(250, 250), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(630, 30), ImGuiCond_FirstUseEver);

        //Add unique id
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
        ImGui::BulletText("up down left right: Move editor camera");
        ImGui::BulletText("Key 1: Zoom in");
        ImGui::BulletText("Key 2: Zoom out");
        ImGui::BulletText("key 0: Reset Camera");
        ImGui::BulletText("click above menu Editor->Play to activate play mode");
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