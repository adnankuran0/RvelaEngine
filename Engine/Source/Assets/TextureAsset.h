#pragma once

#include "Asset.h"
#include <memory>
#include <string>
#include <Rendering/Texture.h>

namespace rv {

class TextureMeta : public AssetMeta
{
public:
    std::unique_ptr<AssetMeta> Clone() const override;
    void Serialize(std::vector<char>& outBuffer, size_t& offset) const override;
    void Deserialize(const std::vector<char>& inBuffer, size_t& offset) override;

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
    TextureAsset(const std::string& path, std::unique_ptr<TextureMeta> textureMeta);
    ~TextureAsset();

    bool Load();
    void Unload();

    unsigned int GetTextureID() const;
    const std::string& GetPath() const;
    bool IsLoaded() const;
    Texture& GetTexture();

private:
    std::string m_Path;
    Texture m_Texture;
    bool m_Loaded = false;
};

}