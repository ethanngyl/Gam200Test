#include "Precompiled.h"
/*
===============================================================================
File:        Shader.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 60%(kah yan)
-------------------------------------------------------------------------------
Brief:
Implementation of the Shader class, which handles loading, compiling, linking,
and managing OpenGL GLSL shader programs. This includes reading source files
from disk, compiling vertex and fragment shaders, linking them into a program,
and providing utility functions for binding and setting uniforms.

Details:
- Reads shader source code from text files.
- Compiles vertex and fragment shaders, checking for compilation errors.
- Links shaders into a complete program and validates link status.
- Deletes intermediate shader objects after linking.
- Provides functions to bind/unbind the shader and set vec3 uniforms.

Notes:
- Requires a valid OpenGL context prior to shader creation.
- Uses GLSL version 4.5 Core Profile.
- Prints shader sources and compile/link results to console for debugging.

Safety:
- Compilation and linking errors are logged to stderr.
- Throws std::runtime_error on linking failure to prevent undefined behavior.
- Gracefully handles missing shader files.

===============================================================================
*/

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

        // Load shader source code from external files
        std::string vertexSrc = LoadFile(vertexPath);
        std::cout << "Vertex Shader Source:\n" << vertexSrc << "\n";

        std::string fragmentSrc = LoadFile(fragmentPath);
        std::cout << "Fragment Shader Source:\n" << fragmentSrc << "\n";

        // Compile vertex and fragment shaders separately
        unsigned int vs = Compile(GL_VERTEX_SHADER, vertexSrc);
        unsigned int fs = Compile(GL_FRAGMENT_SHADER, fragmentSrc);

        // Create a new shader program and attach the compiled shaders
        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);

        // Check if program linking succeeded
        int success;
        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(id, 512, nullptr, infoLog);
            std::cerr << "Shader link failed:\n" << infoLog << "\n";
            throw std::runtime_error("Shader Link failed");
        }

        // Shaders are now linked into the program, so we can delete the originals
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    // Destructor: Deletes the compiled shader program from GPU memory.
    Shader::~Shader() {
        glDeleteProgram(id);
    }

    // Bind: Activates this shader program for subsequent draw calls.
    void Shader::Bind() const {
        glUseProgram(id);
    }

    // Unbind: Deactivates any currently bound shader program.
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
        // Create a new shader object of the requested type
        unsigned int shader = glCreateShader(type);

        // Convert the shader source code to a C-style string
        const char* src = source.c_str();

        // Attach the source code to the shader object
        glShaderSource(shader, 1, &src, nullptr);

        // Compile the shader
        glCompileShader(shader);

        // Check compilation result
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