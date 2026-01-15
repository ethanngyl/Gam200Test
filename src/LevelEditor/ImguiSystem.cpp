/*
===============================================================================
File:        ImGuiSystem.cpp
Author:      Ethan Ng, Jiahao Zhou, Sim Kah Yan
Email:       n.ethanyongle@digipen.edu, jiahao.zhou@digipen.edu, kahyan.sim@digipen.edu
Date:        2025-11-07
Contribution: 40%(Ethan), 50%(Jiahao), 10%(kahyan)
-------------------------------------------------------------------------------
Modified: 2025-11-30


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
- Menu  File: Open / Open… / Save / Save As… / Exit
- Menu  Windows: toggle editor panels
- Menu  Editor: Play (when stopped) / Stop (when playing)
- Assets Browser: double-click texture to spawn at origin; drag filename to future
  drop targets; click “<” to go up one folder
- OS Drag-&-Drop onto window:
  • .txt  load level (clears scene if requested
  • .png, .jpg, .jpeg  spawn sprite

Notes:


Safety:

*/

#include "Precompiled.h"
#include "ImGuiSystem.h"
#include "EntitySpawner.h"
#include "AudioSystem.h"
#include "AudioLoader.h"
#include "Pathfinding.h"
#include "PrefabSerializer.h"
#include "PrefabTracker.h"
#include "SaveLoadSystem.h"
#include <string.h>
#include <GlobalPauseManager.h>
#include <regex> 

namespace Framework {

    static bool wantOpenModal = false;
    static bool wantSaveAsModal = false;
    static bool wantOpenSceneModal = false;
    static bool wantSaveSceneAsModal = false;
    static std::string currentScenePath = "";

    static int GetGsmStateFromLuaName(const std::string& lowerName) {
        if (lowerName.find("level3") != std::string::npos) return LEVEL_3;
        if (lowerName.find("level2") != std::string::npos) return LEVEL_2;
        if (lowerName.find("levelselect") != std::string::npos) return Level_select;
        if (lowerName.find("mainmenu") != std::string::npos) return mainMenu;
        if (lowerName.find("tutorial") != std::string::npos) return TUTORIAL;
        if (lowerName.find("end") != std::string::npos) return LEVEL_END;

        return -1;
    }

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
        // Prevent double shutdown
        if (!imguiInitialized) {
            std::cout << "[ImGui] Already shutdown or never initialized - skipping\n";
            return;
        }

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

