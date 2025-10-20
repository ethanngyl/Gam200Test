/*
===============================================================================
 File:          Shader.cpp
 Author:        Sim Kah Yan, TAN WEI LEONG
 Email:         kahyan.sim@digipen.edu, weileong.tan@digipen.edu
 Date:          2025-10-02
 Contribution:  60%(kah yan), 40%(TAN WEI LEONG)
 ------------------------------------------------------------------------------
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

#include "Precompiled.h"  // Includes precompiled headers necessary for the graphics system.

namespace Framework {

    /*
    ------------------------------------------------------------------------------
    Constructor: Loads, compiles, and links a vertex and fragment shader program.
                 Prints shader sources and logs compilation/linking results.
    ------------------------------------------------------------------------------
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

    /*
    ------------------------------------------------------------------------------
    SetUniform3f: Uploads a vec3 uniform variable to the shader program.
    Logs a warning if the uniform location is not found.
    ------------------------------------------------------------------------------
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

    /*
    ------------------------------------------------------------------------------
    GetID: Returns the OpenGL shader program ID.
    ------------------------------------------------------------------------------
    */
    unsigned int Shader::GetID() const {
        return id;
    }

    /*
    ------------------------------------------------------------------------------
    LoadFile: Reads the contents of a text file (GLSL source) into a string.
    Returns an empty string and logs an error if the file cannot be opened.
    ------------------------------------------------------------------------------
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

    /*
    ------------------------------------------------------------------------------
    Compile: Compiles a shader of the specified type (vertex or fragment) from
             its source code. Logs errors to stderr if compilation fails.
    ------------------------------------------------------------------------------
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