/*
===============================================================================
File:        ImGuiSystem.cpp
Author:      Ethan Ng, Jiahao Zhou, Sim Kah Yan
Email:       n.ethanyongle@digipen.edu, jiahao.zhou@digipen.edu, kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 42%(Ethan), 53%(Jiahao), 5%(kahyan)
-------------------------------------------------------------------------------
ImGui editor/overlay system. Integrates Dear ImGui with GLFW/
OpenGL, draws ImGui editor UI, and bridges runtime actions (play/stop, open/save,
drag–drop, asset browser) to ECS and subsystems.

Responsibilities:
- Initialize/Shutdown ImGui (context, backends) and per-frame begin/end.
- Render main menu bar (File/Windows/Editor) and windows:
  • Entity Inspector, Spawner ,Debug Info ,ImGui Demo ,Assets Browser.
- Level I/O: Open/Save/Save-As of simple TXT format (Transform/Sprite/Colliders).
- File Drag-and-Drop: load txt file as level, spawn sprites for image files
- Editor controls: Play/Stop toggle, default level caching & reload.
- Editor Camera and camera flow: on Play set follow-target to player; on Stop clear follow & reset
  editor camera via GraphicsSystemV2.
- Subsystem hookups: EntityManager, EntitySpawner, AudioSystem (stop all on quit),
  GraphicsSystemV2 (ImGui render + camera helpers).

Controls for:
- Menu → File: Open / Open… / Save / Save As… / Exit
- Menu → Windows: toggle editor panels
- Menu → Editor: Play (when stopped) / Stop (when playing)
- Assets Browser: double-click texture to spawn at origin; drag filename to future
  drop targets; click “<” to go up one folder
- OS Drag-&-Drop onto window:
  • .txt → load level (clears scene if requested
  • .png, .jpg, .jpeg → spawn sprite 

Notes:


Safety:

*/

#include "Precompiled.h"
#include "ImGuiSystem.h"
#include "EntitySpawner.h"
#include "AudioSystem.h"
#include "Pathfinding.h"
#include "PrefabSerializer.h"
#include "PrefabTracker.h"

namespace Framework {

    ImGuiSystem::ImGuiSystem()
        : window(nullptr)
        , entityManager(nullptr)
        , entitySpawner(nullptr)
        , audioSystem(nullptr)
        , showDemo(false)
        , showEntityInspector(true)
        , playerEntity(INVALID_ENTITY)
        , showSpawner(true)
        , showDebug(true)
        , enabled(false)
        , frameTime(0.0f)
        , entityCount(0)
        , showAssets(true)
        , graphicsSystem(nullptr)
        , showAudioErrorPopup(false)        
        , audioErrorMessage("")
        , showPrefabWindow(true)
        , selectedEntity{}
        , selectedPrefabPath("")
    {
    }

    void ImGuiSystem::Shutdown()
    {
        std::cout << "[ImGui] Shutting down...\n";

        // Stop all audio if audio system exists
        if (audioSystem) {
            std::cout << "[ImGui] Stopping all audio...\n";
            audioSystem->StopAllSounds();
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        DeleteViewportFramebuffer();
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
        io.IniFilename = "./assets/imgui.ini";  

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        EnableFileDragAndDrop();

        CreateViewportFramebuffer(1280, 720);
        showGameViewport = true;
        renderToViewport = true;
        std::cout << "[ImGuiSystem] Viewport ready - FBO: " << viewportFBO
            << ", Texture: " << viewportTexture << "\n";
    }

    // ============================================================================
	// This is the function that able to open level from a txt file
    // author: jiahao.zhou@digipen
    // ============================================================================
    bool ImGuiSystem::OpenLevelFromTxt(const std::string& file, bool clearAll) {

		// this is to declare a input file stream called readFile and open the file
        std::ifstream readFile(file);

		// check if the file is open, display error message if not
        if (!readFile.is_open()) {
            std::cerr << "[ImGuiError] Could not open file for reading: " << file << "\n";
            return false;
        }
		// check if entity manager is valid
        if (!entityManager) {
            std::cerr << "[ImGuiError] ImGuiSystem missing managers for loading.\n";
            return false;
        }
		// clear all existing entities if clearAll is true
        if (clearAll) {
            entityManager->ClearAllEntities();
            entityManager->ResetEntityIDCounter();
        }
		// declare a variable to hold the entity being created
        Framework::Entity Entity;
		// a boolean to check if an entity is created
        bool hasEntity = false;
		// read the file line by line
        std::string line;
        
        auto lineNumber = 0;
		// loop through each line in the file
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
			// check if the word is "entity", if so create a new entity
            if (word == "entity") {
                Entity = entityManager->CreateEntity();
                hasEntity = true;
                continue;
            }

            if (!hasEntity) {
                continue;
            }
			// if else condition to check which component to add to the entity
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
                    //sprite.texturePath =  name;
                    std::string label = std::filesystem::path(name).filename().string();
                    sprite.texturePath = std::string("assets/") + label;
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
            if (word == "SpriteAnimation") {
                int rows = 0;
                int columns = 0;
				int frameCount = 0;
                float frameTime = 0.0f;
                int loopInt = 1;
                float uvShrinkPx = 0.0f;

                if (iss >> rows >> columns >> frameCount >> frameTime >> loopInt >> uvShrinkPx) {
                    entityManager->AddComponent<Framework::SpriteAnimation>(Entity);
                    auto& spriteAnimation = entityManager->GetComponent<Framework::SpriteAnimation>(Entity);
                    spriteAnimation.rows = rows;
					spriteAnimation.columns = columns;
					spriteAnimation.frameCount = frameCount;
					spriteAnimation.frameTime = frameTime;
					spriteAnimation.loop = (loopInt != 0);
					spriteAnimation.uvShrinkPx = uvShrinkPx;

					spriteAnimation.playing = true;
					spriteAnimation.currentFrame = 0;

                    if (graphicsSystem && entityManager->HasComponent<Framework::Sprite>(Entity)) {
                        auto& sprite = entityManager->GetComponent<Framework::Sprite>(Entity);
                        if (!sprite.texturePath.empty()) {
                            auto& rm = graphicsSystem->GetResourceManager();
                            spriteAnimation.spriteSheet = rm.LoadTexture(sprite.texturePath);

                            if (auto* texture = rm.GetTexture(spriteAnimation.spriteSheet)) {
                                int colsForSize = (columns > 0) ? columns : 1;
								int rowsForSize = (rows > 0) ? rows : 1;

                                spriteAnimation.frameWidth = texture->GetWidth() / colsForSize;
								spriteAnimation.frameHeight = texture->GetHeight() / rowsForSize;
                            }
                        }
                    }
                    else {
                        std::cerr << "[ImGuiError] parsing SpriteAnimation at line " << lineNumber << "\n";
                        continue;
                    }
                    continue;
                }
            }

            
        }
        RebuildSpatialPartition();
        return true;
    }

    // ============================================================================
    // This is the function that able to save level from a txt file
    // author: jiahao zhou 
    // ============================================================================
    bool ImGuiSystem::SaveLevelToTxt(const std::string& file) {

		//this is to declare a output file stream called writeFile and open the file
        std::ofstream writeFile(file);

		//check if the file is open, display error message if not
        if (!writeFile.is_open()) {
            std::cerr << "[ImGuiError] Could not open file for writing: " << file << "\n";
            return false;
        }

		//write title line
        writeFile << "# Saved from ImGui system\n";

        //get all entities from ECS manager
        auto entities = entityManager->GetAllEntities();

		//Create a loop to go through all entities
        for (const auto& entity : entities) {
			//check if the entity has any of the components we need to write in the file
            const bool hasAny = entityManager->HasComponent<Transform>(entity)
                || entityManager->HasComponent<Sprite>(entity)
                || entityManager->HasComponent<CircleCollider>(entity)
                || entityManager->HasComponent<BoxCollider>(entity)
                || entityManager->HasComponent<SpriteAnimation>(entity);

            //if the component doesnt have any entity, skip this entity
            if (!hasAny) {
                continue;
            }

            //write entity whenever there is a new entity
            writeFile << "entity\n";

			//if else condition to check which component the entity has and write the corresponding data to the file

			//this if else condition is to check if entity has transform component
            if (entityManager->HasComponent<Transform>(entity)) {
				//get reference to the transform component
                auto& transform = entityManager->GetComponent<Transform>(entity);
                //write position (x,y) and the scale(x,y)
                writeFile << "Transform " << transform.position.x << " " << transform.position.y << " "
                          << transform.scale.x << " " << transform.scale.y << "\n";
            }

			//this if else condition is to check if entity has sprite component
            if (entityManager->HasComponent<MeshRenderer>(entity)) {
				// get reference to the meshRenderer component
                // in renderring system, there is also meshrenderer component to generate image
                // some entity may not have 
				auto& meshRenderer = entityManager->GetComponent<MeshRenderer>(entity);
				//write the sprite name
                if (!meshRenderer.spriteName.empty()) {
					writeFile << "Sprite " << meshRenderer.spriteName << "\n";
                }
            }

			// this if else condition is to check if entity has sprite component
            if (entityManager->HasComponent<Sprite>(entity)) {
				// get reference to the sprite component
                auto& sprite = entityManager->GetComponent<Sprite>(entity);
                if (!sprite.texturePath.empty()) {
                    writeFile << "Sprite " << sprite.texturePath << "\n";
                }

            }

			// this if else condition is to check if entity has movement component
            if (entityManager->HasComponent<Movement>(entity)) {
				// get reference to the movement component
                auto& movement = entityManager->GetComponent<Movement>(entity);
				//write the movement speed and direction (x,y)
                writeFile << "Movement " << movement.moveSpeed << " " << movement.direction.x << " " << movement.direction.y << "\n";

            }

			// this if else condition is to check if entity has boxcollider component
            if (entityManager->HasComponent<BoxCollider>(entity)) {
				// get reference to the boxcollider component
                auto& boxCollider = entityManager->GetComponent<BoxCollider>(entity);
				//box collider trigger is bool, we need to convert it to int(0/1) for writing
                const int trigger = boxCollider.isTrigger ? 1 : 0;
				//write the size (x,y), offset(x,y) and trigger(0/1)
                writeFile << "BoxCollider " << boxCollider.size.x << " " << boxCollider.size.y << " " << boxCollider.offset.x << " " << boxCollider.offset.y << " " << trigger << "\n";

            }

			// this if else condition is to check if entity has circlecollider component
            if (entityManager->HasComponent<CircleCollider>(entity)) {
				// get reference to the circlecollider component
                auto& circleCollider = entityManager->GetComponent<CircleCollider>(entity);
				//write the radius and offset(x,y)
                writeFile << "CircleCollider " << circleCollider.radius << " " << circleCollider.offset.x << " " << circleCollider.offset.y << "\n";
            }

            if (entityManager->HasComponent<SpriteAnimation>(entity)) {
                auto& spriteAnimation = entityManager->GetComponent<SpriteAnimation>(entity);
                const int loopInt = spriteAnimation.loop ? 1 : 0;
                writeFile << "SpriteAnimation " << spriteAnimation.rows << " " << spriteAnimation.columns << " "
                          << spriteAnimation.frameCount << " " << spriteAnimation.frameTime << " "
                          << loopInt << " " << spriteAnimation.uvShrinkPx << "\n";
			}

			// save audio source component
            if (entityManager->HasComponent<AudioSource>(entity)) {
                auto& audio = entityManager->GetComponent<AudioSource>(entity);
                writeFile << "AudioSource " << audio.soundName << " "
                    << audio.volume << " " << audio.pitch << " "
                    << (audio.loop ? 1 : 0) << " "
                    << (audio.playOnStart ? 1 : 0) << "\n";
            }

            //save script component
            if (entityManager->HasComponent<ScriptComponent>(entity)) {
                auto& script = entityManager->GetComponent<ScriptComponent>(entity);
                if (!script.scriptPath.empty()) {
                    writeFile << "Script " << script.scriptPath << "\n";
                }
            }
            // add this blank line to separate this entity from the next entity
            writeFile << "\n";
        }
		//Close the file after writing
        writeFile.close();
        //if the save function works, return true
        return true;
    }

