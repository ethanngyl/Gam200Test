#pragma once
// Standard C++ libraries
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <exception>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <cstdint>
#include <unordered_map>
#include <typeindex>
#include <cstdarg>
#include <mutex>
#include <string_view>
#include <functional>
//Components

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
#include "GraphicsSystem.h"
#include "Mesh.h"
#include "MeshFactory.h"
#include "Shader.h"

//Collision
#include "CollisionSystem.h"
#include "Collision.h"

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
#include "PerfViewer.h"
#include "Sinks.h"
#include "Trace.h"

//ECS
#include "ECSComponent.h"
#include "ECSEntity.h"
#include "ECSEntityManager.h"
#include "Component.h"

//Message
#include "Message.h"

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

// Safe deletion macros
#define SAFE_DELETE(p) { if(p) { delete (p); (p) = nullptr; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p) = nullptr; } }