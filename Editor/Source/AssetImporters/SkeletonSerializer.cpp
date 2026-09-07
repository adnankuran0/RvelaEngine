#include "rvelapch.h"
#include "SkeletonSerializer.h"
#include "Core/Log.h"
#include <sstream>

using namespace rv;

namespace {

std::string ToJson(const glm::vec3& v)
{
    std::ostringstream ss;
    ss << v.x << ' ' << v.y << ' ' << v.z;
    return ss.str();
}

std::string ToJson(const glm::quat& q)
{
    std::ostringstream ss;
    ss << q.x << ' ' << q.y << ' ' << q.z << ' ' << q.w;
    return ss.str();
}

std::string ToJson(const glm::mat4& m)
{
    std::ostringstream ss;
    const float* p = &m[0][0];
    for (int i = 0; i < 16; ++i)
    {
        if (i > 0) ss << ' ';
        ss << p[i];
    }
    return ss.str();
}

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

bool SkeletonSerializer::Save(const Ref<SkeletonAsset>& asset, const std::filesystem::path& path)
{
    if (!asset || !asset->IsValid())
    {
        LOG_ERROR("Invalid skeleton: {}", path.string());
        return false;
    }

    json j;
    json bones = json::array();
    for (uint32_t i = 0; i < asset->GetBoneCount(); ++i)
    {
        json b;
        b["name"] = asset->GetBoneName(i);
        b["parent"] = asset->GetParentIndex(i);
        b["bindPosition"] = ToJson(asset->GetBindPosition(i));
        b["bindRotation"] = ToJson(asset->GetBindRotation(i));
        b["bindScale"] = ToJson(asset->GetBindScale(i));
        b["inverseBindMatrix"] = ToJson(asset->GetInverseBindMatrix(i));
        bones.push_back(std::move(b));
    }
    j["bones"] = std::move(bones);

    std::ofstream file(path);
    if (!file)
    {
        LOG_ERROR("Cant open file: {}", path.string());
        return false;
    }

    file << j.dump(2);
    return true;
}

bool SkeletonSerializer::Load(const Ref<SkeletonAsset>& asset, const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file)
    {
        LOG_ERROR("Cant open file: {}", path.string());
        return false;
    }

    json j;
    try
    {
        j = json::parse(file);
    }
    catch (const json::exception&)
    {
        LOG_ERROR("Corrupt JSON in .rskeleton: {}", path.string());
        return false;
    }

    if (!j.contains("bones") || !j["bones"].is_array() || j["bones"].empty())
    {
        LOG_ERROR("Corrupted .rskeleton: {}", path.string());
        return false;
    }

    const auto& bones = j["bones"];

    asset->m_BoneNames.clear();
    asset->m_BoneNameToIndex.clear();
    asset->m_ParentIndices.clear();
    asset->m_BindPositions.clear();
    asset->m_BindRotations.clear();
    asset->m_BindScales.clear();
    asset->m_InverseBindMatrices.clear();

    int32_t index = 0;
    for (const auto& b : bones)
    {
        std::string name = b.value("name", std::string());
        int32_t parent = b.value("parent", -1);

        if (parent >= index)
        {
            LOG_ERROR("Parent index sorting is invalid: {} (bone {})", path.string(), name);
            return false;
        }

        if (!b.contains("bindPosition") || !b.contains("bindRotation") ||
            !b.contains("bindScale") || !b.contains("inverseBindMatrix"))
        {
            LOG_ERROR("Missing bone field(s): {} (bone {})", path.string(), name);
            return false;
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

    return asset->IsValid();
}