        imguiInitialized = false;  // Mark as shutdown
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
            imguiInitialized = false;
            return;
        }

        glfwMakeContextCurrent(window);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking!
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Disabled - causes frame count issues
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

        imguiInitialized = true;  // Mark as successfully initialized
        std::cout << "[ImGui] Initialization complete\n";
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
                float rot = 0.0f;
                if (iss >> px >> py >> sx >> sy ) {

                    if (!(iss >> rot)) {
                        iss.clear(); // Clear the error state if the 5th read failed
                    }

                    entityManager->AddComponent<Framework::Transform>(Entity);
                    auto& transform = entityManager->GetComponent<Framework::Transform>(Entity);
                    transform.position = Vector2D(px, py);
                    transform.scale = Vector2D(sx, sy);
                    transform.rotation = rot;
                }
                else {
                    std::cerr << "[ImGuiError] parsing Transform at line " << lineNumber << "\n";
                    continue;
                }
            }

            if (word == "Sprite") {
                std::string name;
                float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;  // Default white tint
                iss >> name;
                // Try to read optional tint values (r g b a)
                iss >> r >> g >> b >> a;

                if (!name.empty()) {
                    entityManager->AddComponent<Framework::Sprite>(Entity);
                    auto& sprite = entityManager->GetComponent<Framework::Sprite>(Entity);
                    //sprite.texturePath =  name;
                    std::string label = std::filesystem::path(name).filename().string();
                    sprite.texturePath = std::string("assets/") + label;
                    sprite.tint = glm::vec4(r, g, b, a);
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
                float frameTime1 = 0.0f;
                int loopInt = 1;
                float uvShrinkPx = 0.0f;

                if (iss >> rows >> columns >> frameCount >> frameTime1 >> loopInt >> uvShrinkPx) {
                    entityManager->AddComponent<Framework::SpriteAnimation>(Entity);
                    auto& spriteAnimation = entityManager->GetComponent<Framework::SpriteAnimation>(Entity);
                    spriteAnimation.rows = rows;
                    spriteAnimation.columns = columns;
                    spriteAnimation.frameCount = frameCount;
                    spriteAnimation.frameTime = frameTime1;
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
            if (word == "AudioSource") {
                std::string name;
                float vol, pitch;
                int loop, playOnStart;
                // Read the data 
                if (iss >> name >> vol >> pitch >> loop >> playOnStart) {
                    entityManager->AddComponent<Framework::AudioSource>(Entity);
                    auto& audio = entityManager->GetComponent<Framework::AudioSource>(Entity);
                    audio.soundName = name;
                    audio.volume = vol;
                    audio.pitch = pitch;
                    audio.loop = (loop != 0);
                    audio.playOnStart = (playOnStart != 0);
                }
            }

            if (word == "Script") {
                std::string path;
                // Read the script path
                if (iss >> path) {
                    entityManager->AddComponent<Framework::ScriptComponent>(Entity);
                    auto& script = entityManager->GetComponent<Framework::ScriptComponent>(Entity);
                    script.scriptPath = path;
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
                    << transform.scale.x << " " << transform.scale.y << " " 
					<< transform.rotation <<"\n";
            }

            //this if else condition is to check if entity has sprite component
            if (entityManager->HasComponent<MeshRenderer>(entity)) {
                // get reference to the meshRenderer component
                // in renderring system, there is also meshrenderer component to generate image
                // some entity may not have
                auto& meshRenderer = entityManager->GetComponent<MeshRenderer>(entity);
                //write the sprite name with tint
                if (!meshRenderer.spriteName.empty()) {
                    writeFile << "Sprite " << meshRenderer.spriteName << " "
                             << meshRenderer.tint.r << " " << meshRenderer.tint.g << " "
                             << meshRenderer.tint.b << " " << meshRenderer.tint.a << "\n";
                }
            }

            // this if else condition is to check if entity has sprite component
            if (entityManager->HasComponent<Sprite>(entity)) {
                // get reference to the sprite component
                auto& sprite = entityManager->GetComponent<Sprite>(entity);
                if (!sprite.texturePath.empty()) {
                    // Save sprite with tint values (r g b a)
                    writeFile << "Sprite " << sprite.texturePath << " "
                             << sprite.tint.r << " " << sprite.tint.g << " "
                             << sprite.tint.b << " " << sprite.tint.a << "\n";
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

        // Cache directory contents (only scan when path changes)
        static std::filesystem::path cachedPath;
        static std::vector<std::filesystem::directory_entry> cachedEntries;

        if (cachedPath != currentpath) {
            cachedPath = currentpath;
            cachedEntries.clear();
            for (auto const& e : std::filesystem::directory_iterator(currentpath)) {
                cachedEntries.push_back(e);
            }
        }

        // Loop through cached directory entries
        for (auto const& e : cachedEntries) {
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

                            RecordCreationStep(entity);//Record the creation for Undo

                            std::cout << "[Drop] Spawned sprite from: " << filePath
                                << " as entity" << entity.id << "\n";
                        }
                    }
                }

                /*if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("Sprite", &label, label.size());
                    ImGui::Text(label.c_str());
                    ImGui::EndDragDropSource();
                }*/

                if (ImGui::BeginDragDropSource())
                {
                    // Send full texture path, e.g. "assets/player.png"
                    ImGui::SetDragDropPayload(
                        "Sprite",
                        filePath.c_str(),
                        filePath.size() + 1
                    );

                    ImGui::Text("%s", label.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            // ============================================================================ 
            // detail: Handles prefab interaction in the Assets window:
            //         - Detects .prefab/.json files
            //         - Double-click spawns a prefab instance
            //         - Drag-and-drop exposes a "Prefab" payload for drop zones
            // author: Sim Kah Yan 
            // ============================================================================
            if (path.extension() == ".prefab") {
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
                    ImGui::Text("Prefab: %s", label.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            if (path.extension() == ".json")
            {
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Double-click to load scene (JSON)");

                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        std::string scenePath = path.string(); // should already be like "assets/saves/xxx.json"

                        SaveLoadSystem::SetGraphicsSystem(graphicsSystem);
                        if (SaveLoadSystem::LoadFromJSON(scenePath, entityManager, true))
                        {
                            std::cout << "[Assets] Loaded scene JSON: " << scenePath << "\n";
                            RebuildSpatialPartition();
                        }
                        else
                        {
                            std::cerr << "[Assets] Failed to load scene JSON: " << scenePath << "\n";
                        }
                    }
                }
            }

            // =================================================================
            // LUA SCRIPT DOUBLE-CLICK LOADING
            // =================================================================
            if (path.extension() == ".lua") {
                if (ImGui::IsItemHovered()) {
                    

                    if (ImGui::IsMouseDoubleClicked(0)) {
                        std::string filename = path.filename().string();
                        std::string fullPath = path.string();

                        // Check if it is a LEVEL script (contains "level" or "menu")
                        std::string lowerName = filename;
                        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                        bool isLevel = (lowerName.find("level") != std::string::npos) ||
                            (lowerName.find("menu") != std::string::npos);

                        if (isLevel) {
                            std::cout << "[Assets] Double-click detected: Loading " << filename << "\n";

							currentLuaLevelPath = fullPath;
                            pendingLuaGsmState = GetGsmStateFromLuaName(lowerName);

                            // 1. Clear Game Viewport
                            if (entityManager) {
                                entityManager->ClearAllEntities();
                                entityManager->ResetEntityIDCounter();
                            }

                            // 2. Load the new Level
                            // Pass 'true' to tell the script we are in Editor Mode (keep UI enabled)
                            Framework::LevelLoader::GetInstance().LoadLevel(fullPath, true);

                            // 3. FORCE Editor UI to stay ON (Safety override)
                            this->enabled = true;

                            // 4. Reset Camera (Optional)
                            if (graphicsSystem) graphicsSystem->SetCameraPosition(glm::vec3(0, 0, 0));
                        }
                        else {
                            std::cout << "[Assets] Ignored double-click on non-level script: " << filename << "\n";
                        }
                    }
                }
            }
        }

        ImGui::End();  // Only one End() call at the very end
    }

    // ============================================================================
    // This function converts the mouse cursor's screen position (pixels) into 
    // Game World coordinates, accounting for the camera and editor viewport.
    // author: jiahao.zhou@digipen
    // ============================================================================
    Framework::Vector2D ImGuiSystem::EditorScreenWorld() {
        // Safety Check: Ensure the Core engine exists before accessing it
        if (!Framework::CORE) {
            return Framework::Vector2D{ 0.0f, 0.0f };
        }
        // Get references to the Graphics and Window systems
        auto graphics = Framework::CORE->GetGraphicsSystem();
        auto windowSystem = Framework::CORE->GetWindowSystem();
        // Safety Check: If either system is missing, we can't calculate coordinates
        if (!graphics || !windowSystem) {
            return Framework::Vector2D(0.0f, 0.0f);
        }
        // Get the actual OS window handle (GLFW window)
        GLFWwindow* window1= windowSystem->GetWindow();
        if (!window1) {
            return Framework::Vector2D(0.0f, 0.0f);
        }

        // 1. Get Global Mouse Position
        ImVec2 mousePos = ImGui::GetMousePos();
        float mouseX = mousePos.x;
        float mouseY = mousePos.y;

        // 2. Determine Screen Dimensions for NDC Calculation
        float screenX = 0.0f;
        float screenY = 0.0f;
        float screenW = 0.0f;
        float screenH = 0.0f;

        // Check if we are rendering to the ImGui Viewport
        if (IsRenderingToViewport()) {
            // --- VIEWPORT MODE ---
            screenX = m_viewportPos.x;
            screenY = m_viewportPos.y;
            screenW = m_viewportSize.x;
            screenH = m_viewportSize.y;
        }
        else {
            // --- FULLSCREEN MODE (Fallback) ---
            int w, h;
            glfwGetWindowSize(window1, &w, &h);
            screenW = (float)w;
            screenH = (float)h;
        }

        // 3. Convert to Normalized Device Coordinates (NDC) [-1 to 1]
        float localX = mouseX - screenX;
        float localY = mouseY - screenY;

        float ndcX = (2.0f * localX / screenW) - 1.0f;

        // FLIP Y: ImGui (0=Top) vs OpenGL (0=Bottom)
        float ndcY = 1.0f - (2.0f * localY / screenH);

        // 4. Unproject using the Active Camera
        Camera& camera = CORE->IsPlaying() ? graphics->GetCamera() : graphics->GetEditorCamera();
        // Get the inverse of the View-Projection matrix.
        // This allows us to reverse the rendering process (Screen -> World).
        glm::mat4 invViewProj = glm::inverse(camera.GetViewProjectionMatrix());
        // Multiply the NDC coordinate by the inverse matrix to get the World Position.
        glm::vec4 worldPos = invViewProj * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
        // Return the final X, Y coordinates in the game world.
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


        // ========================================================================
        // SETUP DOCKSPACE - Handle docking layout and menu bar
        // ========================================================================
        SetupDockSpace();
    }

    void ImGuiSystem::Update(float dt)
    {
        (void)dt;
        if (pendingToggle) {
            enabled = !enabled;
            pendingToggle = false;
            std::cout << "[ImGuiSystem] Toggled to: " << (enabled ? "ON" : "OFF") << "\n";
        }

        // Auto-Switch Screen Mode
        if (Framework::CORE && Framework::CORE->GetWindowSystem()) {
            Framework::CORE->GetWindowSystem()->SetFullScreen(!enabled);
        }

        if (!enabled) {
            return;
        }

        // This ensures the game renders into the Viewport Window, not over your Editor!
        /*if (graphicsSystem) {
            if (IsRenderingToViewport()) {
                // Draw ONLY to the Game Viewport image
                graphicsSystem->SetRenderTarget(viewportFBO, viewportWidth, viewportHeight);
            }
            else {
                // Draw to main screen
                graphicsSystem->ClearRenderTarget();
            }
        }*/

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        frameTime = ImGui::GetIO().DeltaTime;
        if (entityManager) {
            entityCount = static_cast<int>(entityManager->GetAllEntities().size());
        }


        //ImGui_ImplOpenGL3_NewFrame();
        //ImGui_ImplGlfw_NewFrame();

        

        // ========================================================================
        // SETUP DOCKSPACE - Handle docking layout and menu bar
        // ========================================================================
        SetupDockSpace();

        //object picking
        UpdatePicking();

        UpdateEntityDragging();
        InputSystem* input = CORE->GetInputSystem();
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
        if (CORE->IsEditorMode() && entityManager) {
            InputSystem* input = Framework::CORE->GetInputSystem();

            if (input && selectedEntity.IsValid()) {
                if (input->IsKeyPressed(KEY_DELETE))
                {
                    std::cout << "[ImGui] Delete key pressed on entity "
                        << selectedEntity.id << "\n";

                    RecordDeletionStep(selectedEntity);

                    SpatialPartitioningRemove(selectedEntity);

                    entityManager->DestroyEntity(selectedEntity);


                    selectedEntity = Framework::Entity{};
                    draggingEntity = Framework::Entity{};
                    isDraggingEntity = false;
                }
            }

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

        if (wantOpenSceneModal)
        {
            ImGui::OpenPopup("Open Scene...");
            wantOpenSceneModal = false;
        }

        if (wantSaveSceneAsModal)
        {
            ImGui::OpenPopup("Save Scene As...");
            wantSaveSceneAsModal = false;
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

        // ============================================================================
        // JSON: Open Scene... (file-based)
        // ============================================================================
        if (ImGui::BeginPopupModal("Open Scene...", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char openSceneBuffer[256] = "";
            static bool openSceneError = false;
            static std::string openSceneErrorMsg = "";

            extern int current;
            std::string levelName = "Unknown";
            switch (current)
            {
            case mainMenu: levelName = "MainMenu"; break;
            case Level_select: levelName = "LevelSelect"; break;
            case LEVEL_2: levelName = "Level2"; break;
            case LEVEL_3: levelName = "Level3"; break;
            case TUTORIAL: levelName = "Tutorial"; break;
            case LEVEL_END: levelName = "LevelEnd"; break;
            }

            if (ImGui::IsWindowAppearing())
            {
                std::string defaultPath = currentScenePath.empty()
                    ? ("assets/saves/" + levelName + "_save.json")
                    : currentScenePath;

                std::snprintf(openSceneBuffer, sizeof(openSceneBuffer), "%s", defaultPath.c_str());
                openSceneError = false;
                openSceneErrorMsg.clear();
            }

            ImGui::InputText("Path", openSceneBuffer, sizeof(openSceneBuffer));

            if (openSceneError)
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", openSceneErrorMsg.c_str());
            }

            if (ImGui::Button("Open"))
            {
                currentScenePath = openSceneBuffer;

                SaveLoadSystem::SetGraphicsSystem(graphicsSystem);
                if (SaveLoadSystem::LoadFromJSON(currentScenePath, entityManager, true))
                {
                    LOG_INFO("ImGuiSystem", "Opened scene (JSON): %s", currentScenePath.c_str());
                    RebuildSpatialPartition();
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    openSceneError = true;
                    openSceneErrorMsg = "Invalid path or JSON format: " + std::string(openSceneBuffer);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // ============================================================================
        // JSON: Save Scene As... (file-based)
        // ============================================================================
        if (ImGui::BeginPopupModal("Save Scene As...", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char saveSceneBuffer[256] = "";
            static bool saveSceneError = false;
            static std::string saveSceneErrorMsg = "";

            extern int current;
            std::string levelName = "Unknown";
            switch (current)
            {
            case mainMenu: levelName = "MainMenu"; break;
            case Level_select: levelName = "LevelSelect"; break;
            case LEVEL_2: levelName = "Level2"; break;
            case LEVEL_3: levelName = "Level3"; break;
            case TUTORIAL: levelName = "Tutorial"; break;
            case LEVEL_END: levelName = "LevelEnd"; break;
            }

            if (ImGui::IsWindowAppearing())
            {
                std::string defaultPath = currentScenePath.empty()
                    ? ("assets/saves/" + levelName + "_save.json")
                    : currentScenePath;

                std::snprintf(saveSceneBuffer, sizeof(saveSceneBuffer), "%s", defaultPath.c_str());
                saveSceneError = false;
                saveSceneErrorMsg.clear();
            }

            ImGui::InputText("Path", saveSceneBuffer, sizeof(saveSceneBuffer));

            if (saveSceneError)
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", saveSceneErrorMsg.c_str());
            }

            if (ImGui::Button("Save"))
            {
                currentScenePath = saveSceneBuffer;

                SaveLoadSystem::SetGraphicsSystem(graphicsSystem);
                if (SaveLoadSystem::SaveToJSON(currentScenePath, entityManager, levelName))
                {
                    LOG_INFO("ImGuiSystem", "Saved scene (JSON): %s", currentScenePath.c_str());
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    saveSceneError = true;
                    saveSceneErrorMsg = "Could not save to path: " + std::string(saveSceneBuffer);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
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

            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "ERROR");
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

        if (showAudioNamePopup) {
            ImGui::OpenPopup("Import Audio Asset");
        }

        if (ImGui::BeginPopupModal("Import Audio Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("File detected: %s", pendingAudioPath.filename().string().c_str());
            ImGui::Spacing();

            ImGui::Text("Enter a unique Key Name for this audio:");
            // Input field for the key (e.g., "bgm_boss", "sfx_jump")
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::InputText("##AudioKey", newAudioKeyBuffer, sizeof(newAudioKeyBuffer));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // --- IMPORT BUTTON ---
            if (ImGui::Button("Import & Load", ImVec2(120, 0))) {
                if (audioSystem && strlen(newAudioKeyBuffer) > 0) {
                    std::string keyName = newAudioKeyBuffer;
					std::string ext = pendingAudioPath.extension().string();
					std::string fileName = keyName + ext;

                    // 1. Copy file to assets folder
                    std::filesystem::path destPath = std::filesystem::path("assets") / fileName;
                    bool copySuccess = true;

                    if (!std::filesystem::exists(destPath)) {
                        try {
                            std::filesystem::copy_file(pendingAudioPath, destPath);
                            std::cout << "[Import] Copied file to: " << destPath << "\n";
                        }
                        catch (const std::exception& e) {
                            std::cerr << "[Import] Copy failed: " << e.what() << "\n";
                            copySuccess = false;
                        }
                    }

                    // 2. Update JSON and Reload if copy succeeded (or file existed)
                    if (copySuccess) {
                        // Use the user-entered KEY (newAudioKeyBuffer) instead of just the filename
                        std::cout << "DEBUG: Calling AddAudioToJSON with Key='" << keyName << "' and File='" << fileName << "'\n";
                        bool added = AddAudioToJSON(keyName, fileName);

                        if (added) {
                            // 3. Reload Audio System
                            audioSystem->ReloadAudioLibrary();
                            std::cout << "[Import] Audio library reloaded with key: " << keyName << "\n";

                            // Optional: Play it to confirm
                            audioSystem->PlaySound(keyName.c_str(), false);
                        }
                    }

                    showAudioNamePopup = false;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();

            // --- CANCEL BUTTON ---
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                showAudioNamePopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // show prefab window - kahyan
        if (showPrefabWindow) ShowPrefabWindow();
        ShowScriptBrowserPopup();
        ShowLevelBrowserPopup();
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

        // Handle multi-viewport (required when ViewportsEnable is set)
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
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

        if (entityManager->HasComponent<ScriptComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Script Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& sc = entityManager->GetComponent<ScriptComponent>(selectedEntity);

                ImGui::Text("Script Path:");
                ImGui::SameLine();

                char buffer[512];
                strncpy(buffer, sc.scriptPath.c_str(), 511);
                buffer[511] = '\0';

                ImGui::PushItemWidth(-100);  
                if (ImGui::InputText("##ScriptPath", buffer, 512)) {
                    sc.scriptPath = buffer;
                }
                ImGui::PopItemWidth();

                ImGui::SameLine();

                if (ImGui::Button("Browse...", ImVec2(90, 0))) {
                    entityPendingScriptAssignment = selectedEntity;
                    showScriptBrowser = true;
                }

                ImGui::Spacing();
                if (!sc.scriptPath.empty()) {
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), " Script: %s", sc.scriptPath.c_str());

                    if (sc.initialized) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "(Initialized)");
                    }
                }
                else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "⚠ No script assigned");
                }

                if (sc.initialized) {
                    ImGui::Spacing();
                    ImGui::Text("Callbacks:");
                    ImGui::SameLine();
                    if (sc.hasOnInit) ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "[OnInit]");
                    ImGui::SameLine();
                    if (sc.hasOnUpdate) ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "[OnUpdate]");
                    ImGui::SameLine();
                    if (sc.hasOnDestroy) ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "[OnDestroy]");
                }

                ImGui::Separator();
            }
        }

        std::vector<Entity> allEntities = entityManager->GetAllEntities();
        const int totalEntities = static_cast<int>(allEntities.size());

        // ========================================================================
        // GLOBAL SETTINGS
        // ========================================================================
        if (ImGui::CollapsingHeader("Global Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Master Volume Slider
            static float masterVolume = AudioLoader::GetSettings().masterVolume;

            ImGui::Text("Audio Settings");
            if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f, "%.2f")) {
                // Update audio system
                if (audioSystem) {
                    audioSystem->SetMasterVolume(masterVolume);
                }
                // Save to audio config JSON file
                AudioLoader::SetMasterVolume(masterVolume);
            }

            ImGui::Separator();
        }

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

        // Component removal tracking
        struct ComponentRemoval {
            Entity entity;
            std::string componentType;
        };
        std::vector<ComponentRemoval> componentsToRemove;

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

            snprintf(label, sizeof(label), "Entity %u%s###Entity_%u", entity.GetID(), entityInfo.c_str(), entity.GetID());

            //Check if this entity is the one selected via Picking
            bool isSelected = (selectedEntity.GetID() == entity.GetID());

            //If selected, push a blue color for the header
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.59f, 0.98f, 1.00f)); // Blue highlight
                ImGui::SetNextItemOpen(true, ImGuiCond_Once); // Auto-open the tab when clicked
            }

            if (ImGui::CollapsingHeader(label)) {

                // ==================================================================
                // TRANSFORM COMPONENT
                // ==================================================================
                if (entityManager->HasComponent<Transform>(entity)) {
                    if (ImGui::TreeNode("Transform##TransformNode")) {
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##RemoveTransform")) {
                            componentsToRemove.push_back({ entity, "Transform" });
                        }

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
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##RemoveSprite")) {
                            componentsToRemove.push_back({ entity, "Sprite" });
                        }

                        auto& sprite = entityManager->GetComponent<Sprite>(entity);

                        // Display texture path
                        char pathBuffer[256];
                        strncpy_s(pathBuffer, sizeof(pathBuffer), sprite.texturePath.c_str(), _TRUNCATE);
                        pathBuffer[sizeof(pathBuffer) - 1] = '\0';

                        if (ImGui::InputText("Texture Path", pathBuffer, sizeof(pathBuffer))) {
                            sprite.texturePath = pathBuffer;
                        }

                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Sprite"))
                            {
                                const char* droppedPath = static_cast<const char*>(payload->Data);
                                if (droppedPath && payload->DataSize > 0)
                                {
                                    sprite.texturePath = droppedPath;
                                }
                            }
                            ImGui::EndDragDropTarget();
                        }

                        ImGui::DragInt("Layer", &sprite.layer, 1, -100, 100);

                        // Tint color picker
                        float tint[4] = { sprite.tint.r, sprite.tint.g, sprite.tint.b, sprite.tint.a };
                        if (ImGui::ColorEdit4("Tint##SpriteTint", tint)) {
                            sprite.tint.r = tint[0];
                            sprite.tint.g = tint[1];
                            sprite.tint.b = tint[2];
                            sprite.tint.a = tint[3];
                        }

                        ImGui::TreePop();
                    }
                }

                // ==================================================================
                // MESH RENDERER COMPONENT
                // ==================================================================
                if (entityManager->HasComponent<MeshRenderer>(entity)) {
                    if (ImGui::TreeNode("MeshRenderer##MeshRendererNode")) {
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##RemoveMeshRenderer")) {
                            componentsToRemove.push_back({ entity, "MeshRenderer" });
                        }

                        auto& meshRenderer = entityManager->GetComponent<MeshRenderer>(entity);

                        // Sprite name
                        char nameBuffer[256];
                        //strncpy(nameBuffer, meshRenderer.spriteName.c_str(), sizeof(nameBuffer) - 1);
                        strncpy_s(nameBuffer, sizeof(nameBuffer), meshRenderer.spriteName.c_str(), _TRUNCATE);
                        nameBuffer[sizeof(nameBuffer) - 1] = '\0';

                        if (ImGui::InputText("Sprite Name", nameBuffer, sizeof(nameBuffer))) {
                            meshRenderer.spriteName = nameBuffer;
                        }

                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Sprite"))
                            {
                                const char* droppedPath = static_cast<const char*>(payload->Data);
                                if (droppedPath && payload->DataSize > 0)
                                {
                                    // Use full path so GraphicsSystemV2 can LoadTexture(droppedPath)
                                    meshRenderer.spriteName = droppedPath;

                                    meshRenderer.texture = TextureHandle();

                                    // Optional: keep Sprite component in sync if it exists
                                    if (entityManager->HasComponent<Sprite>(entity))
                                    {
                                        auto& spriteFromRenderer =
                                            entityManager->GetComponent<Sprite>(entity);
                                        spriteFromRenderer.texturePath = droppedPath;
                                    }
                                }
                            }
                            ImGui::EndDragDropTarget();
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
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##RemoveMovement")) {
                            componentsToRemove.push_back({ entity, "Movement" });
                        }

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
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##RemoveBoxCollider")) {
                            componentsToRemove.push_back({ entity, "BoxCollider" });
                        }

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
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##RemoveCircleCollider")) {
                            componentsToRemove.push_back({ entity, "CircleCollider" });
                        }

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
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##RemoveAudioSource")) {
                            componentsToRemove.push_back({ entity, "AudioSource" });
                        }

                        auto& audio = entityManager->GetComponent<AudioSource>(entity);

                        // Display audio name
                        char audioBuffer[256];
                        //strncpy(audioBuffer, audio.soundName.c_str(), sizeof(audioBuffer) - 1);
                        strncpy_s(audioBuffer, sizeof(audioBuffer), audio.soundName.c_str(), _TRUNCATE);
                        audioBuffer[sizeof(audioBuffer) - 1] = '\0';

                        if (ImGui::InputText("Sound Name", audioBuffer, sizeof(audioBuffer))) {
                            audio.soundName = audioBuffer;
                        }

                        ImGui::DragFloat("Volume", &audio.volume, 0.01f, 0.0f, 1.0f);
                        ImGui::DragFloat("Pitch", &audio.pitch, 0.01f, 0.1f, 3.0f);
                        ImGui::Checkbox("Loop", &audio.loop);
                        ImGui::Checkbox("Play", &audio.playOnStart);

                        ImGui::TreePop();
                    }
                }

                // ==================================================================
                                // SCRIPT COMPONENT
                                // ==================================================================
                if (entityManager->HasComponent<ScriptComponent>(entity)) {
                    if (ImGui::TreeNode("Script##ScriptNode")) {
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove##RemoveScript")) {
                            componentsToRemove.push_back({ entity, "ScriptComponent" });
                        }

                        auto& script = entityManager->GetComponent<ScriptComponent>(entity);

                        // Display script path with Browse button
                        ImGui::Text("Script Path:");

                        // Editable script path
                        char pathBuffer[512];
                        strncpy_s(pathBuffer, sizeof(pathBuffer), script.scriptPath.c_str(), _TRUNCATE);
                        pathBuffer[sizeof(pathBuffer) - 1] = '\0';

                        // Input field takes most of the width
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100);
                        if (ImGui::InputText("##ScriptPath", pathBuffer, sizeof(pathBuffer))) {
                            script.scriptPath = pathBuffer;
                        }

                        // Browse button on the same line
                        ImGui::SameLine();
                        if (ImGui::Button("Browse...##BrowseScript", ImVec2(90, 0))) {
                            entityPendingScriptAssignment = entity;
                            showScriptBrowser = true;
                        }

                        // Display script status with color coding
                        ImGui::Spacing();
                        if (!script.scriptPath.empty()) {
                            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                                " Script: %s", script.scriptPath.c_str());

                            if (script.initialized) {
                                ImGui::SameLine();
                                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "(Initialized)");
                            }
                        }
                        else {
                            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f),
                                "⚠ No script assigned");
                        }

                        // Show Lua state info
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Text("Lua State: %s", script.L ? "Active" : "None");

                        // Show available callback functions with visual indicators
                        if (script.initialized) {
                            ImGui::Spacing();
                            ImGui::Text("Callbacks:");

                            if (script.hasOnInit) {
                                ImGui::SameLine();
                                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "[OnInit]");
                            }
                            if (script.hasOnUpdate) {
                                ImGui::SameLine();
                                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "[OnUpdate]");
                            }
                            if (script.hasOnDestroy) {
                                ImGui::SameLine();
                                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "[OnDestroy]");
                            }

                            // Update timer (useful for debugging)
                            ImGui::Spacing();
                            ImGui::Text("Update Timer: %.3f", script.updateTimer);
                        }

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
                // ADD COMPONENT
                // ==================================================================
                ImGui::Separator();
                if (ImGui::Button("Add Component##AddComponentBtn")) {
                    ImGui::OpenPopup("AddComponentPopup");
                }

                if (ImGui::BeginPopup("AddComponentPopup")) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1.0f), "Add Component");
                    ImGui::Separator();

                    // Transform
                    if (!entityManager->HasComponent<Transform>(entity)) {
                        if (ImGui::MenuItem("Transform")) {
                            entityManager->AddComponent<Transform>(entity, Vector2D(0.0f, 0.0f));
                        }
                    }

                    // Movement
                    if (!entityManager->HasComponent<Movement>(entity)) {
                        if (ImGui::MenuItem("Movement")) {
                            entityManager->AddComponent<Movement>(entity);
                        }
                    }

                    // Sprite
                    if (!entityManager->HasComponent<Sprite>(entity)) {
                        if (ImGui::MenuItem("Sprite")) {
                            auto& sprite = entityManager->AddComponent<Sprite>(entity);
                            sprite.texturePath = "";
                            sprite.layer = 0;
                        }
                    }

                    // MeshRenderer
                    if (!entityManager->HasComponent<MeshRenderer>(entity)) {
                        if (ImGui::MenuItem("MeshRenderer")) {
                            auto& meshRenderer = entityManager->AddComponent<MeshRenderer>(entity);
                            meshRenderer.spriteName = "";
                            meshRenderer.layer = 0;
                        }
                    }

                    // BoxCollider
                    if (!entityManager->HasComponent<BoxCollider>(entity)) {
                        if (ImGui::MenuItem("BoxCollider")) {
                            auto& boxCollider = entityManager->AddComponent<BoxCollider>(entity);
                            boxCollider.size = Vector2D(1.0f, 1.0f);
                            boxCollider.offset = Vector2D(0.0f, 0.0f);
                        }
                    }

                    // CircleCollider
                    if (!entityManager->HasComponent<CircleCollider>(entity)) {
                        if (ImGui::MenuItem("CircleCollider")) {
                            entityManager->AddComponent<CircleCollider>(entity, 0.5f, Vector2D(0.0f, 0.0f));
                        }
                    }

                    // AudioSource
                    if (!entityManager->HasComponent<AudioSource>(entity)) {
                        if (ImGui::MenuItem("AudioSource")) {
                            auto& audio = entityManager->AddComponent<AudioSource>(entity);
                            audio.soundName = "";
                            audio.volume = 1.0f;
                        }
                    }

                    // ScriptComponent
                    if (!entityManager->HasComponent<ScriptComponent>(entity)) {
                        if (ImGui::MenuItem("ScriptComponent")) {
                            auto& script = entityManager->AddComponent<ScriptComponent>(entity);
                            script.scriptPath = "";
                        }
                    }

                    // Health
                    if (!entityManager->HasComponent<Health>(entity)) {
                        if (ImGui::MenuItem("Health")) {
                            entityManager->AddComponent<Health>(entity, 50);
                        }
                    }

                    // SpriteAnimation
                    if (!entityManager->HasComponent<SpriteAnimation>(entity)) {
                        if (ImGui::MenuItem("SpriteAnimation")) {
                            auto& anim = entityManager->AddComponent<SpriteAnimation>(entity);
                            anim.animName = "";
                            anim.frameCount = 1;
                        }
                    }

                    // AP
                    if (!entityManager->HasComponent<AP>(entity)) {
                        if (ImGui::MenuItem("AP")) {
                            entityManager->AddComponent<AP>(entity, 3);
                        }
                    }

                    // AttackRangeComponent
                    if (!entityManager->HasComponent<AttackRangeComponent>(entity)) {
                        if (ImGui::MenuItem("AttackRangeComponent")) {
                            entityManager->AddComponent<AttackRangeComponent>(entity, 1, 3);
                        }
                    }

                    // Chest
                    if (!entityManager->HasComponent<Chest>(entity)) {
                        if (ImGui::MenuItem("Chest")) {
                            entityManager->AddComponent<Chest>(entity, 0);
                        }
                    }

                    // Goal
                    if (!entityManager->HasComponent<Goal>(entity)) {
                        if (ImGui::MenuItem("Goal")) {
                            entityManager->AddComponent<Goal>(entity, 0);
                        }
                    }

                    // Inventory
                    if (!entityManager->HasComponent<Inventory>(entity)) {
                        if (ImGui::MenuItem("Inventory")) {
                            entityManager->AddComponent<Inventory>(entity);
                        }
                    }

                    // AttackAP
                    if (!entityManager->HasComponent<AttackAP>(entity)) {
                        if (ImGui::MenuItem("AttackAP")) {
                            entityManager->AddComponent<AttackAP>(entity, 1);
                        }
                    }

                    ImGui::EndPopup();
                }

                // ============================================================================
                // detail: Inspector actions for prefab-aware entities
                //         - Show originating prefab (if any) using PrefabInstanceTracker
                //         - Allow saving the current entity as a prefab asset
                //         - Support duplicating an entity via temp prefab save+load
                //         - Ensure prefab tracking is cleaned up on delete
                // author: Sim Kah Yan
                // ============================================================================

                // ==================================================================
                // PREFAB TRACKER INFO
                // ------------------------------------------------------------------
                // Query the central PrefabInstanceTracker to see if this entity
                // was originally spawned FROM a prefab file. If so, display a
                // small label in the inspector so designers know which prefab
                // the instance is linked to.
                // ==================================================================
                std::string prefabSource = Framework::PrefabInstanceTracker::Get().GetPrefabOf(entity);
                if (!prefabSource.empty()) {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Prefab: %s",
                        std::filesystem::path(prefabSource).filename().string().c_str());
                }

                // ==================================================================
                // ENTITY ACTIONS
                // author: Sim Kah Yan
                // ==================================================================
                ImGui::Separator();

                // ------------------------------------------------------------------
                // Delete button
                // - Mark the entity to be deleted after the UI pass.
                // - Actual destruction is done later (see below) to avoid
                //   modifying ECS state while iterating the entity list.
                // ------------------------------------------------------------------
                if (ImGui::Button("Delete##DelBtn")) {
                    //Record the deletion here immediately
                    RecordDeletionStep(entity);

                    entityToDelete = entity;
                    shouldDelete = true;
                }

                ImGui::SameLine();

                // ------------------------------------------------------------------
                // Save as prefab button
                // author: Sim Kah Yan
                // ------------------------------------------------------------------
                // - Serializes the current entity and its components to a .prefab
                //   file under "assets/prefabs/".
                // - Filename is auto-generated using the entity ID:
                //       entity_<id>.prefab
                // - Uses PrefabSerializer::SavePrefab, which writes out JSON
                //   based on the components currently attached to this entity.
                // ------------------------------------------------------------------
                if (ImGui::Button("Save Prefab##SavePrefabBtn")) {
                    std::string prefabPath = "assets/prefabs/entity_" + std::to_string(entity.GetID()) + ".prefab";
                    std::filesystem::create_directories("assets/prefabs");

                    bool saved = PrefabSerializer::SavePrefab(*entityManager, entity, prefabPath);
                    if (saved) {
                        std::cout << "[Inspector] Saved entity " << entity.GetID() << " as prefab\n";
                    }
                }

                ImGui::SameLine();

                // ------------------------------------------------------------------
                // Duplicate button
                // author: Sim Kah Yan
                // ------------------------------------------------------------------
                // Duplication strategy:
                //  1. Save the current entity to a temporary prefab file
                //     (e.g. "_temp_duplicate.prefab").
                //  2. Load that prefab again to create a new entity, using the
                //     same code path as normal prefab spawning.
                //  3. Optionally offset the new entity's position slightly so it
                //     doesn't overlap exactly on top of the original.
                //
                // Because duplication uses the same Save/Load flow as prefabs,
                // any new components added to prefab serialization automatically
                // participate in duplication as well.
                // ------------------------------------------------------------------
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

            //Restore the original color
            if (isSelected) {
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }

        ImGui::End();

        // ----------------------------------------------------------------------
        // Handle component removal after UI rendering
        // ----------------------------------------------------------------------
        // Process all component removals that were requested during UI rendering
        for (const auto& removal : componentsToRemove) {
            if (removal.componentType == "Transform") {
                entityManager->RemoveComponent<Transform>(removal.entity);
            }
            else if (removal.componentType == "Movement") {
                entityManager->RemoveComponent<Movement>(removal.entity);
            }
            else if (removal.componentType == "Sprite") {
                entityManager->RemoveComponent<Sprite>(removal.entity);
            }
            else if (removal.componentType == "MeshRenderer") {
                entityManager->RemoveComponent<MeshRenderer>(removal.entity);
            }
            else if (removal.componentType == "BoxCollider") {
                entityManager->RemoveComponent<BoxCollider>(removal.entity);
            }
            else if (removal.componentType == "CircleCollider") {
                entityManager->RemoveComponent<CircleCollider>(removal.entity);
            }
            else if (removal.componentType == "AudioSource") {
                entityManager->RemoveComponent<AudioSource>(removal.entity);
            }
            else if (removal.componentType == "ScriptComponent") {
                entityManager->RemoveComponent<ScriptComponent>(removal.entity);
            }
            else if (removal.componentType == "Health") {
                entityManager->RemoveComponent<Health>(removal.entity);
            }
            else if (removal.componentType == "SpriteAnimation") {
                entityManager->RemoveComponent<SpriteAnimation>(removal.entity);
            }
            else if (removal.componentType == "AP") {
                entityManager->RemoveComponent<AP>(removal.entity);
            }
            else if (removal.componentType == "AttackRangeComponent") {
                entityManager->RemoveComponent<AttackRangeComponent>(removal.entity);
            }
            else if (removal.componentType == "Chest") {
                entityManager->RemoveComponent<Chest>(removal.entity);
            }
            else if (removal.componentType == "Goal") {
                entityManager->RemoveComponent<Goal>(removal.entity);
            }
            else if (removal.componentType == "Inventory") {
                entityManager->RemoveComponent<Inventory>(removal.entity);
            }
            else if (removal.componentType == "AttackAP") {
                entityManager->RemoveComponent<AttackAP>(removal.entity);
            }
        }

        // ----------------------------------------------------------------------
        // Handle deletion after UI rendering
        // author: Sim Kah Yan
        // ----------------------------------------------------------------------
        // Once the frame's UI is done, we safely destroy the entity if it was
        // marked for deletion:
        //  - Remove from ECS
        //  - Remove from spatial partitioning
        //  - Unregister from PrefabInstanceTracker so the tracker does not keep
        //    a dangling entry for a destroyed entity.
        // ----------------------------------------------------------------------
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

    // ============================================================================
    // detail: ImGui-based Prefab Editor window.
    //         - Save any existing entity as a .prefab file
    //         - Browse and load prefabs from assets/prefabs
    //         - Track prefab instances via PrefabInstanceTracker
    //         - Provide a "prefab-wide inspector" that edits ALL instances
    //           of a selected prefab and then writes changes back to disk.
    // Author: Sim Kah Yan
    // ============================================================================
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
        // Author: Sim Kah Yan
        // ------------------------------------------------------------------------
        // This section lets the user:
        //   - Pick any existing entity in the scene
        //   - Type a prefab file name
        //   - Save that entity's components as a new .prefab file
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

        // Text input for the prefab file name to save to.
        static char prefabNameBuffer[256] = "my_entity.prefab";
        ImGui::InputText("Prefab Name##PrefabName", prefabNameBuffer, sizeof(prefabNameBuffer));

        // "Save as Prefab" button.
        //
        // - Ensures a valid entity is selected.
        // - Writes a .prefab file to assets/prefabs/<prefabNameBuffer>.
        // - Uses PrefabSerializer::SavePrefab, which serializes all supported
        //   components on the selected entity to JSON.
        if (ImGui::Button("Save as Prefab##SaveBtn", ImVec2(-1, 0))) {
            if (selectedEntity.IsValid()) {
                std::string path = "assets/prefabs/" + std::string(prefabNameBuffer);

                // Create prefabs directory if it doesn't exist
                std::filesystem::create_directories("assets/prefabs");

                bool saved = PrefabSerializer::SavePrefab(*entityManager, selectedEntity, path);

                if (saved) {
                    std::cout << "[Prefab] Saved entity " << selectedEntity.GetID()
                        << " to: " << path << "\n";
                }
                else {
                    std::cerr << "[Prefab]  Failed to save prefab to: " << path << "\n";
                }
            }
            else {
                std::cout << "[Prefab] No entity selected!\n";
            }
        }

        ImGui::Separator();
        ImGui::Spacing();

        // ========================================================================
        // SECTION 2: Load Prefab
        // Author: Sim Kah Yan
        // ------------------------------------------------------------------------
        // This section:
        //   - Scans assets/prefabs for .prefab / .json files
        //   - Lets the user select a prefab from a list
        //   - Spawns a new entity instance of that prefab at a chosen position
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

        // Index of the currently selected prefab in the list.
        static int selectedPrefabIdx = -1;

        // (Reserved for single-instance editing if needed later.)
        static Framework::Entity selectedInstanceForEdit{};

        // List box that shows all discovered prefabs.
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

        // Spawn position for the next loaded prefab instance.
        static float spawnPos[2] = { 0.0f, 0.0f };
        ImGui::DragFloat2("Spawn Position##SpawnPos", spawnPos, 0.01f, -10.0f, 10.0f);

        ImGui::Separator();

        // "Load Prefab" button.
        //
        // - Requires a prefab to be selected in the list.
        // - Calls PrefabSerializer::LoadPrefab to create a new entity.
        // - If the new entity has a Transform, we write the spawn position
        //   into its Transform component.
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

                    std::cout << "[Prefab]  Loaded prefab '" << prefabFiles[selectedPrefabIdx]
                        << "' as entity " << newEntity.GetID() << "\n";
                }
                else {
                    std::cerr << "[Prefab]  Failed to load prefab: " << selectedPrefabPath << "\n";
                }
            }
            else {
                std::cout << "[Prefab] No prefab selected!\n";
            }
        }

        ImGui::Separator();
        ImGui::Spacing();

        // ========================================================================
        // SECTION 3: Prefab-wide Inspector (applies to ALL instances)
        // Author: Sim Kah Yan
        // ------------------------------------------------------------------------
        // This is the core "live prefab" workflow:
        //   - Get all instances of the selected prefab via PrefabInstanceTracker
        //   - Use the FIRST instance as a "template" in the editor
        //   - User edits the template's components (Transform, Sprite, etc.)
        //   - On "Apply To All Instances & Save Prefab":
        //       * Copy edited values from template to every instance
        //       * Save the template to disk as the updated prefab file
        //
        // This keeps all instances visually consistent and keeps the prefab file
        // in sync with what is shown in the editor.
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

            // The first instance acts as our "template" entity for editing.
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
                        //std::strncpy(texBuf, s.texturePath.c_str(), sizeof(texBuf) - 1);
                        strncpy_s(texBuf, sizeof(texBuf), s.texturePath.c_str(), _TRUNCATE);
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
                        //std::strncpy(nameBuf, mr.spriteName.c_str(), sizeof(nameBuf) - 1);
                        strncpy_s(nameBuf, sizeof(nameBuf), mr.spriteName.c_str(), _TRUNCATE);
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
                // Author: Sim Kah Yan
                // ------------------------------------------------
                // When pressed:
                //   1) Copies edited component values from templateEntity to every
                //      instance of this prefab (except position, which remains
                //      per-instance for Transform).
                //   2) Calls PrefabSerializer::SavePrefab on the template entity
                //      to update the on-disk prefab definition.
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

        // Utility button to wipe all prefab instance tracking state.
        // Does NOT delete entities, only clears the registry mapping.
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
        if (!ImGui::Begin("Entity Spawner/Feature Testing##Spawner1", &showSpawner)) {
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
        if (ImGui::Button("Spawn Blank Entity##Btn1", ImVec2(-1, 0))) {
            if (entityManager) {
                // 1. Create a raw entity
                Framework::Entity blankEntity = entityManager->CreateEntity();

                RecordCreationStep(blankEntity); //record the creation for undo

                // 2. Add a Transform component so it uses the slider coordinates
                // Note: We assume AddComponent adds it. We then retrieve it to set data.
                if (!entityManager->HasComponent<Framework::Transform>(blankEntity)) {
                    entityManager->AddComponent<Framework::Transform>(blankEntity);
                }

                auto& transform = entityManager->GetComponent<Framework::Transform>(blankEntity);
                transform.position = Vector2D(spawnX, spawnY);
                transform.scale = Vector2D(1.0f, 1.0f); // Default scale so it's visible if you add a sprite later

                std::cout << "[Spawner] Spawned Blank Entity ID: " << blankEntity.GetID() << "\n";
            }
        }

        ImGui::Separator();

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
                audioSystem->PlaySound("igbgm", false);
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

        // =====================================================================
        // PREFAB DROP ZONE
        // Author: Sim Kah Yan
        // ---------------------------------------------------------------------
        // This UI block creates a dedicated "Prefab Drop Zone" in the editor:
        //
        //  - Visual:
        //      * Shows a titled region with a large button.
        //      * The button has a custom color so it stands out as a drop target.
        //
        //  - Behavior:
        //      * Accepts ImGui drag-drop payloads of type "Prefab" (see Assets
        //        window where the payload is created with ImGui::SetDragDropPayload).
        //      * The payload carries the full prefab path as a C-string.
        //      * On drop:
        //          1. Calls PrefabSerializer::LoadPrefab to spawn a new entity.
        //          2. If the entity has a Transform, its position is set based on
        //             the spawner's spawnX / spawnY values.
        //          3. Logs the action to the console for debugging.
        // =====================================================================
        ImGui::Separator();
        ImGui::Text("Prefab Drop Zone");
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
                    RecordCreationStep(newEntity); //record the creation for undo

                    // Set spawn position from spawner's X/Y values
                    if (entityManager->HasComponent<Transform>(newEntity)) {
                        auto& transform = entityManager->GetComponent<Transform>(newEntity);
                        transform.position.x = spawnX;
                        transform.position.y = spawnY;
                    }

                    std::cout << "[Spawner]  Spawned prefab as entity " << newEntity.GetID() << "\n";
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
        ImGui::BulletText("Q/ESC: Quit");



        ImGui::End();
    }

    // ============================================================================
    //
    // This is the function that allows picking/selecting game objects in editor mode
    // by using the mouse to click on them. It uses the entity colliders to test
    // whether the mouse is inside an object.
    //
    // author: jiahao.zhou@digipen
    //
    // ============================================================================

    void ImGuiSystem::UpdatePicking() {
        // If the core engine does not exist, stop and do nothing
        if (!CORE) return;
        // If the game is currently playing (not in editor mode), do not handle picking
        if (!CORE->IsEditorMode()) return;
        // If there is no entity manager, we cannot access entities, so stop
        if (!entityManager) return;

        // Get ImGui's input/output state (mouse, keyboard, etc.)
        //ImGuiIO& io = ImGui::GetIO();

        // If we are rendering to the ImGui viewport but the mouse is not hovering that viewport,
        // we should not do picking (prevents clicking outside viewport from selecting things)
        if (IsRenderingToViewport() && !m_isViewportHovered) {
            return;
        }

        // Get pointer to the input system so we can read mouse/keyboard
        InputSystem* input = Framework::CORE->GetInputSystem();
        // Get pointer to the UI system (not directly used here, but kept for future editor features)
        UISystem* ui = CORE->GetUISystem();

        // If input system or UI system is missing, we cannot safely continue
        if (!input || !ui) return;
        // Only run picking logic when the left mouse button is pressed this frame
        if (!input->IsKeyPressed(MOUSE_LEFT)) return;

        // Convert the current mouse position from screen/editor space into world coordinates
        // so we can test it against world-space colliders
        Vector2D mouseWorld = EditorScreenWorld();

        // Start with an invalid entity, meaning nothing is picked yet
        Entity picked = INVALID_ENTITY;

        // Loop through every entity managed by the ECS
        for (Entity e : entityManager->GetAllEntities()) {
            // If this entity has no Transform component, we cannot place it in the world, so skip it
            if (!entityManager->HasComponent<Transform>(e)) continue;
            // Get a reference to the entity's Transform
            auto& transform = entityManager->GetComponent<Transform>(e);
            // If this entity is a grid tile (part of the background tilemap), skip it
            // This prevents selecting the entire tile grid when clicking
            if (entityManager->HasComponent<GridTiles>(e)) continue;
            // Temporary collider object that we will build for this entity
            Collider collider;
            // Flag to remember if this entity actually has a collider we can test
            bool hasCollider = false;

            // If the entity has a CircleCollider component, build a circle collider for picking
            if (entityManager->HasComponent<CircleCollider>(e)) {
                // Get the circle collider data
                auto& cc = entityManager->GetComponent<CircleCollider>(e);
                // Read the entity's scale in X and Y
                float scaleX = transform.scale.x;
                float scaleY = transform.scale.y;
                // Take the larger scale as the circle's scale factor (so circle grows with the biggest axis)
                float scaleFactor = scaleX > scaleY ? scaleX : scaleY;
                // Prevent the scale factor from being too small, so the collider never becomes zero-sized
                if (scaleFactor < 0.01f) scaleFactor = 0.01f;
                // Compute the collider radius in world space (original radius * scale factor)
                float worldRadius = cc.radius * scaleFactor;
                // Create a circle collider in world space, centered at entity position + collider offset
                collider = Collider::create_circle(worldRadius, transform.position + cc.offset);
                // Mark that this entity has a collider we can test
                hasCollider = true;
            }
            // Otherwise, if the entity has a BoxCollider component, build a box collider for picking
            else if (entityManager->HasComponent<BoxCollider>(e)) {
                // Get the box collider data
                auto& bc = entityManager->GetComponent<BoxCollider>(e);
                // Read the entity's scale in X and Y
                float scaleX = transform.scale.x;
                float scaleY = transform.scale.y;
                // Make sure the scales are not too small, to avoid zero-sized boxes
                if (scaleX < 0.01f) scaleX = 0.01f;
                if (scaleY < 0.01f) scaleY = 0.01f;

                // Compute the box width and height in world space (original size * scale)
                float worldWidth = bc.size.x * scaleX;
                float worldHeight = bc.size.y * scaleY;
                // Create a rectangle collider in world space, centered at the entity's position
                collider = Collider::create_rect(worldWidth, worldHeight, transform.position);
                // Mark that this entity has a collider we can test
                hasCollider = true;
            }
            // If this entity had no supported collider, skip it and move on to next entity
            if (!hasCollider) continue;
            // Check if the mouse world position lies inside this entity's collider
            if (point_in_collider(mouseWorld, collider)) {
                // If yes, we consider this entity "picked"
                picked = e;
                break;
            }
        }
        // After checking all entities, store the picked entity as the current selected entity
        selectedEntity = picked;
        // If we picked a valid entity, print its ID for debugging
        if (selectedEntity.GetID() != INVALID_ENTITY) {
            std::cout << "[ImGui Picking] Selected entity ID: " << selectedEntity.GetID() << "\n";
        }
        // Otherwise, the mouse click did not hit any collider, so we clicked on empty space
        else {
            std::cout << "[ImGui Picking] Clicked empty space\n";
        }
    }

    // ============================================================================
    // This is the function that handles entity manipulation (Move, Scale, Rotate) via mouse input
    // author: jiahao.zhou@digipen
    // ============================================================================
    void ImGuiSystem::UpdateEntityDragging() {
        // Safety check: Ensure the Core engine exists
        if (!CORE) return;
        // If the game is currently playing, disable editor dragging to prevent conflicts
        if (!CORE->IsEditorMode()) return;
        // Safety check: Ensure the entity manager exists
        if (!entityManager) return;

        // Get the ImGui I/O interface to check mouse states
        //ImGuiIO& io = ImGui::GetIO();

        // Allow dragging if we are hovering OR already dragging 
        if (!isDraggingEntity && !isScalingEntity && !isRotatingEntity) {
            // If we are rendering to the viewport window AND the mouse is NOT hovering over it,
            // return early so we don't accidentally click things outside the game view
            if (IsRenderingToViewport() && !m_isViewportHovered) {
                return;
            }
        }

        // Get references to the Input and UI systems
        InputSystem* input = Framework::CORE->GetInputSystem();
        UISystem* ui = CORE->GetUISystem();

        // Safety check: If systems are missing, stop here
        if (!input || !ui) return;

        // Convert the mouse's screen coordinates into game world coordinates
        Vector2D mouseWorld = EditorScreenWorld();

        // Check if the user just clicked the Left or Right mouse button this frame
        if (input->IsKeyPressed(MOUSE_LEFT) || input->IsKeyPressed(MOUSE_RIGHT)) {
            // If no entity is selected, or the selected entity doesn't have a Transform component (can't move it)
            if (!selectedEntity.IsValid() ||
                !entityManager->HasComponent<Framework::Transform>(selectedEntity)) {
                // Reset all operation flags
                isDraggingEntity = false;
                isScalingEntity = false;
                isRotatingEntity = false;
                // Clear the dragging target
                draggingEntity = Framework::Entity{ INVALID_ENTITY };
                return;
            }
            // Save the entity's current state to the Undo stack before modify it
            RecordUndoStep(selectedEntity);

            // Get the transform component of the selected entity to read its data
            auto& transform = entityManager->GetComponent<Framework::Transform>(selectedEntity);
            // CHECK FOR SCALING: Left Click + Shift Key
            if (input->IsKeyPressed(MOUSE_LEFT) && input->IsKeyDown(KEY_SHIFT)) {
                isScalingEntity = true;
                isDraggingEntity = false;
                isRotatingEntity = false;
                draggingEntity = selectedEntity;

                // Use new mouseWorld
                scaleStartMouse = mouseWorld;
                scaleStartScale = transform.scale;
            }
            // CHECK FOR ROTATING: Right Click
            else if (input->IsKeyPressed(MOUSE_RIGHT)) {
                isRotatingEntity = true;
                isDraggingEntity = false;
                isScalingEntity = false;
                draggingEntity = selectedEntity;

                // Calculate the initial angle between the object and the mouse
                Vector2D toMouse = transform.position;
                rotateStartAngle = std::atan2(toMouse.y, toMouse.x);
                rotateStartRotation = transform.rotation;
            }
            // CHECK FOR DRAGGING: Left Click (only)
            else if (input->IsKeyPressed(MOUSE_LEFT)) {
                isDraggingEntity = true;
                isScalingEntity = false;
                isRotatingEntity = false;
                draggingEntity = selectedEntity;

                // Use new mouseWorld
                dragOffset = transform.position - mouseWorld;
            }
        }

        // While dragging flag is true and Left Mouse is held down
        if (isDraggingEntity && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            // Verify the entity is still valid
            if (!draggingEntity.IsValid() ||
                !entityManager->HasComponent<Framework::Transform>(draggingEntity))
            {
                isDraggingEntity = false;
                return;
            }
            // Get the transform component
            auto& transform = entityManager->GetComponent<Framework::Transform>(draggingEntity);

            // Update the entity position: Mouse World Position + The initial offset
            transform.position = mouseWorld + dragOffset;
        }

        // While scaling flag is true and Left Mouse is held down
        if (isScalingEntity && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            // Verify the entity is still valid
            if (!draggingEntity.IsValid() ||
                !entityManager->HasComponent<Framework::Transform>(draggingEntity))
            {
                isScalingEntity = false;
                return;
            }

            // Get the transform component
            auto& transform = entityManager->GetComponent<Framework::Transform>(draggingEntity);

            // Calculate distance vector between current mouse pos and start mouse pos
            Vector2D delta = mouseWorld - scaleStartMouse;

            // Calculate a scaling factor based on horizontal mouse movement
            float factor = 1.0f + delta.x * 0.5f;

            // Clamp the factor so the object doesn't disappear (too small) or explode (too big)
            if (factor < 0.1f) factor = 0.1f;
            if (factor > 5.0f) factor = 5.0f;

            // Apply the factor to the original scale
            transform.scale.x = scaleStartScale.x * factor;
            transform.scale.y = scaleStartScale.y * factor;
        }

        // --- EXECUTE ROTATION LOGIC ---
        // While rotating flag is true and Right Mouse is held down
        if (isRotatingEntity && ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            // Verify the entity is still valid
            if (!selectedEntity.IsValid() ||
                !entityManager->HasComponent<Framework::Transform>(selectedEntity)) {
                isRotatingEntity = false;
            }
            else {
                // Get the transform component
                auto& transform = entityManager->GetComponent<Framework::Transform>(selectedEntity);
                // Apply a constant rotation speed to the entity
                const float rotationSpeed = 0.2f;
                transform.rotation += rotationSpeed;
            }
        }

        // --- RESET ON RELEASE (LEFT MOUSE) ---
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            // If we were dragging or scaling, stop now
            if (isDraggingEntity || isScalingEntity) {
                isDraggingEntity = false;
                isScalingEntity = false;
                // Update the spatial partition (grid) because the object moved/resized
                RebuildSpatialPartition();
            }
        }
        // --- RESET ON RELEASE (RIGHT MOUSE) ---
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            isRotatingEntity = false;
        }
    }
    // ============================================================================
    // This function saves the current state of an entity before it gets modified.
    // It pushes the state onto a history stack so we can revert changes later.
    // author: jiahao.zhou@digipen
    // ============================================================================
    void ImGuiSystem::RecordUndoStep(Entity entity) {
        // First, check if the entity is valid and has a Transform to save
        if (!entityManager || !entity.IsValid() || !entityManager->HasComponent<Transform>(entity)) {
            return; // Safety check failed, do nothing
        }

        // Get the current position of the entity (before the user moves it)
        auto& transform = entityManager->GetComponent<Transform>(entity);

        // Create a new undo step with this info
        UndoStep step;
        step.type = UndoType::Transform; // Mark as a Transform change
        step.entity = entity;

        //Save ALL transform data
        step.oldPosition = transform.position;
        step.oldScale = transform.scale;
        step.oldRotation = transform.rotation;

        // Add this step to the end of our history list
        undoStack.push_back(step);

        // Check if have exceeded our memory limit 
        if (undoStack.size() > 20) {
            // Remove the oldest step (from the front) when we exceed the limit
            // If the oldest step was a temp file for deletion, we should delete that file to save space
            if (undoStack.front().type == UndoType::Deletion) {
                std::filesystem::remove(undoStack.front().tempFilePath);
            }
            undoStack.erase(undoStack.begin());
        }

        // Log for debugging purposes
        std::cout << "[Editor] Recorded undo step for Entity " << entity.GetID() << "\n";
    }

    // ============================================================================
    // This function records a "Creation" action in the undo history.
    // It allows the user to undo spawning an object by deleting it later.
    // author: jiahao.zhou@digipen
    // ============================================================================
    void ImGuiSystem::RecordCreationStep(Entity entity) {
        UndoStep step;
        step.type = UndoType::Creation;
        step.entity = entity;
        undoStack.push_back(step);
        std::cout << "[Undo] Recorded Creation step for Entity " << entity.GetID() << "\n";
    }

    // ============================================================================
    // This function records a "Deletion" action in the undo history.
    // It saves the entity's data to a temporary file before it gets destroyed,
    // so we can reload/restore it if the user presses Undo.
    // author: jiahao.zhou@digipen
    // ============================================================================
    void ImGuiSystem::RecordDeletionStep(Entity entity) {
        // Save the entity to a temp file so we can restore it later
        std::string tempPath = "assets/prefabs/_undo_temp_" + std::to_string(entity.GetID()) + ".prefab";
        
        // Ensure directory exists
        std::filesystem::create_directories("assets/prefabs");

        // Use your existing PrefabSerializer
        if (PrefabSerializer::SavePrefab(*entityManager, entity, tempPath)) {
            UndoStep step;
            step.type = UndoType::Deletion;
            step.tempFilePath = tempPath; 
            // Note: We don't store step.entity here because the ID might change or be invalid after delete
            
            undoStack.push_back(step);
            std::cout << "[Undo] Recorded Deletion step. Backup at: " << tempPath << "\n";
        }
    }

    // ============================================================================
    // This function reverts the last recorded action from the undo stack.
    // It handles three types of undo operations:
    // 1. Transform: Restores position, scale, and rotation of an entity.
    // 2. Creation: Destroys an entity that was just created.
    // 3. Deletion: Restores a deleted entity by loading from a backup prefab.
    // author: jiahao.zhou@digipen
    // ============================================================================

    void ImGuiSystem::PerformUndo() {
        if (undoStack.empty()) {
            std::cout << "[Editor] Nothing to undo.\n";
            return;
        }

        UndoStep lastStep = undoStack.back();
        undoStack.pop_back();

        // --------------------------------------------------------------------
        // CASE 1: UNDO TRANSFORM (Pos, Scale, Rot)
        // --------------------------------------------------------------------
        if (lastStep.type == UndoType::Transform) {
            if (entityManager->HasComponent<Transform>(lastStep.entity)) {
                auto& transform = entityManager->GetComponent<Transform>(lastStep.entity);

                // Restore ALL values
                transform.position = lastStep.oldPosition;
                transform.scale = lastStep.oldScale;
                transform.rotation = lastStep.oldRotation;

                std::cout << "[Undo] Restored Transform for Entity " << lastStep.entity.GetID() << "\n";
            }
        }
        // --------------------------------------------------------------------
        // CASE 2: UNDO CREATION (Delete the created object)
        // --------------------------------------------------------------------
        else if (lastStep.type == UndoType::Creation) {
            if (lastStep.entity.IsValid()) {
                std::cout << "[Undo] Destroying created entity " << lastStep.entity.GetID() << "\n";
                entityManager->DestroyEntity(lastStep.entity);
                SpatialPartitioningRemove(lastStep.entity);
            }
        }
        // --------------------------------------------------------------------
        // CASE 3: UNDO DELETION (Restore the deleted object)
        // --------------------------------------------------------------------
        else if (lastStep.type == UndoType::Deletion) {
            // Load from the backup file we made
            Entity newEntity = PrefabSerializer::LoadPrefab(*entityManager, lastStep.tempFilePath);

            if (newEntity.IsValid()) {
                std::cout << "[Undo] Restored deleted entity from " << lastStep.tempFilePath << "\n";
                // Optional: Delete the temp file now that we've used it? 
                // Or keep it in case we Redo (if you implement Redo later).
            }
            else {
                std::cerr << "[Undo] Failed to restore entity from " << lastStep.tempFilePath << "\n";
            }
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
                    std::cerr << "[FileDrop]  Failed to load level\n";
                }
                continue;
            }

            // Handle texture files
            if (IsTextureFile(path)) {
                // ONLY spawn if we are dropping into the Game Viewport
                if (m_isViewportHovered) {
                    if (!entitySpawner) {
                        std::cerr << "[FileDrop]  EntitySpawner not available\n";
                        continue;
                    }

                    // [Add your texture copy logic here if needed, as discussed before]

                    std::string label = path.filename().string();
                    std::string filePath = std::string("assets/") + label;

                    Framework::Entity entity = entitySpawner->SpawnSprite(
                        filePath,
                        Vector2D(0.0f, 0.0f),
                        Vector2D(1.0f, 1.0f)
                    );
                    std::cout << "[FileDrop] Spawned sprite as entity " << entity.GetID() << "\n";
                }
                else {
                    std::cout << "[FileDrop] Ignored texture drop (not in Viewport)\n";
                }
                continue;
            }

            // ====================================================================
            // Handle AUDIO files - AUTOMATIC JSON UPDATE
            // ====================================================================
            std::string errorMsg;
            if (IsAudioFileSupported(path, errorMsg)) {
                // Instead of processing immediately, we setup the popup
                if (m_isViewportHovered || true) { // Allow dropping audio anywhere or restrict to viewport

                    // 1. Store the source path
                    pendingAudioPath = path;

                    // 2. Pre-fill the buffer with the filename (as a default key)
                    std::string defaultName = path.stem().string();
                    //strncpy(newAudioKeyBuffer, defaultName.c_str(), sizeof(newAudioKeyBuffer));
                    strncpy_s(newAudioKeyBuffer, sizeof(newAudioKeyBuffer), defaultName.c_str(), _TRUNCATE);
                    newAudioKeyBuffer[sizeof(newAudioKeyBuffer) - 1] = '\0';

                    // 3. Flag the popup to open next frame
                    showAudioNamePopup = true;

                    std::cout << "[FileDrop] Audio detected. Opening import dialog...\n";
                }
                continue;
            }

            // Unsupported format - show error
            auto ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".ogg" || ext == ".mp3" || ext == ".txt" || ext == ".mp4" ||
                ext == ".flac" || ext == ".aiff" || ext == ".aac" || ext == ".m4a") {

                std::cout << "[FileDrop]  Unsupported format: " << ext << "\n";
                audioErrorMessage = errorMsg;
                showAudioErrorPopup = true;
                continue;
            }

            // =================================================================
            // PREFAB FILE DROP HANDLING
            // Author: Sim Kah Yan
            // -----------------------------------------------------------------
            // When a file is dropped onto the window and its extension is
            // recognized as a prefab type (.prefab or .json):
            //
            //  - We call PrefabSerializer::LoadPrefab with the full path.
            //  - If loading succeeds, a new entity is spawned from the prefab
            //    definition, and we log the entity ID for debugging.
            //  - If loading fails, we print an error message to the console.
            //
            // This allows users to drag prefab files directly from the OS
            // (Explorer/Finder) into the editor to quickly spawn instances.
            // =================================================================
            if (path.extension() == ".prefab" || path.extension() == ".json") {
                Entity newEntity = PrefabSerializer::LoadPrefab(*entityManager, path.string());

                if (newEntity.IsValid()) {
                    std::cout << "[FileDrop]  Loaded prefab: " << path.filename()
                        << " as entity " << newEntity.GetID() << "\n";
                }
                else {
                    std::cerr << "[FileDrop]  Failed to load prefab: " << path << "\n";
                }
                continue;
            }
            
			//LUA SCRIPT DROP HANDLING
            if (path.extension() == ".lua") {
                std::string filename = path.filename().string();
                std::string fullPath = path.string();

                // Convert to lowercase for case-insensitive check
                std::string lowerName = filename;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                // 1. Check if it is a LEVEL script (Contains "level" or "menu")
                bool isLevel = (lowerName.find("level") != std::string::npos) ||
                    (lowerName.find("menu") != std::string::npos);

                if (isLevel) {
                    // --- LOAD NEW LEVEL ---
                    std::cout << "[FileDrop] Detected LEVEL Script: " << filename << "\n";

                    // Clean sweep: Remove current entities
                    if (entityManager) {
                        entityManager->ClearAllEntities();
                        // Optional: Reset ID counter if your engine needs it
                        // entityManager->ResetEntityIDCounter(); 
                    }

                    // Load the level via LevelLoader
                    Framework::LevelLoader::GetInstance().LoadLevel(fullPath, true);
                    // This overrides the Lua script's "DisableImGui()" call
                    this->enabled = true;
                }
                else {
                    // --- SPAWN ENTITY SCRIPT ---
                    // Only allow spawning if we drop it onto the game world (Viewport)
                    if (m_isViewportHovered && entityManager) {
                        std::cout << "[FileDrop] Detected ENTITY Script: " << filename << "\n";

                        // 1. Create a blank entity
                        Entity e = entityManager->CreateEntity();

                        // 2. Add standard components
                        entityManager->AddComponent<Transform>(e, Vector2D(0, 0));

                        // 3. Add a placeholder sprite so you can see it
                        auto& sprite = entityManager->AddComponent<Sprite>(e);
                        sprite.texturePath = "assets/testing.png"; // Or any valid default image

                        // 4. Add Collider so you can select/move it in Editor
                        entityManager->AddComponent<BoxCollider>(e);
                        auto& box = entityManager->GetComponent<BoxCollider>(e);
                        box.size = Vector2D(1.0f, 1.0f);
                        box.isTrigger = true;

                        // 5. Attach the Script Component
                        entityManager->AddComponent<ScriptComponent>(e);
                        auto& script = entityManager->GetComponent<ScriptComponent>(e);
                        script.scriptPath = fullPath;

                        std::cout << "[FileDrop] Spawned Entity " << e.GetID() << " with script: " << filename << "\n";
                    }
                    else {
                        std::cout << "[FileDrop] Entity script ignored (Not dropped in Viewport)\n";
                    }
                }
                continue; // Stop processing this file
            }


            std::cerr << "[FileDrop]  Unsupported file type\n";
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
        const std::string jsonPath = "assets/JSON/AudioConfig.json";

        std::cout << "[JSON] Opening: " << jsonPath << "\n";

        // Read existing JSON
        std::ifstream readFile(jsonPath);

        if (!readFile.is_open()) {
            std::cerr << "[JSON]  Could not open audio.json for reading\n";
            return false;
        }

        std::stringstream buffer;
        buffer << readFile.rdbuf();
        readFile.close();

        std::string jsonContent = buffer.str();

        // Check if audio already exists
        /*if (jsonContent.find("\"" + audioName + "\"") != std::string::npos) {
            std::cout << "[JSON] Audio '" << audioName << "' already exists in JSON\n";
            return true;
        }

        // Find where to insert
        size_t lastBrace = jsonContent.rfind('}');
        if (lastBrace == std::string::npos) {
            std::cerr << "[JSON]  Malformed JSON - no closing brace\n";
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
                std::cerr << "[JSON]  Malformed JSON - no opening brace\n";
                return false;
            }
        }

        // Insert new entry
        jsonContent.insert(insertPos + 1, newEntry);

        std::cout << "[JSON] Adding entry: \"" << audioName << "\": \"" << fileName << "\"\n";

        // Write updated JSON
        std::ofstream writeFile(jsonPath);

        if (!writeFile.is_open()) {
            std::cerr << "[JSON]  Could not open audio.json for writing\n";
            return false;
        }*/
        // ---------------------------------------------------------
    // STEP 1: Find the "sounds" array
    // ---------------------------------------------------------
    size_t soundsPos = jsonContent.find("\"sounds\"");
    if (soundsPos == std::string::npos) {
        std::cerr << "[JSON]  Could not find 'sounds' array\n";
        return false;
    }

    // Find the start of the array '['
    size_t arrayStart = jsonContent.find("[", soundsPos);
    if (arrayStart == std::string::npos) return false;

    // ---------------------------------------------------------
    // STEP 2: Find the END of the "sounds" array ']'
    // ---------------------------------------------------------
    // We can't just look for the first ']', because nested objects use them too.
    // We scan forward counting brackets.
    size_t arrayEnd = std::string::npos;
    int bracketCount = 0;
    
    for (size_t i = arrayStart; i < jsonContent.length(); ++i) {
        if (jsonContent[i] == '[') bracketCount++;
        else if (jsonContent[i] == ']') {
            bracketCount--;
            if (bracketCount == 0) {
                arrayEnd = i; // Found the closing bracket of "sounds"
                break;
            }
        }
    }

    if (arrayEnd == std::string::npos) {
        std::cerr << "[JSON]  Malformed JSON (missing closing bracket)\n";
        return false;
    }

    // ---------------------------------------------------------
    // STEP 3: Construct the new JSON Object
    // ---------------------------------------------------------
    // Note: We add a comma at the start because we assume the list isn't empty.
    std::string newEntry = R"(,
    {
      "name": ")" + audioName + R"(",
      "filepath": "assets/)" + fileName + R"(",
      "volume": 1,
      "preload": true
    })";

    // ---------------------------------------------------------
    // STEP 4: Insert it BEFORE the closing bracket ']'
    // ---------------------------------------------------------
    jsonContent.insert(arrayEnd, newEntry);

    // ---------------------------------------------------------
    // STEP 5: Save File
    // ---------------------------------------------------------
    std::ofstream writeFile(jsonPath);
    if (!writeFile.is_open()) {
        std::cerr << "[JSON]  Could not open file for writing\n";
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

    // ============================================================================
    // SetupDockSpace - Creates fullscreen dockspace with menu bar
    // ============================================================================
    // ============================================================================
    // SetupDockSpace - Creates dockspace with menu bar (with version checking)
    // ============================================================================
    // ============================================================================
    // SetupDockSpace - Simple menu bar (NO DOCKING - for older ImGui)
    // ============================================================================
    // ============================================================================
    // SetupDockSpace - Full Docking Support (ImGui 1.89+ docking branch)
    // ============================================================================
    // ============================================================================
    // SetupDockSpace - Simple Docking (No DockBuilder API)
    // Uses basic DockSpace without automatic layout
    // ============================================================================
    void ImGuiSystem::SetupDockSpace()
    {
        // Create fullscreen dockspace window
        static bool opt_fullscreen = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Window", nullptr, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Create DockSpace - THIS WORKS even without DockBuilder
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

            // NOTE: No DockBuilder - user will arrange windows manually
            // First time windows will be floating
            // User can drag them to dock
        }

        // ========================================================================
        // MENU BAR
        // ========================================================================
        if (ImGui::BeginMenuBar()) {
            //File bar
            if (ImGui::BeginMenu("File")) {

                
                // Open Level
                if (ImGui::MenuItem("Open Level...", "Ctrl+O")) {
                    showLevelBrowser = true;
                    selectedLevelPath = "";
                }

                ImGui::Separator();

                // ================================================================
                // JSON SAVE/LOAD - Universal format for all level types
                // ================================================================
                {
                    extern int current;
                    std::string levelName = "Unknown";
                    switch (current) {
                        case mainMenu: levelName = "MainMenu"; break;
                        case Level_select: levelName = "LevelSelect"; break;
                        case LEVEL_2: levelName = "Level2"; break;
                        case LEVEL_3: levelName = "Level3"; break;
                        case TUTORIAL: levelName = "Tutorial"; break;
                        case LEVEL_END: levelName = "LevelEnd"; break;
                    }

                    // Save Scene (JSON)
                    if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                        std::string savePath = "assets/saves/" + levelName + "_save.json";
                        SaveLoadSystem::SetGraphicsSystem(graphicsSystem);
                        if (SaveLoadSystem::SaveToJSON(savePath, entityManager, levelName)) {
                            LOG_INFO("ImGuiSystem", "Saved scene to: %s", savePath.c_str());
                        }
                    }

                    if (ImGui::MenuItem("Save Scene As..."))
                    {
                        wantSaveSceneAsModal = true;
                    }

                    // Load Scene (JSON)
                    if (ImGui::MenuItem("Load Scene")) {
                        std::string loadPath = "assets/saves/" + levelName + "_save.json";
                        SaveLoadSystem::SetGraphicsSystem(graphicsSystem);
                        if (SaveLoadSystem::LoadFromJSON(loadPath, entityManager, true)) {
                            LOG_INFO("ImGuiSystem", "Loaded scene from: %s", loadPath.c_str());
                            RebuildSpatialPartition();
                        }
                    }

                    if (ImGui::MenuItem("Load Scene..."))
                    {
                        wantOpenSceneModal = true;
                    }
                }

                ImGui::Separator();

                // 显示当前关卡
                extern int current;
                const char* currentLevelName = "Unknown";

                switch (current) {
                case mainMenu: currentLevelName = "Main Menu"; break;
                case Level_select: currentLevelName = "Level Select"; break;
                case LEVEL_2: currentLevelName = "Level 2"; break;
                case LEVEL_3: currentLevelName = "Level 3"; break;
                case TUTORIAL: currentLevelName = "Tutorial"; break;
                case LEVEL_END: currentLevelName = "Level End"; break;
                }

                ImGui::TextDisabled("Current: %s", currentLevelName);

                ImGui::Separator();

                if (ImGui::MenuItem("Exit")) {
                    Message quitMsg(Status::Quit);
                    CORE->BroadcastMessage(&quitMsg);
                }

                ImGui::EndMenu();
            }

            //windows bar
            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Entity Inspector", nullptr, &showEntityInspector);
                ImGui::MenuItem("Spawner", nullptr, &showSpawner);
                ImGui::MenuItem("Debug Info", nullptr, &showDebug);
                ImGui::MenuItem("ImGui Demo", nullptr, &showDemo);
                ImGui::MenuItem("Assets", nullptr, &showAssets);
                ImGui::MenuItem("Prefabs", nullptr, &showPrefabWindow);
                ImGui::MenuItem("Game Viewport", nullptr, &showGameViewport);
                ImGui::Separator();
                ImGui::MenuItem("Render to Viewport", nullptr, &renderToViewport);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Editor")) {
                if (!CORE->IsPlaying()) {
                    //  In editor mode: Show "PLAY" to exit editor
                    if (ImGui::MenuItem("PLAY")) {

                        CORE->SetPlaying(true);
                        // Exit editor mode
                        CORE->SetEditorMode(false);

						GlobalPause::SetPaused(false);
                        //  CRITICAL: Use RequestToggle() instead of Disable()
                        // This schedules the disable for AFTER this frame completes
                        
                        /*if (!currentLuaLevelPath.empty()) {

                            // If we can map it to a GSM state, transition GSM so Level3 init runs correctly
                            if (pendingLuaGsmState != -1)
                            {
                                // If already in that state, just reload as GAME-load (IS_EDITOR_LOAD = false)
                                if (pendingLuaGsmState == current)
                                {
                                    Framework::LevelLoader::GetInstance().LoadLevel(currentLuaLevelPath, false);
                                }
                                else
                                {
                                    next = pendingLuaGsmState;
                                }
                            }
                            else {
                                Framework::LevelLoader::GetInstance().LoadLevel(currentLuaLevelPath, false);
                            }
                            currentLuaLevelPath.clear();
							pendingLuaGsmState = -1;
                        }

                        else if (!currentLevelPath.empty()) {
                            Framework::LevelLoader::GetInstance().LoadLevel(currentLevelPath, false);
                        }*/

                        //this->RequestToggle();

                        LOG_INFO("IMGUI", "PLAY clicked - Exiting editor mode");
                    }
                }
                else {
                    if (ImGui::MenuItem("Stop"))
                    {
                        // Stop simulation, return to editing
                        CORE->SetPlaying(false);
                        CORE->SetEditorMode(true);
                        GlobalPause::SetPaused(false);
                        // Not in editor mode: Show hint
                        //ImGui::TextDisabled("Press F1 to enter editor mode"); // Re-enable the hint display
                    }

                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();

        }

        ImGui::End();  // End DockSpace Window
    }


    void ImGuiSystem::ShowGameViewport()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        // No size constraints - let docking system control size
        if (ImGui::Begin("Game##GameViewport", &showGameViewport)) {

            // --- NEW: Track Focus/Hover State ---
            m_isViewportFocused = ImGui::IsWindowFocused();
            m_isViewportHovered = ImGui::IsWindowHovered();

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

                // --- NEW: Capture Position/Size AFTER drawing the image ---
                m_viewportPos = ImGui::GetItemRectMin();  // Screen coordinates of top-left
                m_viewportSize = ImGui::GetItemRectSize(); // Size in pixels
            }
            else {
                ImGui::Text("Viewport too small: %.0fx%.0f", size.x, size.y);
            }
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }

    std::vector<std::string> ImGuiSystem::GetLuaFilesInDirectory(const std::string& directory) {
        std::vector<std::string> luaFiles;

        try {
            if (!std::filesystem::exists(directory)) {
                std::cerr << "[ImGuiSystem] Scripts directory not found: " << directory << std::endl;
                return luaFiles;
            }

            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                    std::filesystem::path relativePath = std::filesystem::relative(entry.path(), directory);

                    std::string pathStr = relativePath.string();
                    std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

                    luaFiles.push_back(pathStr);
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[ImGuiSystem] Error reading scripts directory: " << e.what() << std::endl;
        }

        std::sort(luaFiles.begin(), luaFiles.end());

        return luaFiles;
    }

    void ImGuiSystem::ShowScriptBrowserPopup() {
        if (showScriptBrowser) {
            ImGui::OpenPopup("Script Browser");
            showScriptBrowser = false;  
        }

        ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);

        if (ImGui::BeginPopupModal("Script Browser", nullptr)) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Select a Lua Script");
            ImGui::Separator();

            static char searchBuffer[256] = "";
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##search", "Search scripts...", searchBuffer, 256)) {
            }

            ImGui::Separator();

            ImGui::BeginChild("ScriptList", ImVec2(0, -35), true);

            auto scriptFiles = GetLuaFilesInDirectory("assets/scripts/");

            std::string searchStr = searchBuffer;
            std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

            int selectableCount = 0;
            for (const auto& scriptFile : scriptFiles) {
                std::string lowerScriptFile = scriptFile;
                std::transform(lowerScriptFile.begin(), lowerScriptFile.end(),
                    lowerScriptFile.begin(), ::tolower);

                if (!searchStr.empty() && lowerScriptFile.find(searchStr) == std::string::npos) {
                    continue;
                }

                selectableCount++;

                if (ImGui::Selectable(scriptFile.c_str())) {
                    selectedScriptPath = scriptFile;

                    if (entityPendingScriptAssignment.IsValid() &&
                        entityManager) {

                        if (entityManager->HasComponent<ScriptComponent>(entityPendingScriptAssignment)) {
                            auto& sc = entityManager->GetComponent<ScriptComponent>(entityPendingScriptAssignment);
                            sc.scriptPath = "assets/scripts/" + selectedScriptPath;
                        }
                        else {
                            auto& sc = entityManager->AddComponent<ScriptComponent>(entityPendingScriptAssignment);
                            sc.scriptPath = "assets/scripts/" + selectedScriptPath;
                        }

                        std::cout << "[ImGuiSystem] Assigned script: scripts/" << selectedScriptPath
                            << " to entity " << entityPendingScriptAssignment.GetID() << std::endl;
                    }

                    ImGui::CloseCurrentPopup();
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Full path: scripts/%s", scriptFile.c_str());
                    ImGui::EndTooltip();
                }
            }

            if (selectableCount == 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                    "No scripts found matching '%s'", searchBuffer);
            }

            ImGui::EndChild();

            
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                searchBuffer[0] = '\0';
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                "Found %d script(s)", selectableCount);

            ImGui::EndPopup();
        }
    }

    std::vector<std::string> ImGuiSystem::GetLuaLevelsInDirectory(const std::string& directory) {
        std::vector<std::string> levelFiles;

        try {
            if (!std::filesystem::exists(directory)) {
                LOG_ERROR("ImGuiSystem", "Directory not found: %s", directory.c_str());
                return levelFiles;
            }

            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                    std::string filename = entry.path().filename().string();

                    // 只包含关卡文件
                    if (filename.find("Level") != std::string::npos) {
                        levelFiles.push_back(filename);
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            LOG_ERROR("ImGuiSystem", "Error reading directory: %s", e.what());
        }

        std::sort(levelFiles.begin(), levelFiles.end());
        return levelFiles;
    }

    int ImGuiSystem::GetGameStateFromLevelName(const std::string& levelName) {
        // MainMenu
        if (levelName.find("MainMenu") != std::string::npos) {
            return mainMenu;
        }

        // LevelSelect
        if (levelName.find("LevelSelect") != std::string::npos) {
            return Level_select;
        }

        // Level1, Level2, Level3...
        std::regex levelRegex(R"(Level(\d+)\.lua)");
        std::smatch match;

        if (std::regex_match(levelName, match, levelRegex)) {
            int levelNum = std::stoi(match[1].str());

            switch (levelNum) {
            case 2: return LEVEL_2;
            case 3: return LEVEL_2;
            default:
                LOG_WARN("ImGuiSystem", "Unknown level number: %d", levelNum);
                return -1;
            }
        }

        LOG_ERROR("ImGuiSystem", "Failed to parse level name: %s", levelName.c_str());
        return -1;
    }

    bool ImGuiSystem::LoadLevelViaGSM(const std::string& levelName) {
        LOG_INFO("ImGuiSystem", "Loading level via GSM (Editor Mode): %s", levelName.c_str());

        int gameState = GetGameStateFromLevelName(levelName);

        if (gameState == -1) {
            LOG_ERROR("ImGuiSystem", "Invalid level name: %s", levelName.c_str());
            return false;
        }

        extern int next;
        extern bool g_loadAsEditorMode;
        
        next = gameState;
        g_loadAsEditorMode = true;  // Tell GSM to load in editor mode

        currentEditingLevel = levelName;

        // Enable ImGui immediately since we're loading in editor mode
        enabled = true;

        LOG_INFO("ImGuiSystem", " Switching to game state %d (editor mode)", gameState);

        return true;
    }

    bool ImGuiSystem::SaveCurrentLevel() {
        extern int current;

        std::string levelName;

        switch (current) {
        case LEVEL_2: levelName = "Level2"; break;
        case LEVEL_3: levelName = "Level3"; break;
        case mainMenu: levelName = "MainMenu"; break;
        case Level_select: levelName = "LevelSelect"; break;
        default:
            LOG_WARN("ImGuiSystem", "Cannot save: unknown game state");
            return false;
        }

        LOG_INFO("ImGuiSystem", "Saving level: %s", levelName.c_str());

        std::string savePath = "assets/" + levelName + "_saved.txt";
        bool success = SaveLevelToTxt(savePath);

        if (success) {
            LOG_INFO("ImGuiSystem", " Saved to: %s", savePath.c_str());
            return true;
        }
        else {
            LOG_ERROR("ImGuiSystem", " Failed to save: %s", savePath.c_str());
            return false;
        }
    }

    void ImGuiSystem::ShowLevelBrowserPopup() {
        if (showLevelBrowser) {
            ImGui::OpenPopup("Open Level");
            showLevelBrowser = false;
        }

        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

        if (ImGui::BeginPopupModal("Open Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Select a level to load:");
            ImGui::Separator();
            ImGui::Spacing();

            static char searchBuffer[256] = "";
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##search", "Search...", searchBuffer, 256);
            ImGui::Spacing();

            ImGui::BeginChild("LevelList", ImVec2(0, 250), true);

            auto levelFiles = GetLuaLevelsInDirectory("assets/scripts/");

            std::string searchStr = searchBuffer;
            std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

            bool foundAny = false;

            for (const auto& levelFile : levelFiles) {
                std::string lowerLevelFile = levelFile;
                std::transform(lowerLevelFile.begin(), lowerLevelFile.end(),
                    lowerLevelFile.begin(), ::tolower);

                if (!searchStr.empty() && lowerLevelFile.find(searchStr) == std::string::npos) {
                    continue;
                }

                foundAny = true;
                ImGui::PushID(levelFile.c_str());

                ImGui::Text("📄");
                ImGui::SameLine();

                if (ImGui::Selectable(levelFile.c_str(), selectedLevelPath == levelFile,
                    ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        if (LoadLevelViaGSM(levelFile)) {
                            LOG_INFO("ImGuiSystem", "Loading: %s", levelFile.c_str());
                        }
                        ImGui::PopID();
                        ImGui::CloseCurrentPopup();
                        break;
                    }
                    else {
                        selectedLevelPath = levelFile;
                    }
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Double-click to load");
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Path: assets/scripts/%s", levelFile.c_str());
                    ImGui::EndTooltip();
                }

                ImGui::PopID();
            }

            if (!foundAny) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No levels found");
            }

            ImGui::EndChild();

            ImGui::Separator();
            ImGui::Spacing();

            if (!selectedLevelPath.empty()) {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Selected: %s", selectedLevelPath.c_str());
            }

            ImGui::Spacing();

            if (!selectedLevelPath.empty()) {
                if (ImGui::Button("Load", ImVec2(120, 0))) {
                    if (LoadLevelViaGSM(selectedLevelPath)) {
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            else {
                ImGui::BeginDisabled();
                ImGui::Button("Load", ImVec2(120, 0));
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                searchBuffer[0] = '\0';
                selectedLevelPath = "";
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
} // namespace Framework