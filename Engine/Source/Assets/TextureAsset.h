#pragma once
#include "Renderer/Texture.h"
#include "Core/Ref.h"
#include "AssetUUID.h"
#include <string>
#include "Asset.h"
#include <Core/Log.h>
#include "stb_image/stb_image_write.h"
#include <fstream>

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
    file.close();

    return data;
}



class TextureMeta : public AssetMeta
{
public:
    std::unique_ptr<AssetMeta> Clone() const override 
    {
        return std::make_unique<TextureMeta>(*this);
    }

    void Serialize(std::vector<char>& outBuffer, size_t& offset) const override
    {
        SerializeBase(outBuffer, offset); // UUID, sourcePath, type

        if (outBuffer.size() < offset + sizeof(width) + sizeof(height) + sizeof(mipCount) + sizeof(format) + sizeof(isSRGB) + sizeof(reserved))
            outBuffer.resize(offset + sizeof(width) + sizeof(height) + sizeof(mipCount) + sizeof(format) + sizeof(isSRGB) + sizeof(reserved));

        memcpy(outBuffer.data() + offset, &width, sizeof(width)); offset += sizeof(width);
        memcpy(outBuffer.data() + offset, &height, sizeof(height)); offset += sizeof(height);
        memcpy(outBuffer.data() + offset, &mipCount, sizeof(mipCount)); offset += sizeof(mipCount);
        memcpy(outBuffer.data() + offset, &format, sizeof(format)); offset += sizeof(format);
        memcpy(outBuffer.data() + offset, &isSRGB, sizeof(isSRGB)); offset += sizeof(isSRGB);
        memcpy(outBuffer.data() + offset, reserved, sizeof(reserved)); offset += sizeof(reserved);
    }

    void Deserialize(const std::vector<char>& inBuffer, size_t& offset) override
    {
        DeserializeBase(inBuffer, offset); // UUID, sourcePath, type

        memcpy(&width, inBuffer.data() + offset, sizeof(width)); offset += sizeof(width);
        memcpy(&height, inBuffer.data() + offset, sizeof(height)); offset += sizeof(height);
        memcpy(&mipCount, inBuffer.data() + offset, sizeof(mipCount)); offset += sizeof(mipCount);
        memcpy(&format, inBuffer.data() + offset, sizeof(format)); offset += sizeof(format);
        memcpy(&isSRGB, inBuffer.data() + offset, sizeof(isSRGB)); offset += sizeof(isSRGB);
        memcpy(&reserved, inBuffer.data() + offset, sizeof(reserved));
    }

public:
    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t mipCount = 0;
    TextureFormat format = TextureFormat::Unknown;

    uint8_t isSRGB = 0;
    uint8_t reserved[3] = { 0 };
};

class TextureAsset : public Asset 
{
public:
    TextureAsset(const std::string& path, std::unique_ptr<TextureMeta> textureMeta)
        : m_Path(path), m_Loaded(false), Asset(std::move(textureMeta)) 
    {
        m_Texture.Init();
    }

    ~TextureAsset() 
    {
        Unload();
    }


    inline bool Load()
    {
        if (m_Loaded) return true;

        TextureMeta* meta = GetMetaAs<TextureMeta>();
        if (!meta)
        {
            LOG_ERROR("Failed to get TextureMeta");
            return false;
        }


        std::vector<uint8_t> data = ReadTextureData(m_Path);
        //stbi_write_png("debug_readed.png", meta->width, meta->height, 4, data.data(), meta->width * 4);
        if (data.empty())
        {
            LOG_ERROR("Failed to read texture data from file");
            return false;
        }
        
        m_Texture.GenerateFromMemory(data.data(), meta->width, meta->height, meta->format, meta->isSRGB);
        //m_Texture.GenerateFromImage(std::filesystem::path(m_Path).replace_extension(".png").string());
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) LOG_ERROR("Error while loading texture from memory: {}", err);

        if (m_Texture.GetID() == 0)
        {
            LOG_ERROR("Texture OpenGL ID is 0, loading failed");
            return false;
        }

        m_Loaded = true;
        return true;
    }

    inline void Unload() 
    {
        if (m_Loaded) 
        {
            m_Loaded = false;
        }
    }

    inline unsigned int GetTextureID() const 
    {
        return m_Texture.GetID();
    }

    inline const std::string& GetPath() const 
    {
        return m_Path;
    }

    inline bool IsLoaded() const 
    {
        return m_Loaded;
    }

    inline Texture& GetTexture() 
    {
        return m_Texture;
    }

private:
    std::string m_Path;
    Texture m_Texture;
    bool m_Loaded;
};

