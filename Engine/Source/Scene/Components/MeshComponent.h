#pragma once
#include "Utils/FileUtils.h"
#include "nlohmann/json.hpp"
#include "Assets/MeshAsset.h"
#include "Assets/AssetRegistry.h"

using json = nlohmann::json;

struct MeshComponent 
{
public:
    MeshComponent() = default;
    MeshComponent(const AssetUUID& uuid)
    {
        Load(uuid);
    }

    inline Ref<MeshAsset> GetMesh() { return mesh; }
    inline AssetUUID GetMeshID() const noexcept { return meshUUID; }


    inline void SetMesh(const AssetUUID& uuid) noexcept
    {
        Load(uuid);
        isDirty = true;
    }

    std::string Serialize() const;
    void Deserialize(const std::string& str);

    inline bool IsDirty() noexcept { return isDirty;}
    inline void SetDirty(bool isDirty) noexcept { this->isDirty = isDirty; }

private:
    void Load(const AssetUUID& uuid);

private:
    Ref<MeshAsset> mesh;
    AssetUUID meshUUID;
    bool isDirty = false;
};

