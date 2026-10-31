#include "rvelapch.h"
#include "SkeletonLoader.h"
#include "Asset/Types/SkeletonAsset.h"
#include "Core/Log.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include "Asset/AssetMeta.h"

using namespace rv;
using json = nlohmann::json;

namespace {

glm::vec3 Vec3FromJson(const std::string& s)
{
    std::istringstream ss(s);
    glm::vec3 v;
    ss >> v.x >> v.y >> v.z;
    return v;
}

glm::quat QuatFromJson(const std::string& s)
{
    std::istringstream ss(s);
    float x, y, z, w;
    ss >> x >> y >> z >> w;
    return glm::quat(w, x, y, z);
}

glm::mat4 Mat4FromJson(const std::string& s)
{
    std::istringstream ss(s);
    glm::mat4 m;
    float* p = &m[0][0];
    for (int i = 0; i < 16; ++i)
        ss >> p[i];
    return m;
}

}

Ref<Asset> SkeletonLoader::ParseSkeleton(const json& j, const AssetMeta& meta, const std::string& debugPath)
{
    if (!j.contains("bones") || !j["bones"].is_array() || j["bones"].empty())
    {
        LOG_ERROR("Corrupted .rskeleton (empty bones): {}", debugPath);
        return nullptr;
    }

    auto asset = CreateRef<SkeletonAsset>(meta.uuid);
    const auto& bones = j["bones"];

    asset->m_BoneNames.reserve(bones.size());
    asset->m_ParentIndices.reserve(bones.size());
    asset->m_BindPositions.reserve(bones.size());
    asset->m_BindRotations.reserve(bones.size());
    asset->m_BindScales.reserve(bones.size());
    asset->m_InverseBindMatrices.reserve(bones.size());

    int32_t index = 0;
    for (const auto& b : bones)
    {
        std::string name = b.value("name", std::string());
        int32_t parent = b.value("parent", -1);

        if (parent >= index)
        {
            LOG_ERROR("Corrupted skeleton asset, parent index sorting is invalid: {} (bone {})",
                debugPath, name);
            return nullptr;
        }

        if (!b.contains("bindPosition") || !b.contains("bindRotation") ||
            !b.contains("bindScale") || !b.contains("inverseBindMatrix"))
        {
            LOG_ERROR("Corrupted skeleton asset, missing bone field(s): {} (bone {})",
                debugPath, name);
            return nullptr;
        }

        asset->m_BoneNames.push_back(name);
        asset->m_BoneNameToIndex[name] = index;
        asset->m_ParentIndices.push_back(parent);
        asset->m_BindPositions.push_back(Vec3FromJson(b["bindPosition"].get<std::string>()));
        asset->m_BindRotations.push_back(QuatFromJson(b["bindRotation"].get<std::string>()));
        asset->m_BindScales.push_back(Vec3FromJson(b["bindScale"].get<std::string>()));
        asset->m_InverseBindMatrices.push_back(Mat4FromJson(b["inverseBindMatrix"].get<std::string>()));
        ++index;
    }

    if (!asset->IsValid())
        return nullptr;

    return asset;
}

Ref<Asset> SkeletonLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    std::ifstream file(assetPath);
    if (!file)
    {
        LOG_ERROR("Cannot open: {}", assetPath.string());
        return nullptr;
    }

    json j;
    try
    {
        j = json::parse(file);
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("JSON error in {}: {}", assetPath.string(), e.what());
        return nullptr;
    }

    return ParseSkeleton(j, meta, assetPath.string());
}

Ref<Asset> SkeletonLoader::LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta)
{
    if (!data || size == 0)
    {
        LOG_ERROR("SkeletonLoader::LoadFromMemory: empty data for UUID: {}", meta.uuid.ToString());
        return nullptr;
    }

    json j;
    try
    {
        j = json::parse(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data + size));
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("JSON error in skeleton memory buffer {}: {}", meta.uuid.ToString(), e.what());
        return nullptr;
    }

    return ParseSkeleton(j, meta, meta.uuid.ToString());
}
