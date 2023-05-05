#pragma once
#include "Renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "string"

class Texture {
private:
    unsigned int m_RendererID;
    std::string m_FilePath;
    unsigned char* m_LocalBuffer;
    int m_Width, m_Height, m_BPP;
public:
    Texture(const std::string& path);
    Texture(Texture&& other) noexcept;
    ~Texture();

    void Bind(unsigned int slot = 0) const;
    void Bind(unsigned int slot, uint32_t texture_type) const;
    void Unbind() const;

    inline int GetWidth() const { return m_Width; }
    inline int GetHeight() const { return m_Height; }
};