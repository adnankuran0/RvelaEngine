#pragma once
#include "nlohmann/json.hpp"
#include "Asset/Types/SkeletonAsset.h"
#include "Asset/AssetUUID.h"
#include "Core/Ref.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace rv {

using json = nlohmann::json;

struct SkeletonComponent
{
public:
    SkeletonComponent() = default;
    SkeletonComponent(const AssetUUID& uuid)
    {
        Load(uuid);
    }

    inline Ref<SkeletonAsset> GetSkeleton() { return skeleton; }
    inline AssetUUID GetSkeletonID() const noexcept { return skeletonUUID; }

    inline void SetSkeleton(const AssetUUID& uuid) noexcept
    {
        Load(uuid);
    }

    json Serialize() const;
    void Deserialize(const json& j);

    // runtime
    std::vector<glm::vec3> localPositions;
    std::vector<glm::quat> localRotations;
    std::vector<glm::vec3> localScales;

    std::vector<glm::mat4> modelSpaceMatrices;
    std::vector<glm::mat4> skinningPalette;

    bool isInitialized = false;

    void InitFromAsset();

private:
    void Load(const AssetUUID& uuid);

private:
    Ref<SkeletonAsset> skeleton;
    AssetUUID skeletonUUID;
};

}