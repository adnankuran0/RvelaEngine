#pragma once
#include "Asset/Types/TextureAsset.h"

namespace rv { 

class Texture
{
public:
    Texture();
    ~Texture();
    Texture(const std::string& path);
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept
        : m_Texture(other.m_Texture), m_Width(other.m_Width),
        m_Height(other.m_Height), m_NrChannels(other.m_NrChannels),
        m_Path(std::move(other.m_Path))
    {
        other.m_Texture = 0;
    }

    Texture& operator=(Texture&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            m_Texture = other.m_Texture;
            m_Width = other.m_Width;
            m_Height = other.m_Height;
            m_NrChannels = other.m_NrChannels;
            m_Path = std::move(other.m_Path);
            other.m_Texture = 0;
        }
        return *this;
    }

    void Init();
    void Destroy();
    void Bind(unsigned int activeTexture) const;
    void Bind() const;
    void GenerateFromImage(const std::string& path);
    void GenerateFromMemory(const uint8_t* data, int width, int height, TextureFormat format = TextureFormat::RGBA8, bool srgb = false);

    static Texture Create();
    static void ToImage(int width, int height, const unsigned char* data,int nrChannels);
    void GenerateMipmaps();

    unsigned int GetID() const;
    inline unsigned char* GetTexture() const;
    inline int GetWidth() const;
    inline int GetHeight() const;
    inline int GetNrChannels() const;
    inline const std::string& GetPath() ;

private:
    std::string m_Path;
    unsigned int m_Texture{};
    int m_Width{}, m_Height{}, m_NrChannels{};
};

}
