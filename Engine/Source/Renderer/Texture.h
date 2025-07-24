#pragma once
#include "GLAD/gl.h"


class Texture
{
public:
    Texture();
    ~Texture();
    Texture(const std::string& path);
    Texture(const Texture&) { LOG_INFO("Texture copied!"); }
    void Init();
    void Destroy() const;
    void Bind(unsigned int activeTexture) const;
    void Bind() const;
    void GenerateFromImage(const std::string& path);
    void GenerateFromMemory(const uint8_t* data, int width, int height, GLenum format = GL_RGBA8, bool srgb = false);

    static Texture Create();
    static void ToImage(int width, int height, const unsigned char* data,int nrChannels);
    static void GenerateMipmaps();

    unsigned int GetID() const;
    inline unsigned char* GetTexture() const;
    inline int GetWidth() const;
    inline int GetHeight() const;
    inline int GetNrChannels() const;
    inline const std::string& GetPath() ;

private:
    std::string m_Path;
    unsigned int m_Texture{};
    unsigned char* m_Data{};
    int m_Width{}, m_Height{}, m_NrChannels{};
};

