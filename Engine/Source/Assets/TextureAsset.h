#pragma once
#include "Renderer/Texture.h"
#include "Core/Ref.h"
#include "AssetUUID.h"
#include <string>
#include "Asset.h"

enum class TextureFormat : uint8_t
{
    Unknown = 0,
    RGBA8,
    RGB8,
    BC1, //DXT1
    BC3  //DXT5
};

class TextureMeta : public AssetMeta
{
public:
    std::unique_ptr<AssetMeta> Clone() const override {
        return std::make_unique<TextureMeta>(*this);
    }

    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t mipCount = 0;
    TextureFormat format = TextureFormat::Unknown;

    uint8_t isSRGB = 0;
    uint8_t reserved[3] = { 0 };
};

class TextureAsset : public Asset {
public:
    TextureAsset(const std::string& path, std::unique_ptr<TextureMeta> textureMeta) : m_Path(path) , m_Loaded(false) , Asset(std::move(textureMeta)) { }

    ~TextureAsset() {
        Unload();
    }

    bool Load() {
        if (m_Loaded) return true;
        m_Texture = Texture();
        m_Texture.GenerateFromImage(m_Path);
        m_Loaded = true;
        return m_Loaded;
    }

    void Unload() {
        if (m_Loaded) {
            m_Texture.Destroy();
            m_Loaded = false;
        }
    }

    unsigned int GetTextureID() const {
        return m_Texture.GetID();
    }

    const std::string& GetPath() const {
        return m_Path;
    }

    bool IsLoaded() const {
        return m_Loaded;
    }

    Texture& GetTexture() {
        return m_Texture;
    }

private:
    std::string m_Path;
    Texture m_Texture;
    bool m_Loaded;
};
