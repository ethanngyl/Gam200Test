/*
===============================================================================
 File:          Shader.h
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  100%
 ------------------------------------------------------------------------------
 Declaration of the Shader class, which encapsulates the OpenGL logic required
 to manage and use shader programs for rendering.

 Description:
 -------------
 This file declares the `Shader` class, which handles loading, compiling, linking, 
 and using OpenGL shader programs (vertex and fragment shaders). The class provides 
 functions to bind the shader program, set shader uniforms, and unbind the shader.

 Responsibilities:
 -----------------
 - `Shader()`: Initializes a shader program by loading and compiling vertex and 
   fragment shaders from the provided file paths.
 - `~Shader()`: Cleans up the shader program from OpenGL memory when the object 
   is destroyed.
 - `Bind()`: Binds the shader program, making it the active shader for rendering.
 - `Unbind()`: Unbinds the shader program, disabling it.
 - `SetUniform3f()`: Sets a `vec3` uniform in the shader program.
 - `GetID()`: Returns the OpenGL ID of the shader program, which can be useful 
   for debugging or managing multiple shaders.

 Platform-specific Notes:
 -------------------------
 - The class assumes an OpenGL context has already been initialized in the 
   application. The shader files must be valid and accessible.
 - Ensure the paths to the shader source files are correct and that the OpenGL 
   context is available when creating the shader.

 Safety:
 --------
 - Proper OpenGL resource cleanup is ensured by deleting the shader program 
   during destruction, preventing memory leaks.
 - The class does not perform automatic error checking during shader compilation 
   or linking. It is recommended to check for compilation errors externally or 
   add error handling in the `Shader` class itself.
===============================================================================
*/

#pragma once
#include "Precompiled.h"  // Includes precompiled headers necessary for the graphics system.

namespace Framework {

    /**
     * @brief The Shader class manages OpenGL shader programs (vertex and fragment shaders).
     *
     * This class provides functionality for loading shader files, compiling and linking
     * shaders into a program, and using that program in OpenGL rendering.
     */
    class Shader {
    public:
        /**
         * @brief Constructs a Shader object by loading and compiling vertex and
         *        fragment shaders from the given file paths.
         *
         * This constructor reads the vertex and fragment shader source code from
         * the specified paths, compiles the shaders, links them into a program,
         * and stores the program's OpenGL ID.
         *
         * @param vertexPath The file path to the vertex shader source code.
         * @param fragmentPath The file path to the fragment shader source code.
         */
        Shader(const std::string& vertexPath, const std::string& fragmentPath);

        /**
         * @brief Destructor that cleans up the shader program from OpenGL memory.
         *
         * This destructor deletes the shader program using `glDeleteProgram`, freeing
         * the resources associated with the shader program.
         */
        ~Shader();

        /**
         * @brief Binds the shader program for use in OpenGL rendering.
         *
         * This function activates the shader program, making it the current program
         * for subsequent OpenGL rendering calls.
         */
        void Bind() const;

        /**
         * @brief Unbinds the currently active shader program.
         *
         * This function disables the shader program, effectively unbinding it and
         * returning OpenGL to its default state (no program bound).
         */
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
        unsigned int id;  // OpenGL shader program ID

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