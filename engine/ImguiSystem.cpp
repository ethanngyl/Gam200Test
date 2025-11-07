/*
===============================================================================
File:        ImGuiSystem.cpp
Author:      Ethan Ng, Jiahao Zhou
Email:       n.ethanyongle@digipen.edu, jiahao.zhou@digipen.edu,
Date:        2025-11-07
Contribution: 45%(Ethan), 55%(Jiahao)
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
        //io.IniFilename = nullptr;  // Disable settings file

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        EnableFileDragAndDrop();
        std::cout << "[ImGui] Initialized successfully\n";
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
            
        }
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
                || entityManager->HasComponent<TriangleCollider>(entity)
                || entityManager->HasComponent<BoxCollider>(entity);

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

			// this if else condition is to check if entity has trianglecollider component
            if (entityManager->HasComponent<TriangleCollider>(entity)) {
				// get reference to the trianglecollider component
                auto& triangleCollider = entityManager->GetComponent<TriangleCollider>(entity);
				//write the 3 vertices (x,y)W
                writeFile << "TriangleCollider " << triangleCollider.v0.x << " " << triangleCollider.v0.y << " "
                    << triangleCollider.v1.x << " " << triangleCollider.v1.y << " "
                    << triangleCollider.v2.x << " " << triangleCollider.v2.y << "\n";
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
    // This is the function to handfle files drop onto the window
    // author: jiahao zhou 
    // ============================================================================
    void ImGuiSystem::OnFileDrop(int count, const char** paths) {
		//loop through all dropped files
        for (int i = 0; i < count; ++i) {
			// convert the file path to std::filesystem::path for easier checks
			std::filesystem::path path(paths[i]);
			//if the file is a level file(txt)
            if (IsLevelFile(path)) {
				//try to open the level file
                bool isOpen = OpenLevelFromTxt(path.string(), true);
				// if opened successfully, record the current level path
                if (isOpen) {
                    currentLevelPath = path.string();
                    std::cout << "[Drop] Opened level file: " << currentLevelPath << "\n";
                }
                //else show error message
                else {
                    std::cerr << "[DropError] Unsupported file type: " << path << "\n";
                }
				//move to next dropped file
                continue;
            }

			//check if the file is a texture file(jpg/png/jpeg)
            if (IsTextureFile(path)) {
				//make sure entity spawner is valid
                if (!entitySpawner) {
					//if not valid, show error message and skip this file
					std::cerr << "[DropError] Missing EntitySpawner, cannot spawn sprite" << "\n";
                    continue;
                }

				//get the file name, (e.g. "bird.png") to use as label
                std::string label = path.filename().string();
                //if the filePath is just label, somehow the entity spawner unable to spawn the sprite
				//after asking help from ASC TAs, they suggest need to add "assets/" in-front of the label
                std::string filePath = std::string("assets/") + label;

				//spawn the sprite as an entity using entity spawner
                Framework::Entity entity = entitySpawner->SpawnSprite(
                    filePath,
                    Vector2D(0.0f, 0.0f),
                    Vector2D(1.0f, 1.0f)
				);
                std::cout << "[Drop] Spawned sprite from: " << path << " as entity" << entity.id << "\n";
				//move to next dropped file
				continue;
            }

			std::cerr << "[DropError] Unsupported file type: " << path << "\n";
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
		//set the window size and condition
        ImGui::SetNextWindowSize(ImVec2(320.0f, 420.0f), ImGuiCond_FirstUseEver);
		// begin the window, return if the window is closed
        if (!ImGui::Begin("Assets##Assets", &showAssets))
        {
			// I learned from ASC TA that need to call End() even if the window is collapsed/closed
            ImGui::End();
			// stop drawing this window
            return;
        }

		//draw the current path as separator text
        ImGui::SeparatorText(currentpath.string().c_str());
		// draw back button aligned to the right
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10);

		//Draw back button
        if (ImGui::Button("<##Back")) {
			// go back to previous path only if not in root path
            if (currentpath != rootpath) {
				// if previous path is not empty, go back to previous path
                if (!previouspath.empty()) {
                    
                    currentpath = previouspath;
					// update previous path to its parent path
                    previouspath = currentpath.parent_path();
                }
            }

        }
		// loop through all files and directories in the current path
        for (auto const& e : std::filesystem::directory_iterator(currentpath)) {

			// get the file path and label
            auto const path = e.path();
			// get only the filename part as label
            std::string const label = path.filename().string();
			// create ImGui label, add "->" prefix if it is a directory
            std::string const ImGuilabel = e.is_directory() ? "->" + label : label;
            //std::string const fullPath = path.string();
            std::string filePath = "assets/" + label;
			// create selectable item for the file/directory    
            if (ImGui::Selectable(ImGuilabel.c_str())) {

				//if it is a folder, navigate into the folder
                if (e.is_directory()) {
					// store the current path as previous path
                    previouspath = currentpath;
					// navigate into the folder
                    currentpath = e;
                }
                //if its a texture file, spawn a sprite entity
                if (IsTextureFile(path) && entitySpawner) {
                    
                    std::string filePath = "assets/" + label;
                    //entitySpawner->SpawnSprite(filePath, Vector2D(0.0f, 0.0f));

                }
				// Otherwise, if it is a level file, open the level file
                else if (IsLevelFile(path)) {
                    OpenLevelFromTxt(filePath, true);
                }
            }

            //drag and drop asset window
			// only allow drag and drop for texture files
            if (IsTextureFile(path))
            {
                //making it double click for now until we add drag and drop target
                if (ImGui::IsItemHovered()) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        if (entitySpawner) {
                            Framework::Entity entity = entitySpawner->SpawnSprite(
                                filePath,
                                Vector2D(0.0f, 0.0f),
                                Vector2D(1.0f, 1.0f)
                            );
                            std::cout << "[Drop] Spawned sprite from: " << filePath << " as entity" << entity.id << "\n";
                        }
                    }

                }

				// begin drag drop source
                if (ImGui::BeginDragDropSource()) {
					// put a payload of type "Sprite" into the drag drop source
                    ImGui::SetDragDropPayload("Sprite", &label, label.size());
					// display the label while dragging
                    ImGui::Text(label.c_str());
					// end drag drop source
                    ImGui::EndDragDropSource();
                }
            }
        }
    
		ImGui::End();
    }


    void ImGuiSystem::Update(float dt)
    {
        if (!enabled) {
            return;
        }

        //frameTime = dt;
        if (entityManager) {
            entityCount = static_cast<int>(entityManager->GetAllEntities().size());
        }

        static bool wantOpenModal = false;
        static bool wantSaveAsModal = false;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        frameTime = ImGui::GetIO().DeltaTime;


        // enable docking -jiahao
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
      //  if (ImGui::BeginDragDropTarget()) {
      //      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Sprite")) {
      //          if (payload->DataSize == sizeof(std::string)) {
      //              std::string filePath = *(std::string*)payload->Data;
      //              if (IsTextureFile(filePath)) {
      //                  // Handle texture file drop
      //                  if (entitySpawner) {
      //                      Framework::Entity entity = entitySpawner->SpawnSprite(
      //                          filePath,
      //                          Vector2D(0.0f, 0.0f),
      //                          Vector2D(1.0f, 1.0f)
      //                      );
      //                      std::cout << "[Drop] Spawned sprite from: " << filePath << " as entity" << entity.id << "\n";
						//}
      //              }
      //          }
      //      }
      //      ImGui::EndDragDropTarget();
      //  }


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

        // Show windows
        if (showEntityInspector) ShowEntityInspector();
        if (showSpawner) ShowSpawnerWindow();
        if (showDebug) ShowDebugWindow();
        if (showDemo) ImGui::ShowDemoWindow(&showDemo);
		// show asset window - jiahao
		if (showAssets) ShowAssetsWindow();
    }

    void ImGuiSystem::Render()
    {
        if (!enabled) {
            return;
        }

        if (!window) return;

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

        ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Entity Inspector##Inspector1", &showEntityInspector)) {
            ImGui::End();
            return;
        }

        std::vector<Entity> allEntities = entityManager->GetAllEntities();
        const int totalEntities = static_cast<int>(allEntities.size());

        ImGui::Text("Total Entities: %d", totalEntities);
        ImGui::Separator();

        // ============================================================================
        // PAGINATION CONTROLS
        // ============================================================================
        const int totalPages = (totalEntities + entitiesPerPage - 1) / entitiesPerPage; // Ceiling division

        // Clamp current page to valid range
        if (currentPage < 0) currentPage = 0;
        if (currentPage >= totalPages && totalPages > 0) currentPage = totalPages - 1;

        // Page navigation buttons
        if (totalPages > 1) {
            ImGui::Text("Page %d / %d", currentPage + 1, totalPages);

            // Previous page button
            if (ImGui::Button("< Prev##PagePrev")) {
                if (currentPage > 0) currentPage--;
            }

            ImGui::SameLine();

            // Next page button
            if (ImGui::Button("Next >##PageNext")) {
                if (currentPage < totalPages - 1) currentPage++;
            }

            ImGui::SameLine();

            // Jump to page input
            ImGui::SetNextItemWidth(60);
            int displayPage = currentPage + 1; // Show 1-indexed to user
            if (ImGui::InputInt("##PageNum", &displayPage, 0, 0)) {
                currentPage = displayPage - 1; // Convert back to 0-indexed
                if (currentPage < 0) currentPage = 0;
                if (currentPage >= totalPages) currentPage = totalPages - 1;
            }

            ImGui::Separator();
        }

        // ============================================================================
        // CALCULATE PAGE SLICE - WITHOUT std::min
        // ============================================================================
        const int startIdx = currentPage * entitiesPerPage;
        int endIdx = startIdx + entitiesPerPage;
        if (endIdx > totalEntities) {
            endIdx = totalEntities;
        }

        // Get entities for current page
        std::vector<Entity> pageEntities;
        if (startIdx < totalEntities) {
            pageEntities.assign(
                allEntities.begin() + startIdx,
                allEntities.begin() + endIdx
            );
        }

        ImGui::Text("Showing %d - %d of %d",
            startIdx + 1,
            endIdx,
            totalEntities);
        ImGui::Separator();

        // ============================================================================
        // DISPLAY ENTITIES FOR CURRENT PAGE
        // ============================================================================
        Entity entityToDelete = { 0 };
        bool shouldDelete = false;

        for (size_t i = 0; i < pageEntities.size(); ++i) {
            Entity entity = pageEntities[i];

            // Super unique ID
            ImGui::PushID(static_cast<int>(entity.id * 10000 + i));

            char label[128];
            snprintf(label, sizeof(label), "Entity %u", entity.id);

            if (ImGui::CollapsingHeader(label)) {

                if (entityManager->HasComponent<Transform>(entity)) {
                    auto& transform = entityManager->GetComponent<Transform>(entity);
                    ImGui::Text("Transform:");
                    ImGui::DragFloat2("Position##Pos", &transform.position.x, 0.01f, -10.0f, 10.0f);
                    Vector2D prevScale = transform.scale;
                    if (ImGui::DragFloat2("Scale##Scl", &prevScale.x, 0.01f, 0.01f, 10.0f))
                    {
                        transform.scale.x = prevScale.x;
                        transform.scale.y = prevScale.y;
                    }
                }

                if (entityManager->HasComponent<MeshRenderer>(entity))
                {
                    auto& meshRenderer = entityManager->GetComponent<MeshRenderer>(entity);
                    std::string testString = "Mesh Sprite: " + meshRenderer.spriteName;
                    ImGui::Text(testString.c_str());
                }

                if (entityManager->HasComponent<Movement>(entity)) {
                    auto& movement = entityManager->GetComponent<Movement>(entity);
                    ImGui::Text("Movement:");
                    ImGui::DragFloat2("Direction##Dir", &movement.direction.x, 0.01f, -1.0f, 1.0f);
                    ImGui::DragFloat("Speed##Spd", &movement.moveSpeed, 0.01f, 0.0f, 2.0f);
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

        // Handle deletion after UI rendering
        if (shouldDelete && entityToDelete.id != 0) {
            entityManager->DestroyEntity(entityToDelete);
            Framework::SpatialPartitioningRemove(entityToDelete);

            // If we deleted the last entity on the page, go to previous page
            if (pageEntities.size() == 1 && currentPage > 0) {
                currentPage--;
            }
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

        //ImGui::BeginDisabled();
        //{
        //    auto p = graphicsSystem->GetCamera().GetPosition();
        //    ImGui::DragFloat2("Cam Pos", glm::value_ptr(p));
        //}
        //ImGui::EndDisabled();

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
        ImGui::BulletText("Key 3: Go to level3");
        ImGui::BulletText("key 0: Reset Camera");
        ImGui::BulletText("click above menu Editor->Play to activate play mode");
        ImGui::BulletText("WASD: Move");
        ImGui::BulletText("SPACE: Shoot Up");
        ImGui::BulletText("SHIFT: Shoot Down");
        ImGui::BulletText("Q/ESC: Quit");



        ImGui::End();
    }

} // namespace Framework