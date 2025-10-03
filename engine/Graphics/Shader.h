#pragma once
#include "Precompiled.h"
/*
===============================================================================
File:        Shader.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 60%(kah yan)
-------------------------------------------------------------------------------
Brief:
Declaration of the Shader class, which encapsulates the loading, compilation,
linking, and management of GLSL vertex and fragment shaders for OpenGL.
Provides utility functions to bind/unbind the shader and set uniform variables.

Details:
- Loads shader source code from external GLSL files.
- Compiles vertex and fragment shaders and links them into a single program.
- Provides methods to activate (bind) and deactivate (unbind) the shader.
- Supports setting 3-component float uniforms (e.g., color vectors).
- Exposes the OpenGL program ID for advanced operations if needed.

Notes:
- Requires a valid OpenGL context before creating a Shader object.
- Uses GLSL version 4.5 Core Profile.
- Designed for use with modern OpenGL rendering.

Safety:
- All shader compilation and linking errors should be handled in the implementation.
- Shader program ID is stored and can be retrieved for external uniform handling.

===============================================================================
*/

namespace Framework {

    /*
    ------------------------------------------------------------------------------
    Class: Shader
    Encapsulates an OpenGL GLSL shader program consisting of a vertex and fragment
    shader. Handles loading, compiling, linking, and usage of the shader.
    ------------------------------------------------------------------------------
    */
    class Shader {
    public:
        /*
        ------------------------------------------------------------------------------
        Constructor: Loads, compiles, and links a vertex and fragment shader from
                    the specified file paths.
        @param vertexPath   Path to the vertex shader source file.
        @param fragmentPath Path to the fragment shader source file.
        ------------------------------------------------------------------------------
        */
        Shader(const std::string& vertexPath, const std::string& fragmentPath);
        // Destructor: Deletes the shader program from the GPU.
        ~Shader();

        // Bind: Activates the shader program for subsequent OpenGL draw calls.
        void Bind() const;
        // Unbind: Deactivates the currently bound shader program.
        void Unbind() const;

        /**
         * @brief Sets a 3D vector uniform in the shader program.
         *
         * This function sends a `vec3` (x, y, z) value to the shader as a uniform.
         * It can be used to set properties such as light positions, colors, etc.,
         * directly in the shader.
         *
         * @param name The name of the uniform variable in the shader.
         * @param x The x-component of the vector.
         * @param y The y-component of the vector.
         * @param z The z-component of the vector.
         */
        void SetUniform3f(const std::string& name, float x, float y, float z) const;

        /**
         * @brief Retrieves the OpenGL ID of the shader program.
         *
         * This function returns the OpenGL program ID associated with this shader.
         * The program ID can be used for debugging purposes or when managing
         * multiple shader programs.
         *
         * @return The OpenGL ID of the shader program.
         */
        unsigned int GetID() const;

    private:
        unsigned int id;// OpenGL handle to the compiled and linked shader program

        /**
         * @brief Loads the shader source code from a file.
         *
         * This function reads the contents of a shader file and returns the source
         * code as a string. The source code is used for shader compilation.
         *
         * @param path The path to the shader file.
         * @return A string containing the shader source code.
         */
        std::string LoadFile(const std::string& path);

        /**
         * @brief Compiles a shader from source code.
         *
         * This function compiles a shader (either vertex or fragment) from the
         * provided source code. It returns the OpenGL ID of the compiled shader.
         *
         * @param type The type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
         * @param source The source code of the shader.
         * @return The OpenGL ID of the compiled shader.
         */
        unsigned int Compile(unsigned int type, const std::string& source);
    };

}  // namespace Framework