    // ============================================================================
	// This is the function to turn on drag and drop for files in GLFW window
    // author: jiahao zhou 
    // ============================================================================
    void ImGuiSystem::EnableFileDragAndDrop() {
		//attack the current imguisystem instance to the window user pointer
		// so the drop callback can find the instance
		glfwSetWindowUserPointer(window, this);

		//give instructions to GLFW to use the FileDropCallBack function
		glfwSetDropCallback(window, FileDropCallBack);
    }


    // ============================================================================
	// This is the function of GLFW file drop callback
    // author: jiahao zhou 
    // ============================================================================
    void ImGuiSystem::FileDropCallBack(GLFWwindow* window, int count, const char** paths) {
		//get the imguisystem instance from the window user pointer
        Framework::ImGuiSystem* self = static_cast<Framework::ImGuiSystem*>(glfwGetWindowUserPointer(window));
		// if the instance is valid, call the OnFileDrop member function
        if (self) {
            self->OnFileDrop(count, paths);
        }
    }

    // ============================================================================
	// This is the function to check if the file is a level file
    // author: jiahao.zhou@digipen
    // ============================================================================
    bool ImGuiSystem::IsLevelFile(const std::filesystem::path& path) const {
		// return true only if the file has .txt extension
		return path.has_extension() && path.extension() == ".txt";
    }

    // ============================================================================
	// This is the function to check if the file is a texture file(jpg/png/jpeg)
    // author: jiahao.zhou@digipen
    // ============================================================================
    bool ImGuiSystem::IsTextureFile(const std::filesystem::path& path) const {
		//first check if the file has extension, if not return false
        if (!path.has_extension()) {
            return false;
        }
        //get the file extension string 
		auto ext = path.extension().string();
		//convert it to lower case for easier comparison
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		//return true if the extension is png/jpg/jpeg
		return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
    }


