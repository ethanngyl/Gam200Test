/*
===============================================================================
 File:          Shader.cpp
 Author:        TAN WEI LEONG
 Email:         weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  100%
 ------------------------------------------------------------------------------
 Implementation of the `Shader` class, which encapsulates the OpenGL logic required
 to manage and use shader programs for rendering.

 Description:
 -------------
 This file implements the methods of the `Shader` class, responsible for loading,
 compiling, linking, and using OpenGL shader programs (both vertex and fragment shaders).
 The class provides functions to bind and unbind shaders, set uniforms, and manage
 the shader program lifecycle.

 Responsibilities:
 -----------------
 - `Shader()`: Loads, compiles, and links vertex and fragment shaders to create an OpenGL shader program.
 - `~Shader()`: Cleans up the shader program from OpenGL memory when the shader is destroyed.
 - `Bind()`: Activates the shader program for rendering in OpenGL.
 - `Unbind()`: Deactivates the shader program.
 - `SetUniform3f()`: Sets a 3D vector (`vec3`) uniform in the shader program.
 - `GetID()`: Returns the OpenGL ID of the shader program.

 Platform-specific Notes:
 -------------------------
 - The class requires a valid OpenGL context and proper paths to the shader files.
 - The shader files should be compiled and linked with the OpenGL program, which this
   class helps manage.

 Safety:
 --------
 - Shader programs are properly cleaned up using `glDeleteProgram` in the destructor.
 - The methods provide necessary error handling for shader compilation and linking.
===============================================================================
*/

#include "Precompiled.h"  // Includes precompiled headers necessary for the graphics system.

namespace Framework {

    /**
     * @brief Constructs the Shader program by loading, compiling, and linking
     *        vertex and fragment shaders from the given file paths.
     *
     * This constructor reads shader source code from the provided file paths,
     * compiles the shaders, links them into a program, and stores the program's
     * OpenGL ID. If the shaders cannot be compiled or linked, error messages
     * will be displayed.
     *
     * @param vertexPath The file path to the vertex shader source code.
     * @param fragmentPath The file path to the fragment shader source code.
     */
    Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
        std::cout << "\n\n==============================================\n";
        std::cout << "    VERTEX & FRAGMENT SHADER SRC FILES\n";
        std::cout << "==============================================\n\n";

        // Load vertex and fragment shader source code
        std::string vertexSrc = LoadFile(vertexPath);
        std::cout << "Vertex Shader Source:\n" << vertexSrc << "\n";

        std::string fragmentSrc = LoadFile(fragmentPath);
        std::cout << "Fragment Shader Source:\n" << fragmentSrc << "\n";

        // Compile the shaders
        unsigned int vs = Compile(GL_VERTEX_SHADER, vertexSrc);
        unsigned int fs = Compile(GL_FRAGMENT_SHADER, fragmentSrc);

        // Create shader program, attach shaders, and link the program
        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);

        // Check for linking errors
        int success;
        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(id, 512, nullptr, infoLog);
            std::cerr << "Shader link failed:\n" << infoLog << "\n";
        }

        // Clean up shaders after linking
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    /**
     * @brief Destructor that cleans up the shader program.
     *
     * This destructor deletes the shader program to free the OpenGL resources when
     * the Shader object is destroyed.
     */
    Shader::~Shader() {
        glDeleteProgram(id);
    }

    /**
     * @brief Binds the shader program for use in OpenGL rendering.
     *
     * This function activates the shader program, making it the current program
     * for subsequent OpenGL rendering calls.
     */
    void Shader::Bind() const {
        glUseProgram(id);
    }

    /**
     * @brief Unbinds the currently active shader program.
     *
     * This function deactivates the shader program and returns OpenGL to its default
     * state (no program bound).
     */
    void Shader::Unbind() const {
        glUseProgram(0);
    }

    /**
    * @brief Sets a 3D vector (`vec3`) uniform in the shader program.
    *
    * This function sets the value of a `vec3` uniform in the shader, typically used
    * for properties like colors, light positions, or other 3D vector attributes.
    *
    * @param name The name of the uniform variable in the shader.
    * @param x The x-component of the vector.
    * @param y The y-component of the vector.
    * @param z The z-component of the vector.
    */
    void Shader::SetUniform3f(const std::string& name, float x, float y, float z) const {
        // Retrieve the location of the uniform variable in the shader program
        int location = glGetUniformLocation(id, name.c_str());

        // If the uniform was not found, print a warning message
        if (location == -1) {
            std::cerr << "Warning: uniform '" << name << "' not found!" << std::endl;
        }

        // Set the value of the uniform to the provided 3D vector
        glUniform3f(location, x, y, z);
    }

    /**
     * @brief Retrieves the OpenGL ID of the shader program.
     *
     * This function returns the OpenGL program ID associated with this shader.
     * The ID can be useful for debugging or managing multiple shader programs.
     *
     * @return The OpenGL ID of the shader program.
     */
    unsigned int Shader::GetID() const {
        return id;
    }

    /**
     * @brief Loads the shader source code from a file.
     *
     * This function reads the content of a shader file and returns the source code
     * as a string.
     *
     * @param path The path to the shader file.
     * @return The source code as a string.
     */
    std::string Shader::LoadFile(const std::string& path) {
        // Open the shader file at the given path
        std::ifstream file(path);

        // Check if the file opened successfully
        if (!file) {
            std::cerr << "ERROR: Failed to open shader file: " << path << std::endl;
            return ""; // Return an empty string on failure
        }

        // Read the entire content of the file into a stringstream
        std::stringstream ss;
        ss << file.rdbuf();

        // Return the shader source code as a string
        return ss.str();
    }

    /**
     * @brief Compiles a shader from source code.
     *
     * This function compiles a shader (either vertex or fragment) from the provided
     * source code and returns the OpenGL ID of the compiled shader. Compilation errors
     * will be logged.
     *
     * @param type The type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
     * @param source The source code of the shader.
     * @return The OpenGL ID of the compiled shader.
     */
    unsigned int Shader::Compile(unsigned int type, const std::string& source) {
        // Create a new shader object of the specified type
        unsigned int shader = glCreateShader(type);

        // Convert the shader source code to a C-style string
        const char* src = source.c_str();

        // Attach the source code to the shader object
        glShaderSource(shader, 1, &src, nullptr);

        // Compile the shader
        glCompileShader(shader);

        // Check if the shader compiled successfully
        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        // If compilation failed, print the error log
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << (type == GL_VERTEX_SHADER ? "\nVertex" : "Fragment")
                << " shader compile error:\n" << infoLog << "\n";
        }
        // If compilation succeeded, print a success message
        else {
            std::cout << (type == GL_VERTEX_SHADER ? "\nVertex" : "Fragment")
                << " shader compiled successfully\n";
        }

        // Return the compiled shader object ID
        return shader;
    }

}  // namespace Framework