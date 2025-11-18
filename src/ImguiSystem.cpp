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
#include <build/_deps/glfw-src/include/GLFW/glfw3.h>
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

    void ImGuiSystem::Update(float dt)
    {
        (void)dt;
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
                //prefab window - kahyan
                ImGui::MenuItem("Prefabs", nullptr, &showPrefabWindow);
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

        //ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
        //ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);

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
            ImGui::PushID(static_cast<int>(entity.id + i));

            char label[128];
            snprintf(label, sizeof(label), "Entity %u", static_cast<int>(i+1));

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

                //prefab save/export kahyan
                static char prefabBuffer[128] = "";
                ImGui::Separator();
                ImGui::Text("Save Prefab:");
                ImGui::SetNextItemWidth(160.0f);
                ImGui::InputText("##PrefabName", prefabBuffer, IM_ARRAYSIZE(prefabBuffer));

                ImGui::SameLine();
                if (ImGui::Button("Export##ExportBtn")) {
                    if (!entity.IsValid()) {
                        std::cout << "[Prefab] Not saved. Entity is invalid.\n";
                    }
                    else if (prefabBuffer[0] == '\0') {
                        std::cout << "[Prefab] Not saved. Name is empty.\n";
                    }
                    else {
                        //IMPORTANT: save into assets/prefabs/
                        std::string dir = "assets/prefabs/";
                        std::string filePath = dir + std::string(prefabBuffer) + ".prefab";

                        // Make sure folder exists
                        try {
                            std::filesystem::create_directories(dir);
                        }
                        catch (...) {
                            std::cout << "[Prefab] Failed to create folder: " << dir << "\n";
                        }

                        if (PrefabSerializer::SavePrefab(*entityManager, entity, filePath)) {
                            std::cout << "[Prefab] Saved: " << filePath << " from entity " << entity.id << "\n";
                        }
                        else {
                            std::cout << "[Prefab] Failed to save: " << filePath << "\n";
                        }
                    }

                    // Clear input after click
                    memset(prefabBuffer, 0, sizeof(prefabBuffer));
                }

                ImGui::Separator();

                // Unique button ID
                if (ImGui::Button("Delete##DelBtn")) {
                    entityToDelete = entity;
                    shouldDelete = true;
                }

                ImGui::SameLine();

                char savePrefabBtnLabel[64];
                snprintf(savePrefabBtnLabel, sizeof(savePrefabBtnLabel), "Save Prefab##SavePrefab%u", entity.id);

                if (ImGui::Button(savePrefabBtnLabel)) {
                    // Generate filename from entity ID
                    std::string prefabPath = "assets/prefabs/entity_" + std::to_string(entity.id) + ".prefab";

                    // Create directory if needed
                    std::filesystem::create_directories("assets/prefabs");

                    bool saved = PrefabSerializer::SavePrefab(*entityManager, entity, prefabPath);

                    if (saved) {
                        std::cout << "[Inspector] ✅ Saved entity " << entity.id << " as prefab\n";
                    }
                    else {
                        std::cerr << "[Inspector] ❌ Failed to save prefab\n";
                    }
                }

                // Show prefab source if entity came from a prefab
                std::string prefabSource = Framework::PrefabInstanceTracker::Get().GetPrefabOf(entity);
                if (!prefabSource.empty()) {
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "From: %s",
                        std::filesystem::path(prefabSource).filename().string().c_str());
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
        // SECTION 3: Prefab Instance Tracking
        // ========================================================================
        ImGui::Text("Prefab Instances:");
        ImGui::Spacing();

        // Show tracked instances
        if (selectedPrefabIdx >= 0 && selectedPrefabIdx < static_cast<int>(prefabFiles.size())) {
            std::string prefabPath = "assets/prefabs/" + prefabFiles[selectedPrefabIdx];
            auto instances = Framework::PrefabInstanceTracker::Get().GetInstancesOf(prefabPath);

            ImGui::Text("Instances of '%s': %zu", prefabFiles[selectedPrefabIdx].c_str(), instances.size());

            if (instances.size() > 0) {
                if (ImGui::BeginListBox("##InstanceList", ImVec2(-1, 100))) {
                    for (auto& inst : instances) {
                        std::string label = "Entity " + std::to_string(inst.GetID());
                        ImGui::Selectable(label.c_str(), false);
                    }
                    ImGui::EndListBox();
                }
            }
        }

        // Clear tracking button
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
} // namespace Framework