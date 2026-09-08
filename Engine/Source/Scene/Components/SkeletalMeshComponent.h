#pragma once
#include "nlohmann/json.hpp"
#include "Asset/Types/SkeletalMeshAsset.h"
#include "Asset/AssetUUID.h"
#include "Core/Ref.h"

namespace rv {

using json = nlohmann::json;

struct SkeletalMeshComponent
{
public:
    SkeletalMeshComponent() = default;
    SkeletalMeshComponent(const AssetUUID& uuid)
    {
        Load(uuid);
    }

    inline Ref<SkeletalMeshAsset> GetMesh() { return mesh; }
    inline AssetUUID GetMeshID() const noexcept { return meshUUID; }

    inline void SetMesh(const AssetUUID& uuid) noexcept
    {
        Load(uuid);
        isDirty = true;
    }

    json Serialize() const;
    void Deserialize(const json& j);

    inline bool IsDirty() const noexcept { return isDirty; }
    inline void SetDirty(bool dirty) noexcept { isDirty = dirty; }

private:
    void Load(const AssetUUID& uuid);

private:
    Ref<SkeletalMeshAsset> mesh;
    AssetUUID meshUUID;
    bool isDirty = false;
};

}