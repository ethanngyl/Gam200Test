#pragma once

// Standard C++ libraries
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <cassert>
#include <cmath>

// Windows-specific headers (for timing and input)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>    // For timeGetTime()
#include <conio.h>       // For _kbhit() and _getch()
#pragma comment(lib, "winmm.lib")  // Link timing library
#endif

// OpenGL headers (since you have GLRenderer)
//#ifdef USE_OPENGL
//#include <GL/gl.h>
//#include <GL/glu.h>
//#include <glad/glad.h>
//#endif

// Useful debug macros
#ifdef _DEBUG
#define ASSERT(condition) assert(condition)
#else
#define ASSERT(condition) ((void)0)
#endif

// Safe deletion macros
#define SAFE_DELETE(p) { if(p) { delete (p); (p) = nullptr; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p) = nullptr; } }