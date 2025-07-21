#pragma once
#include "Renderer/Texture.h"
#include "Core/Ref.h"
#include "AssetUUID.h"
#include <string>
#include "Asset.h"



class TextureAsset : public Asset {
public:
    TextureAsset(const std::string& path) : m_Path(path) , m_Loaded(false) { }

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

    unsigned int GetID() const {
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
