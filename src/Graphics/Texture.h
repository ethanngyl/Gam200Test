#pragma once
#include <string>
/*
===============================================================================
File:        Texture.h
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Declaration of the Texture class, which encapsulates loading, creating, and
binding 2D textures for OpenGL rendering. Supports loading image files from
disk, uploading them to the GPU, and binding them to specific texture units.

Details:
- Loads texture data from image files using an external image loader (e.g., stb_image).
- Generates an OpenGL texture object and configures filtering/wrapping parameters.
- Provides functions to bind/unbind the texture for use in rendering.
- Stores texture dimensions and channel count for reference.

Notes:
- Requires a valid OpenGL context before loading textures.
- Default texture unit is 0, but custom slots can be specified.
- Supports only 2D textures (GL_TEXTURE_2D).

Safety:
- Texture ID is properly deleted in the destructor to avoid GPU memory leaks.
- LoadFromFile should be called before using Bind().

===============================================================================
*/

namespace Framework {
    /*
    ------------------------------------------------------------------------------
    Class: Texture
    Manages the lifetime and usage of a 2D OpenGL texture. Handles loading from
    image files, uploading to GPU, and binding for use in shaders.
    ------------------------------------------------------------------------------
    */
    class Texture {
    public:

        // Constructor: Initializes texture properties to safe defaults.
        Texture();

        // Destructor: Deletes the texture object from GPU memory.
        ~Texture();

        /*
        ------------------------------------------------------------------------------
        LoadFromFile: Loads an image file from disk and creates an OpenGL texture.
        @param path File path to the image (e.g. PNG, JPG).
        @return true if the texture was loaded and created successfully.
        ------------------------------------------------------------------------------
        */
        bool LoadFromFile(const std::string& path);

        /*
        ------------------------------------------------------------------------------
        Bind: Binds the texture to the specified texture unit for rendering.
        @param slot The texture unit to bind to (default = 0).
        ------------------------------------------------------------------------------
        */
        void Bind(unsigned int slot = 0) const;

        // Unbind: Unbinds the currently bound texture from the active texture unit.
        void Unbind() const;

        /*
        ------------------------------------------------------------------------------
        SetFilterMode: Changes texture filtering mode.
        @param useNearest If true, uses GL_NEAREST (pixel-perfect, no filtering).
                         If false, uses GL_LINEAR (smooth filtering).
        Use GL_NEAREST for pixel art and UI to avoid filtering artifacts.
        ------------------------------------------------------------------------------
        */
        void SetFilterMode(bool useNearest);

        // GetID: Returns the OpenGL texture object ID.
        unsigned GetID() const { return id; }

        // <-- ADD THESE TWO LINES
        int GetWidth()  const { return width; }
        int GetHeight() const { return height; }

    private:
        unsigned int id;// OpenGL texture object handle
        int width, height, nrChannels;// Width and Height of the loaded texture in pixels, and Number of color channels (e.g. 3 = RGB, 4 = RGBA)
    };
}
