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

        //object picking
        UpdatePicking();

        UpdateEntityDragging();
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

                collider = Collider::create_circle(
                    cc.radius,
                    transform.position + cc.offset
                );

                hasCollider = true;
            }

            else if (entityManager->HasComponent<BoxCollider>(e)) {
                auto& bc = entityManager->GetComponent<BoxCollider>(e);
                collider = Collider::create_rect(
                    bc.size.x,
                    bc.size.y,
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

        if (input->IsKeyPressed(MOUSE_LEFT)) {
            if (selectedEntity.GetID() != INVALID_ENTITY &&
                entityManager->HasComponent<Framework::Transform>(selectedEntity)) {

                float mouseX = 0.0f;
                float mouseY = 0.0f;

                input->GetMousePosition(mouseX, mouseY);

                Vector2D mouseWorld = ui->ScreenToWorld(mouseX, mouseY);

                auto& transform = entityManager->GetComponent<Framework::Transform>(selectedEntity);

                dragOffset = transform.position - mouseWorld;

                draggingEntity = selectedEntity;
                isDraggingEntity = true;
            }
            else {
                isDraggingEntity = false;
                draggingEntity = Framework::Entity{ INVALID_ENTITY };
            }
        }
        if (isDraggingEntity && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (draggingEntity.GetID() == INVALID_ENTITY || 
                !entityManager->HasComponent<Framework::Transform>(draggingEntity)) {
                isDraggingEntity = false;
                return;
            }

            float mouseX = 0.0f;
            float mouseY = 0.0f;

            input->GetMousePosition(mouseX, mouseY);
            Vector2D mouseWorld = ui->ScreenToWorld(mouseX, mouseY);

            auto& transform =
                entityManager->GetComponent<Framework::Transform>(draggingEntity);
            transform.position = mouseWorld + dragOffset;
        }

        if (isDraggingEntity && ImGui::IsMouseReleased(ImGuiMouseButton_Left)){
            isDraggingEntity = false;
            RebuildSpatialPartition();
        }
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