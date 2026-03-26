#include "rvelapch.h"
#include "GLAD/gl.h"
#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <Core/Log.h>

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

#ifndef GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT        0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT  0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT  0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT  0x8C4F
#endif

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

void Texture::Destroy()
{
    if (m_Texture != 0)
    {
        glDeleteTextures(1, &m_Texture);
        m_Texture = 0;
    }
}

void Texture::GenerateFromImage(const std::string& path)
{
    m_Path = path;

    unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_NrChannels, 0);

    if (data)
    {
        ToImage(m_Width, m_Height, data, m_NrChannels);
        GenerateMipmaps();
    }
    else
    {
        LOG_ERROR("Failed to load texture: {}", path);
    }
    stbi_image_free(data);

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

void Texture::GenerateFromAsset(Ref<TextureAsset> asset)
{
    Bind();

    const uint8_t* ptr = asset->GetPixels().data();
    uint32_t mipW = asset->GetWidth();
    uint32_t mipH = asset->GetHeight();

    bool isCompressed = false;
    GLenum internalFormat;
    GLenum dataFormat = GL_RGBA;
    size_t bytesPerPixel = 0;

    switch (asset->GetFormat())
    {
    // UNCOMPRESSED
    case TextureFormat::RGBA8:
        internalFormat = asset->IsSRGB() ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        dataFormat = GL_RGBA;
        bytesPerPixel = 4;
        break;
    case TextureFormat::RGB8:
        internalFormat = asset->IsSRGB() ? GL_SRGB8 : GL_RGB8;
        dataFormat = GL_RGB;
        bytesPerPixel = 3;
        break;
    case TextureFormat::R8:
        internalFormat = GL_R8;
        dataFormat = GL_RED;
        bytesPerPixel = 1;
        break;
    case TextureFormat::RG8:
        internalFormat = GL_RG8;
        dataFormat = GL_RG;
        bytesPerPixel = 2;
        break;

    // COMPRESSED
    case TextureFormat::BC1:
        internalFormat = asset->IsSRGB() ? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
            : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        isCompressed = true;
        break;
    case TextureFormat::BC3:
        internalFormat = asset->IsSRGB() ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
            : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        isCompressed = true;
        break;
    case TextureFormat::BC4:
        internalFormat = GL_COMPRESSED_RED_RGTC1;
        isCompressed = true;
        break;
    case TextureFormat::BC5:
        internalFormat = GL_COMPRESSED_RG_RGTC2;
        isCompressed = true;
        break;

    default:
        LOG_ERROR("Unsupported texture format in GenerateFromAsset");
        return;
    }

    for (uint16_t i = 0; i < asset->GetMipCount(); i++)
    {
        if (isCompressed)
        {
            size_t blockSize = (asset->GetFormat() == TextureFormat::BC1 ||
                asset->GetFormat() == TextureFormat::BC4) ? 8 : 16;

            size_t mipSize = std::max(1u, (mipW + 3) / 4) *
                std::max(1u, (mipH + 3) / 4) *
                blockSize;

            glCompressedTexImage2D(GL_TEXTURE_2D, i, internalFormat,
                mipW, mipH, 0,
                (GLsizei)mipSize, ptr);
            ptr += mipSize;
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, i, internalFormat,
                mipW, mipH, 0,
                dataFormat, GL_UNSIGNED_BYTE, ptr);
            ptr += mipW * mipH * bytesPerPixel;
        }

        mipW = std::max(1u, mipW / 2);
        mipH = std::max(1u, mipH / 2);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, asset->GetMipCount() - 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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

int Texture::GetNrChannels() const
{
    return m_NrChannels;
}

const std::string& Texture::GetPath() 
{
    return m_Path;
}