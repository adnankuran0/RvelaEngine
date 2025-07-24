#include "rvelapch.h"

#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture()
{
    std::cout << "Texture created!\n";

	Init();
}

Texture::~Texture()
{
    Destroy();
}

Texture::Texture(const std::string& path)
{
    Init();
    GenerateFromImage(path);
}

void Texture::Init()
{
    glGenTextures(1, &m_Texture);
    Bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    float maxAniso = 8.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
}

void Texture::Bind(unsigned int activeTexture) const
{
    glBindTextureUnit(activeTexture, m_Texture);
}

void Texture::Bind() const
{
    glBindTexture(GL_TEXTURE_2D, m_Texture);
}

void Texture::Destroy() const
{
    std::cout << "Texture destroyed!\n";
    glDeleteTextures(1, &m_Texture);
}

void Texture::GenerateFromImage(const std::string& path)
{
    m_Path = path;

    m_Data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_NrChannels, 0);

    if (m_Data)
    {
        ToImage(m_Width, m_Height, m_Data, m_NrChannels);
        GenerateMipmaps();
    }
    else
    {
        std::cerr << "Failed to load texture: " << path;
    }
    stbi_image_free(m_Data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::GenerateFromMemory(const uint8_t* data, int width, int height, GLenum format, bool srgb)
{
    Bind();

    GLenum internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    GLenum dataFormat = GL_RGBA; // assuming 4 channels

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

    GenerateMipmaps();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::ToImage(int width, int height, const unsigned char* data, int nrChannels)
{
    GLenum format;
    switch (nrChannels) {
        case 1: format = GL_RED; break;
        case 2: format = GL_RG; break;
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default:
            std::cerr << "Unsupported number of channels: " << nrChannels << std::endl;
            return;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
}

void Texture::GenerateMipmaps()
{
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    if (maxAniso > 0.0f)
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
    }
}

Texture Texture::Create()
{
    return Texture{};
}

unsigned int Texture::GetID() const
{
    return m_Texture;
}

int Texture::GetWidth() const
{
    return m_Width;
}

int Texture::GetHeight() const
{
    return m_Height;
}

unsigned char* Texture::GetTexture() const
{
    return m_Data;
}

int Texture::GetNrChannels() const
{
    return m_NrChannels;
}

const std::string& Texture::GetPath() 
{
    return m_Path;
}
