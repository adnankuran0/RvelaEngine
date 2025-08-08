#pragma once
#include "Scene/Component.h"
#include "Utils/FileUtils.h"
#include "nlohmann/json.hpp"
#include "Assets/MeshAsset.h"
#include "Assets/AssetRegistry.h"

using json = nlohmann::json;

class MeshComponent : public Component
{
public:
    MeshComponent() = default;
    MeshComponent(const AssetUUID& uuid)
    {
        Load(uuid);
    }

    inline Ref<MeshAsset> GetMesh() { return mesh; }
    inline void SetMesh(const AssetUUID& uuid)
    {
        Load(uuid);
        isDirty = true;
    }

    std::string Serialize() const override 
{
        json j;
        j["mesh"] = meshUUID.ToString();
        return j.dump(4);
    }

    void Deserialize(const std::string& str) override 
    {
        json j = json::parse(str);
        meshUUID = AssetUUID::FromString(j["mesh"]);
        Load(meshUUID);
    }

    inline bool IsDirty() { return isDirty; }
    inline void SetDirty(bool isDirty) { this->isDirty = isDirty; }

private:
    inline void Load(const AssetUUID& uuid)
    {
        if (!uuid.IsValid())
        {
            LOG_WARN("Material UUID is not valid!");
            return;
        }
        meshUUID = uuid;
        mesh.Reset();
        mesh = AssetRegistry::GetAsset<MeshAsset>(meshUUID);
    }

    Ref<MeshAsset> mesh;
    AssetUUID meshUUID;

    bool isDirty = false;
};

