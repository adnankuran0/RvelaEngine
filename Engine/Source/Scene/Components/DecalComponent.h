#pragma once
#include "Asset/AssetUUID.h"
#include "Asset/Types/TextureAsset.h"
#include "Asset/AssetManager.h"
#include "glm/glm.hpp"
#include "json.hpp"

namespace rv {

using json = nlohmann::json;

struct DecalComponent
{
    AssetUUID textureUUID = AssetUUID::Invalid();
    glm::vec4 color = glm::vec4(1.0f);
    float opacity = 1.0f;
    float angleCutoff = 60.0f;
    int renderOrder = 0;

    bool lit = true;
    bool receiveShadows = true;

    float upperFade = 0.0f;
    float lowerFade = 0.0f;

    bool distanceFade = false;
    float fadeStart = 40.0f;
    float fadeEnd = 50.0f;

    Ref<TextureAsset> GetTexture() const
    {
        if (textureUUID.IsValid())
            return AssetManager::Get().GetAsset<TextureAsset>(textureUUID);
        return nullptr;
    }

    void SetTexture(const AssetUUID& uuid) { textureUUID = uuid; }
    void ClearTexture() { textureUUID = AssetUUID::Invalid(); }

    json Serialize() const;
    void Deserialize(const json& j);
};

}
