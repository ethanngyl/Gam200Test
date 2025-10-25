# ImportDependencies.cmake
# This file handles all external dependencies for the StructSquad project

include(FetchContent)

# Macro to import GLFW
macro(import_glfw)
    if(NOT TARGET glfw)
        message(STATUS "Importing GLFW...")
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
        )
        
        # Configure GLFW build options
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
        
        FetchContent_MakeAvailable(glfw)
        message(STATUS "GLFW imported successfully")
    endif()
endmacro()

# Macro to import GLM
macro(import_glm)
    if(NOT TARGET glm)
        message(STATUS "Importing GLM...")
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 0.9.9.8
        )
        FetchContent_MakeAvailable(glm)
        target_include_directories(glm SYSTEM INTERFACE 
            ${glm_SOURCE_DIR})
        message(STATUS "GLM imported successfully")
    endif()
endmacro()

# Macro to import GLEW
macro(import_glew)
    if(NOT TARGET libglew_static)
        message(STATUS "Importing GLEW...")
        FetchContent_Declare(
            glew
            URL https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.zip
            URL_HASH SHA256=a9046a913774395a095edcc0b0ac2d81c3aacca61787b39839b941e9be14e0d4
        )
        FetchContent_Populate(glew)
        
        # GLEW source files
        set(GLEW_SOURCES ${glew_SOURCE_DIR}/src/glew.c)
        
        # Create GLEW static library
        add_library(libglew_static STATIC ${GLEW_SOURCES})
        target_include_directories(libglew_static PUBLIC ${glew_SOURCE_DIR}/include)
        target_compile_definitions(libglew_static PUBLIC GLEW_STATIC)
        
        # Create GLEW shared library
        add_library(libglew_shared SHARED ${GLEW_SOURCES})
        target_include_directories(libglew_shared PUBLIC ${glew_SOURCE_DIR}/include)
        
        # Platform-specific linking for GLEW
        if(WIN32)
            target_link_libraries(libglew_static PUBLIC opengl32)
            target_link_libraries(libglew_shared PUBLIC opengl32)
        elseif(APPLE)
            target_link_libraries(libglew_static PUBLIC "-framework OpenGL")
            target_link_libraries(libglew_shared PUBLIC "-framework OpenGL")
        elseif(UNIX)
            target_link_libraries(libglew_static PUBLIC GL)
            target_link_libraries(libglew_shared PUBLIC GL)
        endif()
        
        message(STATUS "GLEW imported successfully")
    endif()
endmacro()

