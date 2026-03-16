#include "rvelapch.h"
#include "GLAD/gl.h"
#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <Core/Log.h>

using namespace rv;

Texture::Texture()
{
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
    if (m_Texture != 0)
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
        LOG_ERROR("Failed to load texture: {}", path);
    }
    stbi_image_free(m_Data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::GenerateFromMemory(const uint8_t* data, int width, int height, TextureFormat format, bool srgb)
{
    Bind();

    if (!data)
    {
        LOG_ERROR("Texture data is null!");
        return;
    }

    if (width <= 0 || height <= 0)
    {
        LOG_ERROR("Invalid texture dimensions: {}x{}", width, height);
        return;
    }

    m_Width = width;
    m_Height = height;

    GLenum internalFormat;
    GLenum dataFormat;

    switch (format)
    {
    case TextureFormat::RGBA8:
        internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        dataFormat = GL_RGBA;
        break;
    case TextureFormat::RGB8:
        internalFormat = srgb ? GL_SRGB8 : GL_RGB8;
        dataFormat = GL_RGB;
        break;
    case TextureFormat::R8:
        internalFormat = GL_R8;
        dataFormat = GL_RED;
        break;
    default:
        LOG_ERROR("Unsupported texture format.");
        return;
    }


    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
        width, height, 0,
        dataFormat, GL_UNSIGNED_BYTE, data);

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
            LOG_ERROR("Unsupported number of channels: {}", nrChannels);
            return;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
}

void Texture::GenerateMipmaps()
{
    Bind();

    glGenerateMipmap(GL_TEXTURE_2D);
    int maxLevel = static_cast<int>(std::floor(std::log2(std::max(m_Width, m_Height))));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxLevel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        maxLevel > 0 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
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