    // ============================================================================
	// This is the function that show the asset window
    // author: jiahao.zhou@digipen
    // ============================================================================
    void ImGuiSystem::ShowAssetsWindow() {
        // Begin always needs an End, regardless of return value
        if (!ImGui::Begin("Assets##Assets", &showAssets))
        {
            ImGui::End();  // Still need to call End even when collapsed
            return;        // But return early - don't draw content
        }

        // Draw separator and back button
        ImGui::SeparatorText(currentpath.string().c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10);

        if (ImGui::Button("<##Back")) {
            if (currentpath != rootpath) {
                if (!previouspath.empty()) {
                    currentpath = previouspath;
                    previouspath = currentpath.parent_path();
                }
            }
        }

        // Loop through directory entries
        for (auto const& e : std::filesystem::directory_iterator(currentpath)) {
            auto const path = e.path();
            std::string const label = path.filename().string();
            std::string const ImGuilabel = e.is_directory() ? "->" + label : label;
            std::string filePath = "assets/" + label;

            if (ImGui::Selectable(ImGuilabel.c_str())) {
                if (e.is_directory()) {
                    previouspath = currentpath;
                    currentpath = e;
                }
                else if (IsLevelFile(path)) {
                    OpenLevelFromTxt(filePath, true);
                }
            }

            // Texture drag-drop
            if (IsTextureFile(path))
            {
                if (ImGui::IsItemHovered()) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        if (entitySpawner) {
                            Framework::Entity entity = entitySpawner->SpawnSprite(
                                filePath,
                                Vector2D(0.0f, 0.0f),
                                Vector2D(1.0f, 1.0f)
                            );
                            std::cout << "[Drop] Spawned sprite from: " << filePath
                                << " as entity" << entity.id << "\n";
                        }
                    }
                }

                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("Sprite", &label, label.size());
                    ImGui::Text(label.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            if (path.extension() == ".prefab" || path.extension() == ".json") {
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Double-click to spawn prefab\nDrag to drop zone");

                    // Double-click to spawn
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        std::string prefabPath = path.string();
                        Entity newEntity = PrefabSerializer::LoadPrefab(*entityManager, prefabPath);

                        if (newEntity.IsValid()) {
                            std::cout << "[Assets] Spawned prefab: " << label
                                << " as entity " << newEntity.GetID() << "\n";
                        }
                    }
                }

                // Drag-drop source for prefabs
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    std::string prefabPath = path.string();
                    ImGui::SetDragDropPayload("Prefab", prefabPath.c_str(), prefabPath.size() + 1);
                    ImGui::Text("📦 %s", label.c_str());
                    ImGui::EndDragDropSource();
                }
            }
        }

        ImGui::End();  // Only one End() call at the very end
    }

    Framework::Vector2D EditorScreenWorld(float screenX, float screenY) {
        if (!Framework::CORE) {
			return Framework::Vector2D{0.0f, 0.0f};
        }

		auto graphics = Framework::CORE->GetGraphicsSystem();
		auto windowSystem = Framework::CORE->GetWindowSystem();

        if (!graphics || !windowSystem) {
            return Framework::Vector2D(0.0f, 0.0f);
        }

		GLFWwindow* window = windowSystem->GetWindow();
        if (!window) {
            return Framework::Vector2D(0.0f, 0.0f);
        }

		int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        if (fbWidth == 0 || fbHeight == 0 || windowWidth == 0 || windowHeight == 0) {
            return Framework::Vector2D(0.0f, 0.0f);
        }

        float fbX = screenX * static_cast<float>(fbWidth) / static_cast<float>(windowWidth);
        float fbY = screenY * static_cast<float>(fbHeight) / static_cast<float>(windowHeight);

        float ndcX = (2.0f * fbX / static_cast<float>(fbWidth)) - 1.0f;
        float ndcY = 1.0f - (2.0f * fbY / static_cast<float>(fbHeight));

        glm::mat4 invViewProj = glm::inverse(graphics->GetCamera().GetViewProjectionMatrix());

        glm::vec4 worldPos = invViewProj * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);

        return Vector2D(worldPos.x, worldPos.y);
    }

    void ImGuiSystem::NewFrame()
    {
        if (!enabled) {
            return;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        frameTime = ImGui::GetIO().DeltaTime;

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode);
    }

    void ImGuiSystem::Update(float dt)
    {
        (void)dt;
        if (pendingToggle) {
            enabled = !enabled;
            pendingToggle = false;
            std::cout << "[ImGuiSystem] Toggled to: " << (enabled ? "ON" : "OFF") << "\n";
        }


        if (!enabled) {
            return;
        }
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        //frameTime = dt;
        if (entityManager) {
            entityCount = static_cast<int>(entityManager->GetAllEntities().size());
        }

        static bool wantOpenModal = false;
        static bool wantSaveAsModal = false;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        frameTime = ImGui::GetIO().DeltaTime;

        //object picking
        UpdatePicking();

        UpdateEntityDragging();
        InputSystem* input = Framework::CORE->GetInputSystem();
        if (input) {
            // Check if either Left Control or Right Control is being held down
            bool isCtrlHeld = input->IsKeyDown(KEY_LEFT_CONTROL) || input->IsKeyDown(KEY_RIGHT_CONTROL);

            // Check if the 'Z' key was just pressed this frame
            bool isZPressed = input->IsKeyPressed(KEY_Z);

            // If both Ctrl and Z are active, trigger the Undo
            if (isCtrlHeld && isZPressed) {
                std::cout << "[Editor] Ctrl+Z pressed - Attempting Undo...\n";
                PerformUndo();
            }
        }

        //delete button to delete selected entity
        if (!CORE->IsPlaying() && entityManager) {
            InputSystem* input = Framework::CORE->GetInputSystem();

            if (input && selectedEntity.IsValid()) {
                if (input->IsKeyPressed(KEY_DELETE))
                {
                    std::cout << "[ImGui] Delete key pressed on entity "
                        << selectedEntity.id << "\n";

                    SpatialPartitioningRemove(selectedEntity);

                    entityManager->DestroyEntity(selectedEntity);


                    selectedEntity = Framework::Entity{};
                    draggingEntity = Framework::Entity{};
                    isDraggingEntity = false;
                }
            }

        }

        // Draw Menu bar
        if (ImGui::BeginMainMenuBar()) {
            //File bar - jiahao
            if (ImGui::BeginMenu("File")) {
                // if the game is playing, function buttons under File should not be available
                if (CORE->IsPlaying()) {
                    ImGui::BeginDisabled();
                }
                // open default level
                if (ImGui::MenuItem("Open")) {
                    // open default level if no current level path
                    bool isOpen = OpenLevelFromTxt("assets/level1.txt", true);
                    if (!isOpen) {
                        std::cerr << "[ImGuiError] Failed to open level.txt\n";
                    }
                    else {
                        currentLevelPath = "assets/level1.txt";
                    }
                }
                // open level from specified path input by user
                if (ImGui::MenuItem("Open...")) {
                    // set default path if current level path is empty
                    if (currentLevelPath.empty()) {
                        currentLevelPath = "assets/level1.txt";
                    }
                    // set the open path to current level path
                    openPath = currentLevelPath;
                    //ImGui::OpenPopup("Open Level...");
                    // set a flag here to show open modal in next frame
                    wantOpenModal = true;
                }

                // save level to current level path
                if (ImGui::MenuItem("Save")) {
                    // use default path if current level path is empty
                    const std::string path = currentLevelPath.empty() ? "assets/level1.txt" : currentLevelPath;
                    // try to save level to the path
                    bool isSave = SaveLevelToTxt(path);
                    if (!isSave) {
                        std::cerr << "[ImGuiError] Failed to save level.txt\n";
                    }
                }

                // save level to specified path input by user
                if (ImGui::MenuItem("Save as ...")) {
                    // set default path if current level path is empty
                    if (currentLevelPath.empty()) {
                        currentLevelPath = "assets/level1.txt";
                    }
                    // set the open path to current level path
                    openPath = currentLevelPath;
                    //ImGui::OpenPopup("Save Level As...");
                    // set a flag here to show save as modal in next frame
                    wantSaveAsModal = true;
                }

                // exit option
                if (ImGui::MenuItem("Exit")) {
                    Message quitMsg(Status::Quit);
                    CORE->BroadcastMessage(&quitMsg);
                }

                // disable function buttons under File when playing
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
                //Asset window - jiahao
                ImGui::MenuItem("Assets", nullptr, &showAssets);
                //prefab window - kahyan
                ImGui::MenuItem("Prefabs", nullptr, &showPrefabWindow);
                ImGui::MenuItem("Game Viewport", nullptr, &showGameViewport);  // ADD
                ImGui::Separator();
                ImGui::MenuItem("Render to Viewport", nullptr, &renderToViewport);
                ImGui::EndMenu();
            }

            // ============================================================================
            // This is the if else condition to control play and stop button in the editor menu
            // author: jiahao.zhou@digipen
            // ============================================================================
            if (ImGui::BeginMenu("Editor")) {

                // if the game is not playing, show play button
                if (!CORE->IsPlaying()) {
                    if (ImGui::MenuItem("Play")) {
                        //if there is no current level path, save to default level path
                        if (!SaveLevelToTxt(defaultLevelPath)) {
                            std::cerr << "[ImGuiError] Could not create default setting"
                                << defaultLevelPath << "\n";
                        }
                        else {
                            // change engine state into playing
                            CORE->SetPlaying(true);
                            // set camera to follow player entity
                            if (auto* gfx = CORE->GetGraphicsSystem())
                            {
                                if (entityManager)
                                {
                                    //find player entity by checking circle collider radius
                                    Framework::Entity player{};
                                    for (auto e : entityManager->GetAllEntities())
                                    {
                                        if (entityManager->HasComponent<Framework::CircleCollider>(e))
                                        {
                                            // get reference to circle collider component
                                            auto& c = entityManager->GetComponent<Framework::CircleCollider>(e);
                                            if (c.radius > 0.12f && c.radius < 0.18f)
                                            {
                                                // mark this entity as player
                                                player = e;
                                                break;
                                            }
                                        }
                                    }

                                    // set follow target if player entity is valid
                                    if (player.IsValid())
                                    {
                                        // inform graphics system to follow player
                                        gfx->SetFollowTarget(player);
                                    }
                                }
                            }
                        }

                    }
                }

                //if the game is playing, show stop button
                else {
                    if (ImGui::MenuItem("Stop")) {
                        CORE->SetPlaying(false);

                        //Reset Camera
                        if (auto gfx = CORE->GetGraphicsSystem()) {
                            gfx->ClearFollowTarget();
                            gfx->ResetEditorCamera();
                        }

                        //Reload default level
                        if (!OpenLevelFromTxt(defaultLevelPath, true)) {
                            if (!currentLevelPath.empty()) {
                                OpenLevelFromTxt(currentLevelPath, true);
                            }
                            else {
                                std::cerr << "[ImGuiError] Could not create default setting " << defaultLevelPath << "\n";
                                OpenLevelFromTxt("assets/level1.txt", true);
                            }
                        }

                        //clear all entities and reload default level
                        entityManager->ClearAllEntities();
                        // reload default level
                        OpenLevelFromTxt(defaultLevelPath, true);
                    }
                }

                //close the editor menu
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();


        }

        // ============================================================================
        // This is the if else condition to control open and save as modal windows
        // by asking ASC TAs and online research, the ImGUi::BgeginPopupModal takes in char array
        // std::string will cause errors
        // author: jiahao.zhou@digipen
        // ============================================================================
        //if the flag is set to open modal, open the modal and reset the flag
        if (wantOpenModal) {
            // open the modal popup
            ImGui::OpenPopup("Open Level...");
            // reset the flag so it only open once
            wantOpenModal = false;
        }

        // same as save level as modal
        if (wantSaveAsModal) {
            ImGui::OpenPopup("Save Level As...");
            wantSaveAsModal = false;
        }

        // open level modal window. auto resize to fit content
        if (ImGui::BeginPopupModal("Open Level...", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            // according research, openbuffer must be char array
            // when I initially tried std::string, it caused a lot of errors
            static char openBuffer[256] = "";
            static bool openError = false;
            static std::string openErrorMsg = "";

            // when the window first appears, initialize the openBuffer with openPath
            if (ImGui::IsWindowAppearing()) {
                std::snprintf(openBuffer, sizeof(openBuffer), "%s", openPath.c_str());
                openError = false;
                openErrorMsg.clear();
            }
            // input text box for user to enter path
            ImGui::InputText("Path", openBuffer, sizeof(openBuffer));

            if (openError) {
                // add spacing
                ImGui::Spacing();
                // display error message in red color
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", openErrorMsg.c_str());

            }

            // open button
            if (ImGui::Button("Open")) {
                // set openPath to user input path
                openPath = openBuffer;
                // try to open level from the path
                bool isOpen = OpenLevelFromTxt(openPath, true);
                if (isOpen) {
                    // if opened successfully, record the current level path
                    currentLevelPath = openPath;
                    // close the modal
                    ImGui::CloseCurrentPopup();
                }
                else {
                    openError = true;
                    openErrorMsg = "Invalid path or file format: " + std::string(openBuffer);
                }

            }

            // put cancel button on the same line as open button
            ImGui::SameLine();

            // cancel button
            if (ImGui::Button("Cancel")) {

                // close the modal
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // save level as modal window. auto resize to fit content
        if (ImGui::BeginPopupModal("Save Level As...", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

            // same as open modal, use char array for saveBuffer
            static char saveBuffer[256] = "";
            static bool saveError = false;
            static std::string saveErrorMsg = "";

            if (ImGui::IsWindowAppearing())
            {
                std::snprintf(saveBuffer, sizeof(saveBuffer), "%s", openPath.c_str());
                saveError = false;
                saveErrorMsg.clear();
            }

            ImGui::InputText("Path", saveBuffer, sizeof(saveBuffer));

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

        if (showAudioErrorPopup) {
            ImGui::OpenPopup("Audio Format Error##AudioError");
            showAudioErrorPopup = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(550.0f, 350.0f), ImGuiCond_Appearing);
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));

        if (ImGui::BeginPopupModal("Audio Format Error##AudioError", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {

            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "⚠️ ERROR");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextWrapped("%s", audioErrorMessage.c_str());
            ImGui::Spacing();
            ImGui::Separator();

            float buttonWidth = 120.0f;
            ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f);

            if (ImGui::Button("OK##AudioErrorOK", ImVec2(buttonWidth, 0))) {
                audioErrorMessage.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(2);

        // Show windows
        if (showEntityInspector) ShowEntityInspector();
        if (showSpawner) ShowSpawnerWindow();
        if (showDebug) ShowDebugWindow();
        if (showDemo) ImGui::ShowDemoWindow(&showDemo);
        // show asset window - jiahao
        if (showAssets) ShowAssetsWindow();
        // show prefab window - kahyan
        if (showPrefabWindow) ShowPrefabWindow();
        if (showGameViewport) ShowGameViewport();
    }

    void ImGuiSystem::Render()
    {
        if (!enabled) {
            return;
        }

        //if (!window) return;

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

    void ImGuiSystem::SetGraphicsSystem(GraphicsSystemV2* graphics)
    {
        graphicsSystem = graphics;
    }

    // ============================================================================
    // UI WINDOWS - WITH ## UNIQUE IDS
    // ============================================================================

    void ImGuiSystem::ShowEntityInspector()
    {
        if (!entityManager) return;

        if (!ImGui::Begin("Entity Inspector##Inspector1", &showEntityInspector)) {
            ImGui::End();
            return;
        }

        std::vector<Entity> allEntities = entityManager->GetAllEntities();
        const int totalEntities = static_cast<int>(allEntities.size());

        ImGui::Text("Total Entities: %d", totalEntities);
        ImGui::Separator();

        // ========================================================================
        // PAGINATION CONTROLS
        // ========================================================================
        const int totalPages = (totalEntities + entitiesPerPage - 1) / entitiesPerPage;

        if (currentPage < 0) currentPage = 0;
        if (currentPage >= totalPages && totalPages > 0) currentPage = totalPages - 1;

        if (totalPages > 1) {
            ImGui::Text("Page %d / %d", currentPage + 1, totalPages);

            if (ImGui::Button("< Prev##PagePrev")) {
                if (currentPage > 0) currentPage--;
            }
            ImGui::SameLine();
            if (ImGui::Button("Next >##PageNext")) {
                if (currentPage < totalPages - 1) currentPage++;
            }
            ImGui::Separator();
        }

        // Calculate page slice
        const int startIdx = currentPage * entitiesPerPage;
        int endIdx = startIdx + entitiesPerPage;
        if (endIdx > totalEntities) {
            endIdx = totalEntities;
        }

        std::vector<Entity> pageEntities;
        if (startIdx < totalEntities) {
            pageEntities.assign(
                allEntities.begin() + startIdx,
                allEntities.begin() + endIdx
            );
        }

        ImGui::Text("Showing %d - %d of %d", startIdx + 1, endIdx, totalEntities);
        ImGui::Separator();

        // ========================================================================
        // DISPLAY ENTITIES
        // ========================================================================
        Entity entityToDelete = { 0 };
        bool shouldDelete = false;

        for (size_t i = 0; i < pageEntities.size(); ++i) {
            Entity entity = pageEntities[i];

            ImGui::PushID(static_cast<int>(entity.GetID()));

            // Create label with entity info
            char label[256];
            std::string entityInfo = "";

            // Add sprite name if available
            if (entityManager->HasComponent<MeshRenderer>(entity)) {
                auto& mr = entityManager->GetComponent<MeshRenderer>(entity);
                if (!mr.spriteName.empty()) {
                    entityInfo = " (" + mr.spriteName + ")";
                }
            }

            snprintf(label, sizeof(label), "Entity %u%s", entity.GetID(), entityInfo.c_str());

            if (ImGui::CollapsingHeader(label)) {

                // ==================================================================
                // TRANSFORM COMPONENT
                // ==================================================================
                if (entityManager->HasComponent<Transform>(entity)) {
                    if (ImGui::TreeNode("Transform##TransformNode")) {
                        auto& transform = entityManager->GetComponent<Transform>(entity);

                        ImGui::DragFloat2("Position", &transform.position.x, 0.01f, -100.0f, 100.0f);
                        ImGui::DragFloat2("Scale", &transform.scale.x, 0.01f, 0.01f, 100.0f);
                        ImGui::DragFloat("Rotation", &transform.rotation, 0.1f, -360.0f, 360.0f);

                        ImGui::TreePop();
                    }
                }

                // ==================================================================
                // SPRITE COMPONENT
                // ==================================================================
                if (entityManager->HasComponent<Sprite>(entity)) {
                    if (ImGui::TreeNode("Sprite##SpriteNode")) {
                        auto& sprite = entityManager->GetComponent<Sprite>(entity);

                        // Display texture path
                        char pathBuffer[256];
                        strncpy(pathBuffer, sprite.texturePath.c_str(), sizeof(pathBuffer) - 1);
                        pathBuffer[sizeof(pathBuffer) - 1] = '\0';

                        if (ImGui::InputText("Texture Path", pathBuffer, sizeof(pathBuffer))) {
                            sprite.texturePath = pathBuffer;
                        }

                        ImGui::DragInt("Layer", &sprite.layer, 1, -100, 100);

                        ImGui::TreePop();
                    }
                }

                // ==================================================================
                // MESH RENDERER COMPONENT
                // ==================================================================
                if (entityManager->HasComponent<MeshRenderer>(entity)) {
                    if (ImGui::TreeNode("MeshRenderer##MeshRendererNode")) {
                        auto& meshRenderer = entityManager->GetComponent<MeshRenderer>(entity);

                        // Sprite name
                        char nameBuffer[256];
                        strncpy(nameBuffer, meshRenderer.spriteName.c_str(), sizeof(nameBuffer) - 1);
                        nameBuffer[sizeof(nameBuffer) - 1] = '\0';

                        if (ImGui::InputText("Sprite Name", nameBuffer, sizeof(nameBuffer))) {
                            meshRenderer.spriteName = nameBuffer;
                        }

                        ImGui::DragInt("Layer", &meshRenderer.layer, 1, -100, 100);
                        ImGui::DragInt("Order in Layer", &meshRenderer.orderInLayer, 1, -100, 100);

                        // Tint color
                        float tint[4] = { meshRenderer.tint.r, meshRenderer.tint.g,
                                          meshRenderer.tint.b, meshRenderer.tint.a };
                        if (ImGui::ColorEdit4("Tint", tint)) {
                            meshRenderer.tint.r = tint[0];
                            meshRenderer.tint.g = tint[1];
                            meshRenderer.tint.b = tint[2];
                            meshRenderer.tint.a = tint[3];
                        }

                        ImGui::TreePop();
                    }
                }

                // ==================================================================
                // MOVEMENT COMPONENT
                // ==================================================================
                if (entityManager->HasComponent<Movement>(entity)) {
                    if (ImGui::TreeNode("Movement##MovementNode")) {
                        auto& movement = entityManager->GetComponent<Movement>(entity);

                        ImGui::DragFloat("Speed", &movement.moveSpeed, 0.01f, 0.0f, 100.0f);
                        ImGui::DragFloat2("Direction", &movement.direction.x, 0.01f, -1.0f, 1.0f);

                        ImGui::TreePop();
                    }
                }

                // ==================================================================
                // BOX COLLIDER COMPONENT
                // ==================================================================
                if (entityManager->HasComponent<BoxCollider>(entity)) {
                    if (ImGui::TreeNode("BoxCollider##BoxColliderNode")) {
                        auto& boxCollider = entityManager->GetComponent<BoxCollider>(entity);

                        ImGui::DragFloat2("Size", &boxCollider.size.x, 0.01f, 0.0f, 100.0f);
                        ImGui::DragFloat2("Offset", &boxCollider.offset.x, 0.01f, -100.0f, 100.0f);
                        ImGui::Checkbox("Is Trigger", &boxCollider.isTrigger);

                        ImGui::TreePop();
                    }
                }

                // ==================================================================
                // CIRCLE COLLIDER COMPONENT
                // ==================================================================
                if (entityManager->HasComponent<CircleCollider>(entity)) {
                    if (ImGui::TreeNode("CircleCollider##CircleColliderNode")) {
                        auto& circleCollider = entityManager->GetComponent<CircleCollider>(entity);

                        ImGui::DragFloat("Radius", &circleCollider.radius, 0.01f, 0.0f, 100.0f);
                        ImGui::DragFloat2("Offset", &circleCollider.offset.x, 0.01f, -100.0f, 100.0f);

                        ImGui::TreePop();
                    }
                }

                // ==================================================================
                // SPRITE ANIMATION COMPONENT
                // ==================================================================
                //if (entityManager->HasComponent<SpriteAnimation>(entity)) {
                // 
                //    if (ImGui::TreeNode("SpriteAnimation##SpriteAnimNode")) {
                //        auto& anim = entityManager->GetComponent<SpriteAnimation>(entity);

                //        ImGui::DragInt("Frame Width", &anim.frameWidth, 1, 1, 1024);
                //        ImGui::DragInt("Frame Height", &anim.frameHeight, 1, 1, 1024);
                //        ImGui::DragInt("Frame Count", &anim.frameCount, 1, 1, 100);
                //        ImGui::DragInt("Current Frame", &anim.currentFrame, 1, 0, anim.frameCount - 1);
                //        ImGui::DragFloat("Frame Time", &anim.frameTime, 0.01f, 0.0f, 10.0f);
                //        ImGui::DragFloat("Elapsed", &anim.elapsed, 0.01f, 0.0f, 100.0f);
                //        ImGui::Checkbox("Flip X", &anim.flipX);

                //        ImGui::TreePop();
                //    }
                //}

                // ==================================================================
                // AUDIO SOURCE COMPONENT (if you have one)
                // ==================================================================
                if (entityManager->HasComponent<AudioSource>(entity)) {
                    if (ImGui::TreeNode("AudioSource##AudioSourceNode")) {
                        auto& audio = entityManager->GetComponent<AudioSource>(entity);

                        // Display audio name
                        char audioBuffer[256];
                        strncpy(audioBuffer, audio.soundName.c_str(), sizeof(audioBuffer) - 1);
                        audioBuffer[sizeof(audioBuffer) - 1] = '\0';

                        if (ImGui::InputText("Sound Name", audioBuffer, sizeof(audioBuffer))) {
                            audio.soundName = audioBuffer;
                        }

                        ImGui::DragFloat("Volume", &audio.volume, 0.01f, 0.0f, 1.0f);
                        ImGui::DragFloat("Pitch", &audio.pitch, 0.01f, 0.1f, 3.0f);
                        ImGui::Checkbox("Loop", &audio.loop);
                        ImGui::Checkbox("Play on Start", &audio.playOnStart);

                        ImGui::TreePop();
                    }
                }

                // ==================================================================
                // SCRIPT COMPONENT
                // ==================================================================
                if (entityManager->HasComponent<ScriptComponent>(entity)) {
                    if (ImGui::TreeNode("Script##ScriptNode")) {
                        auto& script = entityManager->GetComponent<ScriptComponent>(entity);

                        // Display script path
                        ImGui::Text("Path: %s", script.scriptPath.c_str());

                        // Extract and show script name
                        std::string scriptName = std::filesystem::path(script.scriptPath).stem().string();
                        ImGui::Text("Name: %s", scriptName.c_str());

                        // Status
                        ImGui::Text("Initialized: %s", script.initialized ? "Yes" : "No");
                        ImGui::Text("Lua State: %s", script.L ? "Active" : "None");

                        // Functions available
                        ImGui::Separator();
                        ImGui::Text("Functions:");
                        ImGui::BulletText("OnInit: %s", script.hasOnInit ? "Yes" : "No");
                        ImGui::BulletText("OnUpdate: %s", script.hasOnUpdate ? "Yes" : "No");
                        ImGui::BulletText("OnDestroy: %s", script.hasOnDestroy ? "Yes" : "No");

                        // Update timer
                        ImGui::Text("Update Timer: %.3f", script.updateTimer);

                        ImGui::TreePop();
                    }
                }

                // ==================================================================
                // RIGIDBODY COMPONENT (if you have one)
                // ==================================================================
                /*
                if (entityManager->HasComponent<Rigidbody>(entity)) {
                    if (ImGui::TreeNode("Rigidbody##RigidbodyNode")) {
                        auto& rb = entityManager->GetComponent<Rigidbody>(entity);

                        ImGui::DragFloat2("Velocity", &rb.velocity.x, 0.01f, -100.0f, 100.0f);
                        ImGui::DragFloat("Mass", &rb.mass, 0.1f, 0.1f, 1000.0f);
                        ImGui::DragFloat("Drag", &rb.drag, 0.01f, 0.0f, 10.0f);
                        ImGui::DragFloat("Gravity Scale", &rb.gravityScale, 0.1f, 0.0f, 10.0f);
                        ImGui::Checkbox("Is Kinematic", &rb.isKinematic);

                        ImGui::TreePop();
                    }
                }
                */

                // ==================================================================
                // PREFAB TRACKER INFO
                // ==================================================================
                std::string prefabSource = Framework::PrefabInstanceTracker::Get().GetPrefabOf(entity);
                if (!prefabSource.empty()) {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Prefab: %s",
                        std::filesystem::path(prefabSource).filename().string().c_str());
                }

                // ==================================================================
                // ENTITY ACTIONS
                // ==================================================================
                ImGui::Separator();

                // Delete button
                if (ImGui::Button("Delete##DelBtn")) {
                    entityToDelete = entity;
                    shouldDelete = true;
                }

                ImGui::SameLine();

                // Save as prefab button
                if (ImGui::Button("Save Prefab##SavePrefabBtn")) {
                    std::string prefabPath = "assets/prefabs/entity_" + std::to_string(entity.GetID()) + ".prefab";
                    std::filesystem::create_directories("assets/prefabs");

                    bool saved = PrefabSerializer::SavePrefab(*entityManager, entity, prefabPath);
                    if (saved) {
                        std::cout << "[Inspector] Saved entity " << entity.GetID() << " as prefab\n";
                    }
                }

                ImGui::SameLine();

                // Duplicate button
                if (ImGui::Button("Duplicate##DuplicateBtn")) {
                    // Save to temp prefab and reload
                    std::string tempPath = "assets/prefabs/_temp_duplicate.prefab";
                    if (PrefabSerializer::SavePrefab(*entityManager, entity, tempPath)) {
                        Entity newEntity = PrefabSerializer::LoadPrefab(*entityManager, tempPath);
                        if (newEntity.IsValid()) {
                            // Offset position slightly
                            if (entityManager->HasComponent<Transform>(newEntity)) {
                                auto& t = entityManager->GetComponent<Transform>(newEntity);
                                t.position.x += 0.5f;
                                t.position.y += 0.5f;
                            }
                            std::cout << "[Inspector] Duplicated as entity " << newEntity.GetID() << "\n";
                        }
                    }
                }
            }

            ImGui::PopID();
        }

        ImGui::End();

        // Handle deletion after UI rendering
        if (shouldDelete && entityToDelete.IsValid()) {
            entityManager->DestroyEntity(entityToDelete);
            Framework::SpatialPartitioningRemove(entityToDelete);
            Framework::PrefabInstanceTracker::Get().UnregisterInstance(entityToDelete);

            if (pageEntities.size() == 1 && currentPage > 0) {
                currentPage--;
            }
        }
        if (showGameViewport) ShowGameViewport();
    }

    void ImGuiSystem::ShowPrefabWindow()
    {
        if (!entityManager) return;

        if (!ImGui::Begin("Prefab Editor##PrefabWindow", &showPrefabWindow)) {
            ImGui::End();
            return;
        }

        ImGui::Text("Prefab System");
        ImGui::Separator();

        // ========================================================================
        // SECTION 1: Entity Selection for Saving
        // ========================================================================
        ImGui::Text("Save Entity as Prefab:");
        ImGui::Spacing();

        // Get all entities
        std::vector<Entity> allEntities = entityManager->GetAllEntities();

        // Entity dropdown
        static int selectedIdx = 0;
        if (ImGui::BeginCombo("Select Entity##EntityCombo",
            selectedEntity.IsValid() ?
            ("Entity " + std::to_string(selectedEntity.GetID())).c_str() :
            "None")) {

            for (int i = 0; i < static_cast<int>(allEntities.size()); ++i) {
                Entity e = allEntities[i];
                std::string label = "Entity " + std::to_string(e.GetID());

                // Show component info
                if (entityManager->HasComponent<MeshRenderer>(e)) {
                    auto& mr = entityManager->GetComponent<MeshRenderer>(e);
                    if (!mr.spriteName.empty()) {
                        label += " (" + mr.spriteName + ")";
                    }
                }

                bool isSelected = (selectedEntity.GetID() == e.GetID());
                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    selectedEntity = e;
                    selectedIdx = i;
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // Prefab name input
        static char prefabNameBuffer[256] = "my_entity.prefab";
        ImGui::InputText("Prefab Name##PrefabName", prefabNameBuffer, sizeof(prefabNameBuffer));

        // Save button
        if (ImGui::Button("Save as Prefab##SaveBtn", ImVec2(-1, 0))) {
            if (selectedEntity.IsValid()) {
                std::string path = "assets/prefabs/" + std::string(prefabNameBuffer);

                // Create prefabs directory if it doesn't exist
                std::filesystem::create_directories("assets/prefabs");

                bool saved = PrefabSerializer::SavePrefab(*entityManager, selectedEntity, path);

                if (saved) {
                    std::cout << "[Prefab] ✅ Saved entity " << selectedEntity.GetID()
                        << " to: " << path << "\n";
                }
                else {
                    std::cerr << "[Prefab] ❌ Failed to save prefab to: " << path << "\n";
                }
            }
            else {
                std::cout << "[Prefab] ⚠️ No entity selected!\n";
            }
        }

        ImGui::Separator();
        ImGui::Spacing();

        // ========================================================================
        // SECTION 2: Load Prefab
        // ========================================================================
        ImGui::Text("Load Prefab:");
        ImGui::Spacing();

        // List available prefabs
        static std::vector<std::string> prefabFiles;
        static bool needsRefresh = true;

        if (ImGui::Button("Refresh List##RefreshPrefabs")) {
            needsRefresh = true;
        }

        if (needsRefresh) {
            prefabFiles.clear();
            std::string prefabDir = "assets/prefabs";

            if (std::filesystem::exists(prefabDir)) {
                for (auto const& entry : std::filesystem::directory_iterator(prefabDir)) {
                    if (entry.path().extension() == ".prefab" ||
                        entry.path().extension() == ".json") {
                        prefabFiles.push_back(entry.path().filename().string());
                    }
                }
            }
            needsRefresh = false;
        }

        // Prefab list
        static int selectedPrefabIdx = -1;

        static Framework::Entity selectedInstanceForEdit{};

        ImGui::Text("Available Prefabs:");
        if (ImGui::BeginListBox("##PrefabList", ImVec2(-1, 150))) {
            for (int i = 0; i < static_cast<int>(prefabFiles.size()); ++i) {
                bool isSelected = (selectedPrefabIdx == i);
                if (ImGui::Selectable(prefabFiles[i].c_str(), isSelected)) {
                    selectedPrefabIdx = i;
                    selectedPrefabPath = "assets/prefabs/" + prefabFiles[i];
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox();
        }

        // Spawn position
        static float spawnPos[2] = { 0.0f, 0.0f };
        ImGui::DragFloat2("Spawn Position##SpawnPos", spawnPos, 0.01f, -10.0f, 10.0f);

        ImGui::Separator();

        // Load button
        if (ImGui::Button("Load Prefab##LoadBtn", ImVec2(-1, 0))) {
            if (selectedPrefabIdx >= 0 && selectedPrefabIdx < static_cast<int>(prefabFiles.size())) {
                Entity newEntity = PrefabSerializer::LoadPrefab(*entityManager, selectedPrefabPath);

                if (newEntity.IsValid()) {
                    // Set spawn position
                    if (entityManager->HasComponent<Transform>(newEntity)) {
                        auto& transform = entityManager->GetComponent<Transform>(newEntity);
                        transform.position.x = spawnPos[0];
                        transform.position.y = spawnPos[1];
                    }

                    std::cout << "[Prefab] ✅ Loaded prefab '" << prefabFiles[selectedPrefabIdx]
                        << "' as entity " << newEntity.GetID() << "\n";
                }
                else {
                    std::cerr << "[Prefab] ❌ Failed to load prefab: " << selectedPrefabPath << "\n";
                }
            }
            else {
                std::cout << "[Prefab] ⚠️ No prefab selected!\n";
            }
        }

        ImGui::Separator();
        ImGui::Spacing();

        // ========================================================================
        // SECTION 3: Prefab-wide Inspector (applies to ALL instances)
        // ========================================================================
        ImGui::Text("Prefab Instances:");
        ImGui::Spacing();

        if (selectedPrefabIdx >= 0 && selectedPrefabIdx < static_cast<int>(prefabFiles.size())) {
            std::string prefabPath = "assets/prefabs/" + prefabFiles[selectedPrefabIdx];
            auto instances = Framework::PrefabInstanceTracker::Get().GetInstancesOf(prefabPath);

            ImGui::Text("Instances of '%s': %zu",
                prefabFiles[selectedPrefabIdx].c_str(),
                instances.size());

            // Read-only list of instances (no selection)
            if (!instances.empty()) {
                if (ImGui::BeginListBox("##InstanceList", ImVec2(-1, 80))) {
                    for (auto& inst : instances) {
                        std::string label = "Entity " + std::to_string(inst.GetID());
                        ImGui::Selectable(label.c_str(), false);
                    }
                    ImGui::EndListBox();
                }
            }

            // Use the FIRST instance as the "template" for editing
            Framework::Entity templateEntity = instances.empty() ? Framework::Entity{} : instances[0];

            ImGui::Separator();
            ImGui::Text("Prefab Inspector (applies to ALL instances)");
            ImGui::Spacing();

            if (templateEntity.IsValid()) {

                // ---------------- TRANSFORM ----------------
                if (entityManager->HasComponent<Transform>(templateEntity)) {
                    if (ImGui::TreeNode("Transform##PrefabAllTransform")) {
                        auto& t = entityManager->GetComponent<Transform>(templateEntity);

                        ImGui::DragFloat2("Position##AllPos", &t.position.x, 0.01f, -100.0f, 100.0f);
                        ImGui::DragFloat2("Scale##AllScale", &t.scale.x, 0.01f, 0.01f, 100.0f);
                        ImGui::DragFloat("Rotation##AllRot", &t.rotation, 0.1f, -360.0f, 360.0f);

                        ImGui::TreePop();
                    }
                }

                // ---------------- SPRITE ----------------
                if (entityManager->HasComponent<Sprite>(templateEntity)) {
                    if (ImGui::TreeNode("Sprite##PrefabAllSprite")) {
                        auto& s = entityManager->GetComponent<Sprite>(templateEntity);

                        char texBuf[256];
                        std::strncpy(texBuf, s.texturePath.c_str(), sizeof(texBuf) - 1);
                        texBuf[sizeof(texBuf) - 1] = '\0';

                        if (ImGui::InputText("Texture Path##AllTex", texBuf, sizeof(texBuf))) {
                            s.texturePath = texBuf;
                        }

                        ImGui::DragInt("Layer##AllSpriteLayer", &s.layer, 1, -100, 100);

                        ImGui::TreePop();
                    }
                }

                // ---------------- MESH RENDERER ----------------
                if (entityManager->HasComponent<MeshRenderer>(templateEntity)) {
                    if (ImGui::TreeNode("MeshRenderer##PrefabAllMesh")) {
                        auto& mr = entityManager->GetComponent<MeshRenderer>(templateEntity);

                        char nameBuf[256];
                        std::strncpy(nameBuf, mr.spriteName.c_str(), sizeof(nameBuf) - 1);
                        nameBuf[sizeof(nameBuf) - 1] = '\0';

                        if (ImGui::InputText("Sprite Name##AllSpriteName", nameBuf, sizeof(nameBuf))) {
                            mr.spriteName = nameBuf;
                        }

                        ImGui::DragInt("Layer##AllMeshLayer", &mr.layer, 1, -100, 100);
                        ImGui::DragInt("Order in Layer##AllOrder", &mr.orderInLayer, 1, -100, 100);

                        float tint[4] = { mr.tint.r, mr.tint.g, mr.tint.b, mr.tint.a };
                        if (ImGui::ColorEdit4("Tint##AllTint", tint)) {
                            mr.tint.r = tint[0];
                            mr.tint.g = tint[1];
                            mr.tint.b = tint[2];
                            mr.tint.a = tint[3];
                        }

                        ImGui::TreePop();
                    }
                }

                // ---------------- MOVEMENT ----------------
                if (entityManager->HasComponent<Movement>(templateEntity)) {
                    if (ImGui::TreeNode("Movement##PrefabAllMove")) {
                        auto& m = entityManager->GetComponent<Movement>(templateEntity);

                        ImGui::DragFloat("Speed##AllSpeed", &m.moveSpeed, 0.01f, 0.0f, 100.0f);
                        ImGui::DragFloat2("Direction##AllDir", &m.direction.x, 0.01f, -1.0f, 1.0f);

                        ImGui::TreePop();
                    }
                }

                // ---------------- BOX COLLIDER ----------------
                if (entityManager->HasComponent<BoxCollider>(templateEntity)) {
                    if (ImGui::TreeNode("BoxCollider##PrefabAllBox")) {
                        auto& bc = entityManager->GetComponent<BoxCollider>(templateEntity);

                        ImGui::DragFloat2("Size##AllBoxSize", &bc.size.x, 0.01f, 0.0f, 100.0f);
                        ImGui::DragFloat2("Offset##AllBoxOffset", &bc.offset.x, 0.01f, -100.0f, 100.0f);
                        ImGui::Checkbox("Is Trigger##AllBoxTrigger", &bc.isTrigger);

                        ImGui::TreePop();
                    }
                }

                // ---------------- CIRCLE COLLIDER ----------------
                if (entityManager->HasComponent<CircleCollider>(templateEntity)) {
                    if (ImGui::TreeNode("CircleCollider##PrefabAllCircle")) {
                        auto& cc = entityManager->GetComponent<CircleCollider>(templateEntity);

                        ImGui::DragFloat("Radius##AllCircleRadius", &cc.radius, 0.01f, 0.0f, 100.0f);
                        ImGui::DragFloat2("Offset##AllCircleOffset", &cc.offset.x, 0.01f, -100.0f, 100.0f);

                        ImGui::TreePop();
                    }
                }

                // ------------------------------------------------
                // APPLY TO ALL INSTANCES + SAVE PREFAB
                // ------------------------------------------------
                ImGui::Separator();
                if (ImGui::Button("Apply To All Instances & Save Prefab##ApplyAll2", ImVec2(-1, 0))) {

                    // 1) propagate templateEntity's values to every instance
                    for (auto& inst : instances) {
                        if (!inst.IsValid())
                            continue;

                        // --- Transform (position, scale, rotation) ---
                        if (entityManager->HasComponent<Transform>(templateEntity) &&
                            entityManager->HasComponent<Transform>(inst)) {

                            auto& src = entityManager->GetComponent<Transform>(templateEntity);
                            auto& dst = entityManager->GetComponent<Transform>(inst);

                            //dst.position = src.position;
                            dst.scale = src.scale;
                            dst.rotation = src.rotation;
                        }

                        // --- Sprite ---
                        if (entityManager->HasComponent<Sprite>(templateEntity) &&
                            entityManager->HasComponent<Sprite>(inst)) {

                            auto& src = entityManager->GetComponent<Sprite>(templateEntity);
                            auto& dst = entityManager->GetComponent<Sprite>(inst);

                            dst.texturePath = src.texturePath;
                            dst.layer = src.layer;
                        }

                        // --- MeshRenderer ---
                        if (entityManager->HasComponent<MeshRenderer>(templateEntity) &&
                            entityManager->HasComponent<MeshRenderer>(inst)) {

                            auto& src = entityManager->GetComponent<MeshRenderer>(templateEntity);
                            auto& dst = entityManager->GetComponent<MeshRenderer>(inst);

                            dst.spriteName = src.spriteName;
                            dst.layer = src.layer;
                            dst.orderInLayer = src.orderInLayer;
                            dst.tint = src.tint;
                        }

                        // --- Movement ---
                        if (entityManager->HasComponent<Movement>(templateEntity) &&
                            entityManager->HasComponent<Movement>(inst)) {

                            auto& src = entityManager->GetComponent<Movement>(templateEntity);
                            auto& dst = entityManager->GetComponent<Movement>(inst);

                            dst.moveSpeed = src.moveSpeed;
                            dst.direction = src.direction;
                        }

                        // --- BoxCollider ---
                        if (entityManager->HasComponent<BoxCollider>(templateEntity) &&
                            entityManager->HasComponent<BoxCollider>(inst)) {

                            auto& src = entityManager->GetComponent<BoxCollider>(templateEntity);
                            auto& dst = entityManager->GetComponent<BoxCollider>(inst);

                            dst.size = src.size;
                            dst.offset = src.offset;
                            dst.isTrigger = src.isTrigger;
                        }

                        // --- CircleCollider ---
                        if (entityManager->HasComponent<CircleCollider>(templateEntity) &&
                            entityManager->HasComponent<CircleCollider>(inst)) {

                            auto& src = entityManager->GetComponent<CircleCollider>(templateEntity);
                            auto& dst = entityManager->GetComponent<CircleCollider>(inst);

                            dst.radius = src.radius;
                            dst.offset = src.offset;
                        }
                    }

                    // 2) write the prefab file using the template entity
                    PrefabSerializer::SavePrefab(*entityManager, templateEntity, prefabPath);

                    std::cout << "[Prefab] Applied changes to " << instances.size()
                        << " instances and saved prefab: " << prefabPath << "\n";
                }
            }
        }

        // Clear tracking button (can stay as before)
        if (ImGui::Button("Clear All Tracking##ClearTracking", ImVec2(-1, 0))) {
            Framework::PrefabInstanceTracker::Get().Clear();
            std::cout << "[Prefab] Cleared all instance tracking\n";
        }


        ImGui::End();
    }

    void ImGuiSystem::ShowSpawnerWindow()
    {
        if (!entitySpawner) return;

        //ImGui::SetNextWindowSize(ImVec2(250, 450), ImGuiCond_FirstUseEver);
        //ImGui::SetNextWindowPos(ImVec2(370, 30), ImGuiCond_FirstUseEver);

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
            Framework::Entity enemy = entitySpawner->SpawnEnemy(Vector2D(spawnX, spawnY));
            //temporary put spatialPartitioningInsert function here 
			//to show that how does the spatial partitioning works with entity spawner
            Framework::SpatialPartitioningInsert(enemy);
        }

        ImGui::Separator();

        static int waveCount = 5;
        ImGui::SliderInt("Wave Size##Wave", &waveCount, 1, 20);
        if (ImGui::Button("Spawn Enemy Wave##Btn5", ImVec2(-1, 0))) {
            entitySpawner->SpawnEnemyWave(waveCount, 0.8f);
        }

        ImGui::Separator();

        if (ImGui::Button("Trigger Leaves SFX##Btn8", ImVec2(-1, 0))) {
            if (audioSystem) {
                std::cout << "[DEBUG] AudioSystem exists\n";
                audioSystem->PlaySound("leaves", false);
                std::cout << "[DEBUG] PlaySound called\n";
            }
            else {
                std::cout << "[DEBUG] ERROR: AudioSystem is NULL!\n";
            }
        }

        if (ImGui::Button("Trigger Shooting SFX##Btn9", ImVec2(-1, 0))) {
            if (audioSystem) {
                std::cout << "[DEBUG] AudioSystem exists\n";
                audioSystem->PlaySound("shooting", false);
                std::cout << "[DEBUG] PlaySound called\n";
            }
            else {
                std::cout << "[DEBUG] ERROR: AudioSystem is NULL!\n";
            }
        }

        if (ImGui::Button("Trigger Menu BGM##Btn9", ImVec2(-1, 0))) {
            if (audioSystem) {
                std::cout << "[DEBUG] AudioSystem exists\n";
                audioSystem->PlaySound("mmbgm", false);
                std::cout << "[DEBUG] PlaySound called\n";
            }
            else {
                std::cout << "[DEBUG] ERROR: AudioSystem is NULL!\n";
            }
        }

        if (ImGui::Button("Trigger In-Game BGM##Btn10", ImVec2(-1, 0))) {
            if (audioSystem) {
                std::cout << "[DEBUG] AudioSystem exists\n";
                audioSystem->PlaySound("bgm", false);
                std::cout << "[DEBUG] PlaySound called\n";
            }
            else {
                std::cout << "[DEBUG] ERROR: AudioSystem is NULL!\n";
            }
        }

        if (ImGui::Button("Stop All Audio##Btn11", ImVec2(-1, 0))) {
            if (audioSystem) {
                std::cout << "[DEBUG] Stopping all audio\n";
                audioSystem->StopAllSounds();
                std::cout << "[DEBUG] All audio stopped\n";
            }
            else {
                std::cout << "[DEBUG] ERROR: AudioSystem is NULL!\n";
            }
        }

        ImGui::Separator();
        ImGui::Text("Quick Audio Test Zone");
        ImGui::Text("(Drag audio files here to play)");

        // Create a colored button as drop target
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 0.4f));
        ImGui::Button("Drop Audio Here##AudioDropZone", ImVec2(-1, 60));
        ImGui::PopStyleColor();

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Audio")) {
                const char* audioName = static_cast<const char*>(payload->Data);

                std::cout << "[Spawner] Playing dropped audio: " << audioName << "\n";

                if (audioSystem) {
                    audioSystem->PlaySound(audioName, false);
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::Separator();
        ImGui::Text("📦 Prefab Drop Zone");
        ImGui::TextWrapped("Drag prefabs here to spawn");

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.4f, 0.8f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.5f, 0.9f, 0.6f));
        ImGui::Button("Drop Prefab Here##PrefabDropZone", ImVec2(-1, 60));
        ImGui::PopStyleColor(2);

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Prefab")) {
                const char* prefabPath = static_cast<const char*>(payload->Data);

                std::cout << "[Spawner] Prefab dropped: " << prefabPath << "\n";

                Entity newEntity = PrefabSerializer::LoadPrefab(*entityManager, prefabPath);

                if (newEntity.IsValid()) {
                    // Set spawn position from spawner's X/Y values
                    if (entityManager->HasComponent<Transform>(newEntity)) {
                        auto& transform = entityManager->GetComponent<Transform>(newEntity);
                        transform.position.x = spawnX;
                        transform.position.y = spawnY;
                    }

                    std::cout << "[Spawner] ✅ Spawned prefab as entity " << newEntity.GetID() << "\n";
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::End();
        if (showGameViewport) ShowGameViewport();

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
        ImGui::BulletText("Key 3: Go to level3");
        ImGui::BulletText("Key 5: Go to Main Menu");
        ImGui::BulletText("key 0: Reset Camera");
        ImGui::BulletText("click above menu Editor->Play to activate play mode");
        ImGui::BulletText("WASD: Move");
        ImGui::BulletText("SPACE: Shoot Up");
        ImGui::BulletText("SHIFT: Shoot Down");
        ImGui::BulletText("Q/ESC: Quit");



        ImGui::End();
    }

    //game object picking in editor - jiahao
    void ImGuiSystem::UpdatePicking() {
        if (!CORE) {
            return;
        }

        if (CORE->IsPlaying()) {
            return;
        }

        if (!entityManager) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

        if (io.WantCaptureMouse) {
            return;
        }

        InputSystem* input = Framework::CORE->GetInputSystem();

        UISystem* ui = CORE->GetUISystem();

        if (!input || !ui) {
            return;
        }

        if (!input->IsKeyPressed(MOUSE_LEFT)) {
            return;
        }

        float mouseX = 0.0f;
        float mouseY = 0.0f;

        input->GetMousePosition(mouseX, mouseY);

        Vector2D mouseWorld = ui->ScreenToWorld(mouseX, mouseY);

        Entity picked = INVALID_ENTITY;

        for (Entity e : entityManager->GetAllEntities()) {
            if (!entityManager->HasComponent<Transform>(e)) {
                continue;
            }

            auto& transform = entityManager->GetComponent<Transform>(e);

            if (entityManager->HasComponent<GridTiles>(e)) {
                continue;
            }

            Collider collider;
            bool hasCollider = false;

            if (entityManager->HasComponent<CircleCollider>(e)) {
                auto& cc = entityManager->GetComponent<CircleCollider>(e);

				float scaleX = transform.scale.x;
				float scaleY = transform.scale.y;

				float scaleFactor = scaleX > scaleY ? scaleX : scaleY;

                if (scaleFactor < 0.01f) {
                    scaleFactor = 0.01f;
                }

				float worldRadius = cc.radius * scaleFactor;

                collider = Collider::create_circle(
                    worldRadius,
                    transform.position + cc.offset
                );

                hasCollider = true;
            }

            else if (entityManager->HasComponent<BoxCollider>(e)) {
                auto& bc = entityManager->GetComponent<BoxCollider>(e);

                float scaleX = transform.scale.x;
                float scaleY = transform.scale.y;

				if (scaleX < 0.01f) scaleX = 0.01f;
                if (scaleY < 0.01f) scaleY = 0.01f;

                float worldWidth = bc.size.x * scaleX;
				float worldHeight = bc.size.y * scaleY;

                collider = Collider::create_rect(
                    worldWidth,
                    worldHeight,
                    transform.position
                );

                hasCollider = true;
            }

            if (!hasCollider) {
                continue;
            }

            if (point_in_collider(mouseWorld, collider)) {
                picked = e;
                break;
            }
        }

        selectedEntity = picked;

        if (selectedEntity.GetID() != INVALID_ENTITY) {
            std::cout << "[ImGui Picking] Selected entity ID: "
                << selectedEntity.GetID() << "\n";
        }
        else
        {
            std::cout << "[ImGui Picking] Clicked empty space\n";
        }
    }

    //
    void ImGuiSystem::UpdateEntityDragging() {
        if (!CORE) {
            return;
        }

        if (CORE->IsPlaying()) {
            return;
        }

        if (!entityManager) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

        if (io.WantCaptureMouse) {
            return;
        }

        InputSystem* input = Framework::CORE->GetInputSystem();
        UISystem* ui = CORE->GetUISystem();

        if (!input || !ui) {
            return;
        }

		float mouseX = 0.0f;
		float mouseY = 0.0f;
        input->GetMousePosition(mouseX, mouseY);
		Vector2D mouseWorld = ui->ScreenToWorld(mouseX, mouseY);

        if (input->IsKeyPressed(MOUSE_LEFT) || input->IsKeyPressed(MOUSE_RIGHT)) {
            if (!selectedEntity.IsValid() ||
                !entityManager->HasComponent<Framework::Transform>(selectedEntity)) {
				isDraggingEntity = false;
				isScalingEntity = false;
				isRotatingEntity = false;
				draggingEntity = Framework::Entity{ INVALID_ENTITY};
                return;
            }
            RecordUndoStep(selectedEntity);

			auto& transform = entityManager->GetComponent<Framework::Transform>(selectedEntity);

            if (input->IsKeyPressed(MOUSE_LEFT) && input->IsKeyDown(KEY_SHIFT)) {
				isScalingEntity = true;
				isDraggingEntity = false;
				isRotatingEntity = false;

				draggingEntity = selectedEntity;
				scaleStartMouse = mouseWorld;
				scaleStartScale = transform.scale;
            }
            else if (input->IsKeyPressed(MOUSE_RIGHT)) {
				isRotatingEntity = true;
				isDraggingEntity = false;
				isScalingEntity = false;

				draggingEntity = selectedEntity;

                Vector2D toMouse = transform.position;
                rotateStartAngle = std::atan2(toMouse.y, toMouse.x);
				rotateStartRotation = transform.rotation;
            }

            else if (input->IsKeyPressed(MOUSE_LEFT)) {
				isDraggingEntity = true;
				isScalingEntity = false;
				isRotatingEntity = false;

				draggingEntity = selectedEntity;
				dragOffset = transform.position - mouseWorld;
            }
        }
        if (isDraggingEntity && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!draggingEntity.IsValid() ||
                !entityManager->HasComponent<Framework::Transform>(draggingEntity))
            {
                isDraggingEntity = false;
                return;
            }
            float moveX = 0.0f;
			float moveY = 0.0f;
			input->GetMousePosition(moveX, moveY);
            Vector2D moveWorld = ui->ScreenToWorld(moveX, moveY);

			auto& transform =
                entityManager->GetComponent<Framework::Transform>(draggingEntity);

			transform.position = moveWorld + dragOffset;
        }

        if (isScalingEntity && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!draggingEntity.IsValid() ||
                !entityManager->HasComponent<Framework::Transform>(draggingEntity)) 
            {
                isScalingEntity = false;
                return;
            }

            float scaleX = 0.0f;
            float scaleY = 0.0f;

            input->GetMousePosition(scaleX, scaleY);
            Vector2D scaleMouseWorld = ui->ScreenToWorld(scaleX, scaleY);

            auto& transform =
                entityManager->GetComponent<Framework::Transform>(draggingEntity);

            Vector2D delta = scaleMouseWorld - scaleStartMouse;

            float factor = 1.0f + delta.x * 0.5f;
            if (factor < 0.1f) {
                factor = 0.1f;
            }
            if (factor > 5.0f) {
                factor = 5.0f;
            }

            transform.scale.x = scaleStartScale.x * factor;
            transform.scale.y = scaleStartScale.y * factor;
        }
        if (isRotatingEntity && ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            /*if (!draggingEntity.IsValid() ||
                !entityManager->HasComponent<Framework::Transform>(draggingEntity)) {

                isRotatingEntity = false;
                return;
            }

            float rotX = 0.0f;
            float rotY = 0.0f;
            input->GetMousePosition(rotX, rotY);
            Vector2D rotMouseWorld = ui->ScreenToWorld(rotX, rotY);

            auto& transform =
                entityManager->GetComponent<Framework::Transform>(draggingEntity);

            Vector2D toMouse = rotMouseWorld - transform.position;
            float currentAngle = std::atan2(toMouse.y, toMouse.x);

            float deltaAngle = currentAngle - rotateStartAngle;
            transform.rotation = rotateStartRotation + deltaAngle;*/
            if (!selectedEntity.IsValid() ||
                !entityManager->HasComponent<Framework::Transform>(selectedEntity)) {
				isRotatingEntity = false;
            }
            else {
                auto& transform = entityManager->GetComponent<Framework::Transform>(selectedEntity);

				const float rotationSpeed = 0.2f;

				transform.rotation += rotationSpeed;
            }
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (isDraggingEntity || isScalingEntity) {
                isDraggingEntity = false;
                isScalingEntity = false;
                RebuildSpatialPartition();
            }
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            
                isRotatingEntity = false;
            
        }
    }
    //undo - jiahao
    void ImGuiSystem::RecordUndoStep(Entity entity) {
        // First, check if the entity is valid and has a Transform to save
        if (!entityManager || !entity.IsValid() || !entityManager->HasComponent<Transform>(entity)) {
            return; // Safety check failed, do nothing
        }

        // Get the CURRENT position of the entity (before the user moves it)
        auto& transform = entityManager->GetComponent<Transform>(entity);

        // Create a new undo step with this info
        UndoStep step;
        step.entity = entity;
        step.oldPosition = transform.position;

        // Add this step to the end of our history list
        undoStack.push_back(step);

        // Check if we have exceeded our memory limit (e.g., 20 steps)
        if (undoStack.size() > 20) {
            // Remove the oldest step (the one at the front/beginning)
            undoStack.erase(undoStack.begin());
        }

        // Log for debugging purposes
        std::cout << "[Editor] Recorded undo step for Entity " << entity.GetID() << "\n";
    }
    //undo - jiahao
    void ImGuiSystem::PerformUndo() {
        // 1. Check if we have anything to undo
        if (undoStack.empty()) {
            std::cout << "[Editor] Nothing to undo.\n";
            return;
        }

        // 2. Get the last action we recorded (the one at the back of the vector)
        UndoStep lastStep = undoStack.back();

        // 3. Verify the entity still exists (it might have been deleted since we saved it!)
        if (entityManager && entityManager->HasComponent<Transform>(lastStep.entity)) {

            // Get access to the entity's transform component
            auto& transform = entityManager->GetComponent<Transform>(lastStep.entity);

            // 4. Restore the position to what it was in the saved step
            transform.position = lastStep.oldPosition;

            std::cout << "[Editor] Undid movement for Entity " << lastStep.entity.GetID() << "\n";
        }
        else {
            std::cout << "[Editor] Cannot undo: Entity no longer exists.\n";
        }

        // 5. Remove this step from the history since we just used it
        undoStack.pop_back();
    }

    bool ImGuiSystem::IsAudioFile(const std::filesystem::path& path) const {
        if (!path.has_extension()) {
            return false;
        }

        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // Accept .wav format
        return ext == ".wav";
    }

    // ============================================================================
    // Updated OnFileDrop to handle audio files
    // author: Ethan Ng (modification)
    // ============================================================================
    void ImGuiSystem::OnFileDrop(int count, const char** paths) {
        std::cout << "\n[FileDrop] Received " << count << " file(s)\n";

        for (int i = 0; i < count; ++i) {
            std::filesystem::path path(paths[i]);
            std::cout << "[FileDrop] Processing: " << path.filename() << "\n";

            // Handle level files
            if (IsLevelFile(path)) {
                bool isOpen = OpenLevelFromTxt(path.string(), true);
                if (isOpen) {
                    currentLevelPath = path.string();
                    std::cout << "[FileDrop]  Loaded level\n";
                }
                else {
                    std::cerr << "[FileDrop] ❌ Failed to load level\n";
                }
                continue;
            }

            // Handle texture files
            if (IsTextureFile(path)) {
                if (!entitySpawner) {
                    std::cerr << "[FileDrop] ❌ EntitySpawner not available\n";
                    continue;
                }

                std::string label = path.filename().string();
                std::string filePath = std::string("assets/") + label;

                Framework::Entity entity = entitySpawner->SpawnSprite(
                    filePath,
                    Vector2D(0.0f, 0.0f),
                    Vector2D(1.0f, 1.0f)
                );
                std::cout << "[FileDrop]  Spawned sprite as entity " << entity.id << "\n";
                continue;
            }

            // ====================================================================
            // Handle AUDIO files - AUTOMATIC JSON UPDATE
            // ====================================================================
            std::string errorMsg;
            if (IsAudioFileSupported(path, errorMsg)) {
                if (!audioSystem) {
                    std::cerr << "[FileDrop] ❌ AudioSystem not available\n";
                    continue;
                }

                std::string audioName = path.stem().string();
                std::string fileName = path.filename().string();

                std::cout << "[FileDrop]  Valid audio file: " << audioName << ".wav\n";

                // Copy file to assets folder
                std::filesystem::path destPath = std::filesystem::path("assets") / fileName;

                if (!std::filesystem::exists(destPath)) {
                    try {
                        std::filesystem::copy_file(path, destPath);
                        std::cout << "[FileDrop] Copied to: " << destPath << "\n";
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[FileDrop] ❌ Failed to copy: " << e.what() << "\n";
                        continue;
                    }
                }
                else {
                    std::cout << "[FileDrop] File already in assets\n";
                }

                // Add to audio.json
                bool added = AddAudioToJSON(audioName, fileName);

                if (added) {
                    std::cout << "[FileDrop]  Added to audio.json\n";

                    // Reload audio system
                    audioSystem->ReloadAudioLibrary();
                    std::cout << "[FileDrop]  Audio library reloaded\n";

                    // Play new audio
                    audioSystem->PlaySound(audioName.c_str(), false);
                    std::cout << "[FileDrop]  Playing: " << audioName << "\n";
                }
                else {
                    std::cerr << "[FileDrop] ❌ Failed to add to audio.json\n";
                }

                continue;
            }

            // Unsupported format - show error
            auto ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".ogg" || ext == ".mp3" || ext == ".txt" ||
                ext == ".flac" || ext == ".aiff" || ext == ".aac" || ext == ".m4a") {

                std::cout << "[FileDrop] ❌ Unsupported format: " << ext << "\n";
                audioErrorMessage = errorMsg;
                showAudioErrorPopup = true;
                continue;
            }

            if (path.extension() == ".prefab" || path.extension() == ".json") {
                Entity newEntity = PrefabSerializer::LoadPrefab(*entityManager, path.string());

                if (newEntity.IsValid()) {
                    std::cout << "[FileDrop] ✅ Loaded prefab: " << path.filename()
                        << " as entity " << newEntity.GetID() << "\n";
                }
                else {
                    std::cerr << "[FileDrop] ❌ Failed to load prefab: " << path << "\n";
                }
                continue;
            }

            std::cerr << "[FileDrop] ❌ Unsupported file type\n";
        }



        std::cout << "[FileDrop] Processing complete\n\n";
    }

    bool ImGuiSystem::IsAudioFileSupported(const std::filesystem::path& path, std::string& errorMsg) const {
        if (!path.has_extension()) {
            errorMsg = "File has no extension";
            return false;
        }

        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // Check if it's .wav (supported)
        if (ext == ".wav") {
            return true;
        }

        // Reject other formats
        if (ext == ".ogg") {
            errorMsg = "Unsupported audio format: .ogg\n\nPlease use .wav format instead.";
            return false;
        }

        if (ext == ".mp3") {
            errorMsg = "Unsupported audio format: .mp3\n\n";
            errorMsg += "Please use .wav format instead.\n\n";
            errorMsg += "To convert:\n";
            errorMsg += "1. Use Audacity or online converter\n";
            errorMsg += "2. Export as WAV\n";
            errorMsg += "3. Drag the .wav file into the editor";
            return false;
        }

        if (ext == ".txt") {
            errorMsg = "Invalid file type: .txt\n\n";
            errorMsg += "This is a text file, not an audio file.\n\n";
            errorMsg += "Please drag an audio file (.wav format)";
            return false;
        }

        // Other formats
        errorMsg = "Unsupported audio format: " + ext + "\n\n";
        errorMsg += "Only .wav format is supported.";
        return false;
    }

    bool ImGuiSystem::AddAudioToJSON(const std::string& audioName, const std::string& fileName) {
        const std::string jsonPath = "assets/audio.json";

        std::cout << "[JSON] Opening: " << jsonPath << "\n";

        // Read existing JSON
        std::ifstream readFile(jsonPath);

        if (!readFile.is_open()) {
            std::cerr << "[JSON] ❌ Could not open audio.json for reading\n";
            return false;
        }

        std::stringstream buffer;
        buffer << readFile.rdbuf();
        readFile.close();

        std::string jsonContent = buffer.str();

        // Check if audio already exists
        if (jsonContent.find("\"" + audioName + "\"") != std::string::npos) {
            std::cout << "[JSON] Audio '" << audioName << "' already exists in JSON\n";
            return true;
        }

        // Find where to insert
        size_t lastBrace = jsonContent.rfind('}');
        if (lastBrace == std::string::npos) {
            std::cerr << "[JSON] ❌ Malformed JSON - no closing brace\n";
            return false;
        }

        size_t insertPos = jsonContent.rfind(',', lastBrace);

        // Build new entry
        std::string newEntry = ",\n    \"" + audioName + "\": \"" + fileName + "\"";

        if (insertPos == std::string::npos) {
            size_t openBrace = jsonContent.find('{');
            if (openBrace != std::string::npos) {
                insertPos = openBrace;
                newEntry = "\n    \"" + audioName + "\": \"" + fileName + "\"";
            }
            else {
                std::cerr << "[JSON] ❌ Malformed JSON - no opening brace\n";
                return false;
            }
        }

        // Insert new entry
        jsonContent.insert(insertPos + 1, newEntry);

        std::cout << "[JSON] Adding entry: \"" << audioName << "\": \"" << fileName << "\"\n";

        // Write updated JSON
        std::ofstream writeFile(jsonPath);

        if (!writeFile.is_open()) {
            std::cerr << "[JSON] ❌ Could not open audio.json for writing\n";
            return false;
        }

        writeFile << jsonContent;
        writeFile.close();

        std::cout << "[JSON]  Successfully updated audio.json\n";

        return true;
    }

    void ImGuiSystem::BeginGameRender()
    {
        // Bind the framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO);
        glViewport(0, 0, viewportWidth, viewportHeight);

        // Clear the framebuffer
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);  // Dark background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void ImGuiSystem::EndGameRender()
    {
        // Unbind framebuffer - return to default
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // ============================================================================
    // STEP 3: Game Viewport Window
    // ============================================================================

    void ImGuiSystem::CreateViewportFramebuffer(int width, int height)
    {
        viewportWidth = width;
        viewportHeight = height;

        // Create framebuffer
        glGenFramebuffers(1, &viewportFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO);

        std::cout << "[ImGuiSystem] Created FBO: " << viewportFBO << "\n";

        // Create color texture
        glGenTextures(1, &viewportTexture);
        glBindTexture(GL_TEXTURE_2D, viewportTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, viewportTexture, 0);

        std::cout << "[ImGuiSystem] Created Texture: " << viewportTexture << "\n";

        // Create depth/stencil renderbuffer
        glGenRenderbuffers(1, &viewportRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, viewportRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, viewportRBO);

        // Check completeness
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "[ImGuiSystem] ERROR: Framebuffer incomplete! Status: 0x"
                << std::hex << status << std::dec << "\n";
        }
        else {
            std::cout << "[ImGuiSystem] Framebuffer complete: " << width << "x" << height << "\n";
        }

        // IMPORTANT: Unbind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void ImGuiSystem::ResizeViewportFramebuffer(int width, int height)
    {
        if (width <= 0 || height <= 0) return;
        if (width == viewportWidth && height == viewportHeight) return;

        viewportWidth = width;
        viewportHeight = height;

        // Resize texture
        glBindTexture(GL_TEXTURE_2D, viewportTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Resize renderbuffer
        glBindRenderbuffer(GL_RENDERBUFFER, viewportRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        std::cout << "[ImGuiSystem] Viewport resized: " << width << "x" << height << "\n";
    }

    void ImGuiSystem::DeleteViewportFramebuffer()
    {
        if (viewportFBO) {
            glDeleteFramebuffers(1, &viewportFBO);
            viewportFBO = 0;
        }
        if (viewportTexture) {
            glDeleteTextures(1, &viewportTexture);
            viewportTexture = 0;
        }
        if (viewportRBO) {
            glDeleteRenderbuffers(1, &viewportRBO);
            viewportRBO = 0;
        }
    }

    void ImGuiSystem::ShowGameViewport()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        // Set minimum window size
        ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));

        if (ImGui::Begin("Game##GameViewport", &showGameViewport)) {

            ImVec2 size = ImGui::GetContentRegionAvail();

            // Only resize if size is reasonable
            if (size.x >= 100 && size.y >= 100) {
                int newW = static_cast<int>(size.x);
                int newH = static_cast<int>(size.y);

                if (newW != viewportWidth || newH != viewportHeight) {
                    ResizeViewportFramebuffer(newW, newH);
                }

                // Display texture
                ImGui::Image(
                    (ImTextureID)(intptr_t)viewportTexture,
                    size,
                    ImVec2(0, 1),
                    ImVec2(1, 0)
                );
            }
            else {
                ImGui::Text("Viewport too small: %.0fx%.0f", size.x, size.y);
            }
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }
} // namespace Framework