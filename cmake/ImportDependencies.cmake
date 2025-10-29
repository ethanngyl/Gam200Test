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
        message(STATUS "Importing FMOD...")
        
        # Set FMOD root to your library folder
        set(FMOD_ROOT "${CMAKE_SOURCE_DIR}/library/FMOD Studio API Windows")
        
        # Set paths
        set(FMOD_CORE_LIB_DIR "${FMOD_ROOT}/api/core/lib/x64")
        set(FMOD_STUDIO_LIB_DIR "${FMOD_ROOT}/api/studio/lib/x64")
        set(FMOD_CORE_INCLUDE_DIR "${FMOD_ROOT}/api/core/inc")
        set(FMOD_STUDIO_INCLUDE_DIR "${FMOD_ROOT}/api/studio/inc")
        
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
            message(FATAL_ERROR "FMOD library not found at: ${FMOD_CORE_LIB_DIR}")
        endif()
        
        # Create FMOD Core target
        add_library(fmod INTERFACE)
        target_include_directories(fmod INTERFACE "${FMOD_CORE_INCLUDE_DIR}")
        target_link_libraries(fmod INTERFACE 
            $<$<CONFIG:Debug>:${FMODL_LIBRARY}>
            $<$<NOT:$<CONFIG:Debug>>:${FMOD_LIBRARY}>
        )
        
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
            target_link_libraries(fmod_studio INTERFACE 
                $<$<CONFIG:Debug>:${FMOD_STUDIOL_LIBRARY}>
                $<$<NOT:$<CONFIG:Debug>>:${FMOD_STUDIO_LIBRARY}>
                fmod
            )
        endif()
        
        message(STATUS "FMOD imported successfully")
    endif()
endmacro()

# Macro to import FreeType
macro(import_freetype)
    if(NOT TARGET Freetype::Freetype)
        message(STATUS "Importing FreeType...")
        include(FetchContent)
        # Pull a stable tag
        FetchContent_Declare(
            freetype
            GIT_REPOSITORY https://github.com/freetype/freetype.git
            GIT_TAG VER-2-13-2
        )
        # Prefer a static library to avoid DLL hassle
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(freetype)

        # Some versions export target "freetype"; add an alias for consistency
        if(TARGET freetype AND NOT TARGET Freetype::Freetype)
            add_library(Freetype::Freetype ALIAS freetype)
        endif()

        message(STATUS "FreeType imported successfully")
    endif()
endmacro()


# Main function to import all dependencies
function(importDependencies)
    message(STATUS "=== Importing Dependencies ===")
    
    # Import dependencies in correct order (dependencies first)
    import_glfw()
    import_glm() 
    import_glew()
    import_freetype() 
    import_imgui()
    import_stb()
    import_fmod()
    message(STATUS "=== All Dependencies Imported ===")
endfunction()