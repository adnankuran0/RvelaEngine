#include "rvelapch.h"
#include "TextureAsset.h"
#include "Core/Log.h"

#include <fstream>
#include <filesystem>
#include <cstring>
#include "GLAD/gl.h"

static std::vector<uint8_t> ReadTextureData(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("Failed to open file: {}", filePath.string());
        return {};
    }

    AssetHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file)
    {
        LOG_ERROR("Failed to read asset header.");
        return {};
    }

    size_t dataOffset = sizeof(AssetHeader) + header.metaSize;

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    if (fileSize < dataOffset)
    {
        LOG_ERROR("Invalid asset file size.");
        return {};
    }

    size_t dataSize = fileSize - dataOffset;
    std::vector<uint8_t> data(dataSize);

    file.seekg(dataOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), dataSize);

    return data;
}

std::unique_ptr<AssetMeta> TextureMeta::Clone() const
{
    return std::make_unique<TextureMeta>(*this);
}

void TextureMeta::Serialize(std::vector<char>& outBuffer, size_t& offset) const
{
    SerializeBase(outBuffer, offset);

    size_t totalSize =
        sizeof(width) +
        sizeof(height) +
        sizeof(mipCount) +
        sizeof(format) +
        sizeof(isSRGB) +
        sizeof(reserved);

    if (outBuffer.size() < offset + totalSize)
        outBuffer.resize(offset + totalSize);

    memcpy(outBuffer.data() + offset, &width, sizeof(width)); offset += sizeof(width);
    memcpy(outBuffer.data() + offset, &height, sizeof(height)); offset += sizeof(height);
    memcpy(outBuffer.data() + offset, &mipCount, sizeof(mipCount)); offset += sizeof(mipCount);
    memcpy(outBuffer.data() + offset, &format, sizeof(format)); offset += sizeof(format);
    memcpy(outBuffer.data() + offset, &isSRGB, sizeof(isSRGB)); offset += sizeof(isSRGB);
    memcpy(outBuffer.data() + offset, reserved, sizeof(reserved)); offset += sizeof(reserved);
}

void TextureMeta::Deserialize(const std::vector<char>& inBuffer, size_t& offset)
{
    DeserializeBase(inBuffer, offset);

    memcpy(&width, inBuffer.data() + offset, sizeof(width)); offset += sizeof(width);
    memcpy(&height, inBuffer.data() + offset, sizeof(height)); offset += sizeof(height);
    memcpy(&mipCount, inBuffer.data() + offset, sizeof(mipCount)); offset += sizeof(mipCount);
    memcpy(&format, inBuffer.data() + offset, sizeof(format)); offset += sizeof(format);
    memcpy(&isSRGB, inBuffer.data() + offset, sizeof(isSRGB)); offset += sizeof(isSRGB);
    memcpy(reserved, inBuffer.data() + offset, sizeof(reserved)); offset += sizeof(reserved);
}

TextureAsset::TextureAsset(const std::string& path, std::unique_ptr<TextureMeta> textureMeta)
    : Asset(std::move(textureMeta)), m_Path(path)
{
    m_Texture.Init();
}

TextureAsset::~TextureAsset()
{
    Unload();
}

bool TextureAsset::Load()
{
    if (m_Loaded)
        return true;

    TextureMeta* meta = GetMetaAs<TextureMeta>();
    if (!meta)
    {
        LOG_ERROR("Failed to get TextureMeta");
        return false;
    }

    std::vector<uint8_t> data = ReadTextureData(m_Path);
    if (data.empty())
    {
        LOG_ERROR("Failed to read texture data");
        return false;
    }

    m_Texture.GenerateFromMemory(
        data.data(),
        meta->width,
        meta->height,
        meta->format,
        meta->isSRGB
    );

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        LOG_ERROR("OpenGL error while loading texture: {}", err);
        return false;
    }

    if (m_Texture.GetID() == 0)
    {
        LOG_ERROR("Texture ID is 0");
        return false;
    }

    m_Loaded = true;
    return true;
}

void TextureAsset::Unload()
{
    if (!m_Loaded)
        return;

    m_Loaded = false;
}

unsigned int TextureAsset::GetTextureID() const
{
    return m_Texture.GetID();
}

const std::string& TextureAsset::GetPath() const
{
    return m_Path;
}

bool TextureAsset::IsLoaded() const
{
    return m_Loaded;
}

Texture& TextureAsset::GetTexture()
{
    return m_Texture;
}
