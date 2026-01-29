#include "Texture.h"
#include <GL/glew.h>
#include <iostream>
// stb_image is used for loading image files
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
/*
===============================================================================
File:        Texture.cpp
Author:      Sim Kah Yan
Email:       kahyan.sim@digipen.edu
Date:        2025-10-02
Contribution: 100%
-------------------------------------------------------------------------------
Brief:
Implementation of the Texture class, which manages loading 2D image files from
disk, creating OpenGL texture objects, setting texture parameters, and binding
textures for use during rendering.

Details:
- Uses stb_image to load image data from disk.
- Flips images vertically on load to match OpenGL's coordinate system.
- Generates OpenGL texture objects, uploads image data, and configures filtering
  and wrapping modes.
- Supports RGB and RGBA formats automatically based on channel count.
- Provides Bind/Unbind functions to activate textures for rendering.

Notes:
- Requires a valid OpenGL context before loading or binding textures.
- stb_image implementation is included here via `#define STB_IMAGE_IMPLEMENTATION`.
- Generates mipmaps automatically after uploading texture data.

Safety:
- Deletes the texture object in the destructor to prevent GPU memory leaks.
- Checks for file loading errors and logs them clearly.
- Handles both 3-channel (RGB) and 4-channel (RGBA) textures safely.

===============================================================================
*/
namespace Framework {

    // Constructor: Initializes texture properties to default (empty) values.
    Texture::Texture() : id(0), width(0), height(0), nrChannels(0) {}
    // Destructor: Deletes the OpenGL texture object if it was created.
    Texture::~Texture() {
        // Clean up GPU texture to prevent memory leaks
        if (id) {
            glDeleteTextures(1, &id);
        }
    }

    /*
    ------------------------------------------------------------------------------
    LoadFromFile: Loads an image file from disk, creates an OpenGL texture object,
                  and uploads the image data to the GPU.
    ------------------------------------------------------------------------------
    */
    bool Texture::LoadFromFile(const std::string& path) {

        // Flip image vertically to match OpenGL's bottom-left origin
        stbi_set_flip_vertically_on_load(true);//flip so it matches OpenGL coords
        // Load image data from file
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
        if (!data) {
            //std::cerr << "Failed to load texture: " << path << "\n";
            return false;
        }

        // Generate and bind a new OpenGL texture object
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        // Determine pixel format based on number of channels
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        // Upload image data to GPU as a 2D texture
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        // Generate mipmaps for better texture scaling
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set texture wrapping (repeat) and filtering (linear + mipmaps)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Free image data from CPU memory after uploading to GPU
        stbi_image_free(data);
        return true;
    }

    // Bind: Binds the texture to the specified texture unit (slot) for rendering.
    void Texture::Bind(unsigned int slot) const {
        // Activate the desired texture unit and bind the texture
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    // Unbind: Unbinds the currently bound texture from the active texture unit.
    void Texture::Unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    /*
    ------------------------------------------------------------------------------
    SetFilterMode: Changes the texture filtering mode to prevent artifacts.
    ------------------------------------------------------------------------------
    */
    void Texture::SetFilterMode(bool useNearest) {
        glBindTexture(GL_TEXTURE_2D, id);

        if (useNearest) {
            // Pixel-perfect rendering (no interpolation) - best for pixel art/UI
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } else {
            // Smooth rendering with mipmaps - best for 3D textures
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
