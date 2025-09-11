#ifndef PLATFORM_H
#define PLATFORM_H

// Platform detection
#ifdef _WIN32
#define PLATFORM_WINDOWS
#endif

// Platform-specific includes
#ifdef PLATFORM_WINDOWS
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstring>

// Windows logging macros
#include <cstdio>
#define LOGI(...) printf("[INFO] " __VA_ARGS__); printf("\n")
#define LOGE(...) printf("[ERROR] " __VA_ARGS__); printf("\n")
#define LOGD(...) printf("[DEBUG] " __VA_ARGS__); printf("\n")
#endif

#endif // PLATFORM_H 