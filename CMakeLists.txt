# ======================= Project Configuration =========================
cmake_minimum_required(VERSION 3.5...3.13)
project(StructSquad)

# Set C++ standard early
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the directory where custom CMake modules are stored
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Include project-wide utility and dependency modules
include(ImportDependencies)
importDependencies()

# Global compile flags and macros
add_definitions(-DGLEW_STATIC)
add_definitions(-DUSE_CSD3151_AUTOMATION=0) # Used for instructor's automation

# ======================= Source Configuration =========================

set(SRC_DIR ./engine)
file(GLOB_RECURSE APP_SRC 
   "${SRC_DIR}/*.cpp"
   "${SRC_DIR}/*.h"
   "${SRC_DIR}/*.hpp"
)

# ======================= Target Creation =========================

# Create executable
add_executable(${CMAKE_PROJECT_NAME} ${APP_SRC} 
"shaders/basic.vert"
"shaders/basic.frag"
"engine/main.cpp")

# ======================= Linking =========================

# Link libraries (fixed syntax)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    glfw
    glm
    libglew_static
    imgui
)

# Mark shaders as non-compiled
set_source_files_properties(
    "shaders/basic.vert"
    "shaders/basic.frag"
    PROPERTIES HEADER_FILE_ONLY TRUE
)

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/engine/shaders
     DESTINATION ${CMAKE_BINARY_DIR})
# ======================= Include Directories =========================

# Include directories for the project (fixed to use directory, not files)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${SRC_DIR}
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/Core
    ${CMAKE_SOURCE_DIR}/engine/ECS
    ${CMAKE_SOURCE_DIR}/engine/Collision
    ${CMAKE_SOURCE_DIR}/engine/Movement
    ${CMAKE_SOURCE_DIR}/engine/Debugger
    ${CMAKE_SOURCE_DIR}/engine/Graphics  
    ${CMAKE_SOURCE_DIR}/engine/Input
    ${CMAKE_SOURCE_DIR}/engine/Math
    ${CMAKE_SOURCE_DIR}/engine/Systems
    ${CMAKE_SOURCE_DIR}/engine/Windows
    ${CMAKE_SOURCE_DIR}/engine/Components
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ======================= Compiler Settings =========================

# Compiler-specific warnings (fixed undefined variable)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra)
elseif (MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4)
endif()

# ======================= Platform-Specific Linking =========================
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE  # This makes it a Windows GUI app
)
# Add platform-specific OpenGL libraries
if(WIN32)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE opengl32)
elseif(APPLE)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "-framework OpenGL")
elseif(UNIX)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GL X11 pthread dl)
endif()

# ======================= Runtime Data Copy =========================

# Copy shaders folder
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/engine/shaders" "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/shaders")

# Copy assets folder
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/assets")
