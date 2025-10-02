#pragma once
#include <string>


namespace Framework {
    class Texture {
    public:
        Texture();
        ~Texture();

        bool LoadFromFile(const std::string& path);
        void Bind(unsigned int slot = 0) const;
        void Unbind() const;

        unsigned GetID() const { return id; }

    private:
        unsigned int id;
        int width, height, nrChannels;
    };
}
