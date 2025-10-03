/*
===============================================================================
 File:          Shader.h
 Author:        Sim Kah Yan, TAN WEI LEONG
 Email:         kahyan.sim@digipen.edu, weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  60%(kah yan),  40%(TAN WEI LEONG)
 ------------------------------------------------------------------------------

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

#pragma once
#include "Precompiled.h"  // Includes precompiled headers necessary for the graphics system.

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

        /*
        ------------------------------------------------------------------------------
        SetUniform3f: Sets a vec3 uniform variable in the shader program.
        @param name Name of the uniform variable in GLSL.
        @param x    X component of the vector.
        @param y    Y component of the vector.
        @param z    Z component of the vector.
        ------------------------------------------------------------------------------
        */
        void SetUniform3f(const std::string& name, float x, float y, float z) const;

        // GetID: Returns the OpenGL program ID for this shader.
        unsigned int GetID() const;

    private:
        unsigned int id;  // OpenGL shader program ID

        /*
        ------------------------------------------------------------------------------
        LoadFile: Loads shader source code from a file.
        @param path Path to the shader source file.
        @return Contents of the file as a single string.
        ------------------------------------------------------------------------------
        */
        std::string LoadFile(const std::string& path);

        /*
        ------------------------------------------------------------------------------
        Compile: Compiles a shader of the given type (vertex or fragment) from source.
        @param type   GLenum specifying shader type (GL_VERTEX_SHADER / GL_FRAGMENT_SHADER).
        @param source Shader source code as a string.
        @return OpenGL handle to the compiled shader object.
        ------------------------------------------------------------------------------
        */
        unsigned int Compile(unsigned int type, const std::string& source);
    };

}  // namespace Framework