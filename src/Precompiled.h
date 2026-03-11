/**
===============================================================================
 File:           Precompiled.h
 Author:         ETHAN NG YONG LE
 Email:          n.ethanyongle@digipen.edu
 Date:           2025-09-30
 Contribution:   100%
 ------------------------------------------------------------------------------

  Design notes:
  Central include file that precompiles commonly-used headers to improve
 * compilation times. All .cpp files in the project should include this
 * as their first include.
===============================================================================
 */
#pragma once

// Standard C++ libraries
#include <vector> //Dynamic Array
#include <string> //String Handling
#include <iostream> //Console I/O
#include <memory> //Smart pointer
#include <cassert> //Assertions
#include <cmath> //Math functions
#include <fstream> //File I/O
#include <sstream> //String streams
#include <cstdio> //C-style I/O
#include <exception> //Exception handling
#include <iomanip> //I/O formatting
#include <chrono> //Timing utilities 
#include <algorithm> //STL Algorithms
#include <array> //Fixed-size array
#include <cstddef> //Standard definitions
#include <map> //Sorted associative containers
#include <cstdint> //Fixed-width integer types
#include <unordered_map> //Hash maps
#include <typeindex> //Type identification
#include <cstdarg> //Variable Arguements
#include <mutex> //Thread synchronization
#include <string_view> //Non-owning string references
#include <functional> //Function objects
#include <queue>
#include <stdexcept>

//Components

//New Graphics 
#include "ResourceHandle.h"
#include "Material.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "RenderCommand.h"
#include "RenderComponents.h"
#include "GraphicsSystemV2.h"

//Particles
#include "Graphics/ParticleSystem.h"
#include "Graphics/ParticleSystemManager.h"

//Animations
#include "AnimationSystem.h"

//Core
#include "Core.h"

//Window
#include "WindowSystem.h"
#include "Windows.h"

//Input
#include "Input.h"

//Interface
#include "Interface.h" // The base class for all your engine systems

//Graphics
#include "Mesh.h"
#include "Shader.h"

//Collision
#include "CollisionSystem.h"

//Math
#include "MathTestSystem.h"
#include "Vector2D.h"
#include "Matrix3x3.h"

//Movement
#include "MovementSystem.h"

//Debugger
#include "CrashLogger.h"
#include "Log.h"
#include "Perf.h"
#include "Trace.h"
#include "PerfViewer.h"
#include "Sinks.h"
#include "DebugConfig.h"

//GSM
#include "GSM\GameStateList.h"
#include "GSM\GameStateManager.h"

//UI
#include "UI\UISystem.h"

//ConfigReader
#include "ConfigReader\ConfigReader.h"

//Memory Manager (custom pool allocator for game objects)
#include "Memory/MemoryManager.h"

//ECS
#include "ECSComponent.h"
#include "ECSEntity.h"
#include "ECSEntityManager.h"
#include "Component.h"

//Message
#include "Message.h"

//Event
#include "Event\Event.h"

//Grid
#include "Grid/Grid.h"
#include "Grid/GridTile.h"
//#include "Grid/GridECS.h"

//Skills
#include "Skills/SkillComponent.h"
#include "Skills/SkillSystem.h"
#include "Skills/PopUp.h"

//#include "fsm/Istate.h"
//#include "fsm/StateMachine.h"
//#include "fsm/FSMComponent.h"
//#include "fsm/FSMSystem.h"

// Windows-specific headers (for timing and input)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>    // For timeGetTime()
#include <conio.h>       // For _kbhit() and _getch()
#pragma comment(lib, "winmm.lib")  // Link timing library
#endif

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

//GLEW
#include <GL/glew.h>
#include "GL/gl.h"

//GLFW
#include <GLFW/glfw3.h>
#include <stb_image.h>

// Useful debug macros
#ifdef _DEBUG
#define ASSERT(condition) assert(condition)
#else
#define ASSERT(condition) ((void)0)
#endif

#include <Text/TextSystem.h>

// Safe deletion macros
#define SAFE_DELETE(p) { if(p) { delete (p); (p) = nullptr; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p) = nullptr; } }