# Macro to import ImGui
macro(import_imgui)
    if(NOT TARGET imgui)
        message(STATUS "Importing ImGui...")
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.89.9
        )
        FetchContent_Populate(imgui)
        
        # ImGui source files
        set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
        
        # Create ImGui library
        add_library(imgui STATIC ${IMGUI_SOURCES})
        target_include_directories(imgui PUBLIC 
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        
        # ImGui needs GLFW and OpenGL
        target_link_libraries(imgui PUBLIC glfw libglew_static)
        target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
        
        message(STATUS "ImGui imported successfully")
    endif()
endmacro()

macro(import_stb)
    if(NOT TARGET stb)
        message(STATUS "Importing stb_image...")
        FetchContent_Declare(
            stb
            GIT_REPOSITORY https://github.com/nothings/stb.git
            GIT_TAG master
        )
        FetchContent_Populate(stb)
        
        # stb is header-only, just create an interface library
        add_library(stb INTERFACE)
        target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
        
        message(STATUS "stb_image imported successfully")
    endif()
endmacro()

# Macro to import FMOD (Windows only)
macro(import_fmod)
    if(NOT TARGET fmod)
        message(STATUS "=== Importing FMOD ===")
        
        # Search for FMOD in library folder
        set(FMOD_SEARCH_PATHS
            "${CMAKE_SOURCE_DIR}/library/FMOD Studio API Windows"
            "${CMAKE_SOURCE_DIR}/library/fmodstudioapi20309windows"
        )
        
        # Allow user override
        set(FMOD_ROOT "" CACHE PATH "Path to FMOD installation directory")
        
        # If user didn't specify, search for it
        if(NOT FMOD_ROOT OR NOT EXISTS "${FMOD_ROOT}")
            message(STATUS "FMOD_ROOT not set, searching in library folder...")
            foreach(SEARCH_PATH ${FMOD_SEARCH_PATHS})
                if(EXISTS "${SEARCH_PATH}")
                    set(FMOD_ROOT "${SEARCH_PATH}")
                    message(STATUS "Found FMOD at: ${FMOD_ROOT}")
                    break()
                endif()
            endforeach()
        endif()
        
        if(NOT FMOD_ROOT OR NOT EXISTS "${FMOD_ROOT}")
            message(WARNING "FMOD not found. Searched in: ${CMAKE_SOURCE_DIR}/library/")
            add_library(fmod INTERFACE)
            set(FMOD_FOUND FALSE CACHE BOOL "FMOD library found" FORCE)
            return()
        endif()
        
        # Windows paths
        set(FMOD_CORE_LIB_DIR "${FMOD_ROOT}/api/core/lib/x64")
        set(FMOD_STUDIO_LIB_DIR "${FMOD_ROOT}/api/studio/lib/x64")
        set(FMOD_CORE_INCLUDE_DIR "${FMOD_ROOT}/api/core/inc")
        set(FMOD_STUDIO_INCLUDE_DIR "${FMOD_ROOT}/api/studio/inc")
        
        message(STATUS "Library directory: ${FMOD_CORE_LIB_DIR}")
        message(STATUS "Include directory: ${FMOD_CORE_INCLUDE_DIR}")
        
        # Check directories exist
        if(NOT EXISTS "${FMOD_CORE_LIB_DIR}")
            message(FATAL_ERROR "FMOD library directory not found: ${FMOD_CORE_LIB_DIR}")
        endif()
        
        if(NOT EXISTS "${FMOD_CORE_INCLUDE_DIR}")
            message(FATAL_ERROR "FMOD include directory not found: ${FMOD_CORE_INCLUDE_DIR}")
        endif()
        
        # Find FMOD Core libraries
        find_library(FMOD_LIBRARY
            NAMES fmod_vc fmod
            PATHS ${FMOD_CORE_LIB_DIR}
            NO_DEFAULT_PATH
        )
        
        find_library(FMODL_LIBRARY
            NAMES fmodL_vc fmodL
            PATHS ${FMOD_CORE_LIB_DIR}
            NO_DEFAULT_PATH
        )
        
        if(NOT FMOD_LIBRARY)
            message(FATAL_ERROR "FMOD library not found in: ${FMOD_CORE_LIB_DIR}")
        endif()
        
        # Create FMOD Core library
        add_library(fmod INTERFACE)
        target_include_directories(fmod INTERFACE "${FMOD_CORE_INCLUDE_DIR}")
        
        if(FMODL_LIBRARY)
            target_link_libraries(fmod INTERFACE 
                $<$<CONFIG:Debug>:${FMODL_LIBRARY}>
                $<$<NOT:$<CONFIG:Debug>>:${FMOD_LIBRARY}>
            )
        else()
            target_link_libraries(fmod INTERFACE ${FMOD_LIBRARY})
        endif()
        
        set(FMOD_FOUND TRUE CACHE BOOL "FMOD library found" FORCE)
        message(STATUS " FMOD Core imported successfully!")
        message(STATUS "  Release: ${FMOD_LIBRARY}")
        if(FMODL_LIBRARY)
            message(STATUS "  Debug: ${FMODL_LIBRARY}")
        endif()
        
        # Find FMOD Studio libraries
        find_library(FMOD_STUDIO_LIBRARY
            NAMES fmodstudio_vc fmodstudio
            PATHS ${FMOD_STUDIO_LIB_DIR}
            NO_DEFAULT_PATH
        )
        
        find_library(FMOD_STUDIOL_LIBRARY
            NAMES fmodstudioL_vc fmodstudioL
            PATHS ${FMOD_STUDIO_LIB_DIR}
            NO_DEFAULT_PATH
        )
        
        if(FMOD_STUDIO_LIBRARY)
            add_library(fmod_studio INTERFACE)
            target_include_directories(fmod_studio INTERFACE "${FMOD_STUDIO_INCLUDE_DIR}")
            
            if(FMOD_STUDIOL_LIBRARY)
                target_link_libraries(fmod_studio INTERFACE 
                    $<$<CONFIG:Debug>:${FMOD_STUDIOL_LIBRARY}>
                    $<$<NOT:$<CONFIG:Debug>>:${FMOD_STUDIO_LIBRARY}>
                    fmod
                )
            else()
                target_link_libraries(fmod_studio INTERFACE ${FMOD_STUDIO_LIBRARY} fmod)
            endif()
            
            message(STATUS "FMOD Studio imported successfully!")
        endif()
        
        message(STATUS "=== FMOD Import Complete ===")
    endif()
endmacro()

# Main function to import all dependencies
function(importDependencies)
    message(STATUS "=== Importing Dependencies ===")
    
    # Import dependencies in correct order (dependencies first)
    import_glfw()
    import_glm() 
    import_glew()
    import_imgui()
    import_stb()
    import_fmod()
    message(STATUS "=== All Dependencies Imported ===")
endfunction()