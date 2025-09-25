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

# Main function to import all dependencies
function(importDependencies)
    message(STATUS "=== Importing Dependencies ===")
    
    # Import dependencies in correct order (dependencies first)
    import_glfw()
    # import_glm() 
    import_glew()
    import_imgui()
    
    message(STATUS "=== All Dependencies Imported ===")
endfunction()