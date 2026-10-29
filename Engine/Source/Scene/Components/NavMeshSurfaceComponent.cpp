#include "rvelapch.h"
#include "NavMeshSurfaceComponent.h"
#include "NavMeshObstacleComponent.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TagComponent.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/ColliderComponents.h"
#include "Asset/Types/MeshAsset.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

namespace rv {

NavMeshSurfaceComponent::NavMeshSurfaceComponent()
    : m_NavMesh(nullptr)
    , m_NavQuery(nullptr)
    , m_IsBaked(false)
{
}

NavMeshSurfaceComponent::~NavMeshSurfaceComponent()
{
    Cleanup();
}

NavMeshSurfaceComponent::NavMeshSurfaceComponent(const NavMeshSurfaceComponent& other)
    : config(other.config)
    , autoBakeOnPlay(other.autoBakeOnPlay)
    , drawDebugMesh(other.drawDebugMesh)
    , debugColor(other.debugColor)
    , m_NavMesh(nullptr)
    , m_NavQuery(nullptr)
    , m_IsBaked(false)
{
}

NavMeshSurfaceComponent& NavMeshSurfaceComponent::operator=(const NavMeshSurfaceComponent& other)
{
    if (this != &other)
    {
        Cleanup();
        config = other.config;
        autoBakeOnPlay = other.autoBakeOnPlay;
        drawDebugMesh = other.drawDebugMesh;
        debugColor = other.debugColor;
        m_IsBaked = false;
    }
    return *this;
}

NavMeshSurfaceComponent::NavMeshSurfaceComponent(NavMeshSurfaceComponent&& other) noexcept
    : config(other.config)
    , autoBakeOnPlay(other.autoBakeOnPlay)
    , drawDebugMesh(other.drawDebugMesh)
    , debugColor(other.debugColor)
    , m_NavMesh(other.m_NavMesh)
    , m_NavQuery(other.m_NavQuery)
    , m_IsBaked(other.m_IsBaked)
{
    other.m_NavMesh = nullptr;
    other.m_NavQuery = nullptr;
    other.m_IsBaked = false;
}

NavMeshSurfaceComponent& NavMeshSurfaceComponent::operator=(NavMeshSurfaceComponent&& other) noexcept
{
    if (this != &other)
    {
        Cleanup();
        config = other.config;
        autoBakeOnPlay = other.autoBakeOnPlay;
        drawDebugMesh = other.drawDebugMesh;
        debugColor = other.debugColor;
        m_NavMesh = other.m_NavMesh;
        m_NavQuery = other.m_NavQuery;
        m_IsBaked = other.m_IsBaked;

        other.m_NavMesh = nullptr;
        other.m_NavQuery = nullptr;
        other.m_IsBaked = false;
    }
    return *this;
}

void NavMeshSurfaceComponent::InitQuery()
{
    if (m_NavQuery)
    {
        dtFreeNavMeshQuery(m_NavQuery);
        m_NavQuery = nullptr;
    }

    if (m_NavMesh)
    {
        m_NavQuery = dtAllocNavMeshQuery();
        if (m_NavQuery)
        {
            dtStatus status = m_NavQuery->init(m_NavMesh, 2048);
            if (dtStatusFailed(status))
            {
                LOG_ERROR("[NavMesh] Failed to initialize DetourNavMeshQuery (status 0x{:x})", status);
                dtFreeNavMeshQuery(m_NavQuery);
                m_NavQuery = nullptr;
            }
        }
    }
}

void NavMeshSurfaceComponent::Cleanup()
{
    if (m_NavQuery)
    {
        dtFreeNavMeshQuery(m_NavQuery);
        m_NavQuery = nullptr;
    }
    if (m_NavMesh)
    {
        NavMeshBuilder::FreeNavMesh(m_NavMesh);
        m_NavMesh = nullptr;
    }
    m_IsBaked = false;
}

void NavMeshSurfaceComponent::Clear()
{
    Cleanup();
    LOG_INFO("[NavMesh] Cleared NavMesh.");
}

bool NavMeshSurfaceComponent::Bake(Scene& scene)
{
    Cleanup();

    std::vector<float> worldVertices;
    std::vector<int> worldIndices;
    std::vector<unsigned char> triangleAreas;

    auto meshView = scene.GetRegistry().view<TransformComponent, MeshComponent>();
    for (auto entity : meshView)
    {
        if (scene.HasComponent<TagComponent>(entity) && !scene.GetComponent<TagComponent>(entity).isActive)
            continue;

        bool isObstacle = false;
        if (scene.HasComponent<NavMeshObstacleComponent>(entity))
        {
            const auto& obs = scene.GetComponent<NavMeshObstacleComponent>(entity);
            if (!obs.affectNavMesh)
                continue;
            if (!obs.isWalkable)
                isObstacle = true;
        }

        if (scene.HasComponent<RigidbodyComponent>(entity))
        {
            const auto& rb = scene.GetComponent<RigidbodyComponent>(entity);
            if (rb.bodyType != Physics::MotionType::STATIC)
                continue;
        }

        auto& transform = scene.GetComponent<TransformComponent>(entity);
        auto& meshComp = scene.GetComponent<MeshComponent>(entity);
        auto meshAsset = meshComp.GetMesh();
        if (!meshAsset || !meshAsset->IsValid())
            continue;

        const glm::mat4 worldMatrix = transform.GetWorldMatrix();
        const auto verts = meshAsset->GetVertices();
        const auto indices = meshAsset->GetIndices();

        int baseVertexIndex = static_cast<int>(worldVertices.size() / 3);

        for (const auto& v : verts)
        {
            glm::vec4 worldPos = worldMatrix * glm::vec4(v.position, 1.0f);
            worldVertices.push_back(worldPos.x);
            worldVertices.push_back(worldPos.y);
            worldVertices.push_back(worldPos.z);
        }

        const unsigned char triArea = isObstacle ? RC_NULL_AREA : RC_WALKABLE_AREA;

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            worldIndices.push_back(baseVertexIndex + static_cast<int>(indices[i]));
            worldIndices.push_back(baseVertexIndex + static_cast<int>(indices[i + 1]));
            worldIndices.push_back(baseVertexIndex + static_cast<int>(indices[i + 2]));
            triangleAreas.push_back(triArea);
        }
    }

    // Support NavMeshObstacleComponent on entities with BoxColliderComponent but no visual MeshComponent
    auto obsBoxView = scene.GetRegistry().view<TransformComponent, BoxColliderComponent, NavMeshObstacleComponent>(entt::exclude<MeshComponent>);
    for (auto entity : obsBoxView)
    {
        if (scene.HasComponent<TagComponent>(entity) && !scene.GetComponent<TagComponent>(entity).isActive)
            continue;

        const auto& obs = scene.GetComponent<NavMeshObstacleComponent>(entity);
        if (!obs.affectNavMesh)
            continue;

        auto& transform = scene.GetComponent<TransformComponent>(entity);
        auto& box = scene.GetComponent<BoxColliderComponent>(entity);

        glm::vec3 halfSize = box.GetSize();
        glm::vec3 offset = box.GetOffset();
        glm::mat4 worldMatrix = transform.GetWorldMatrix();

        static const glm::vec3 unitCorners[8] = {
            { -1.0f, -1.0f, -1.0f },
            {  1.0f, -1.0f, -1.0f },
            {  1.0f,  1.0f, -1.0f },
            { -1.0f,  1.0f, -1.0f },
            { -1.0f, -1.0f,  1.0f },
            {  1.0f, -1.0f,  1.0f },
            {  1.0f,  1.0f,  1.0f },
            { -1.0f,  1.0f,  1.0f }
        };

        static const int boxIndices[36] = {
            0, 2, 1,  0, 3, 2,
            4, 5, 6,  4, 6, 7,
            0, 1, 5,  0, 5, 4,
            2, 3, 7,  2, 7, 6,
            0, 4, 7,  0, 7, 3,
            1, 2, 6,  1, 6, 5
        };

        int baseVertexIndex = static_cast<int>(worldVertices.size() / 3);
        for (int i = 0; i < 8; ++i)
        {
            glm::vec3 localPos = offset + unitCorners[i] * halfSize;
            glm::vec4 worldPos = worldMatrix * glm::vec4(localPos, 1.0f);
            worldVertices.push_back(worldPos.x);
            worldVertices.push_back(worldPos.y);
            worldVertices.push_back(worldPos.z);
        }

        const unsigned char triArea = obs.isWalkable ? RC_WALKABLE_AREA : RC_NULL_AREA;
        for (int i = 0; i < 36; i += 3)
        {
            worldIndices.push_back(baseVertexIndex + boxIndices[i]);
            worldIndices.push_back(baseVertexIndex + boxIndices[i + 1]);
            worldIndices.push_back(baseVertexIndex + boxIndices[i + 2]);
            triangleAreas.push_back(triArea);
        }
    }

    if (worldVertices.empty() || worldIndices.empty())
    {
        LOG_WARN("[NavMesh] No static mesh geometry found in scene to bake NavMesh.");
        return false;
    }

    if (!NavMeshBuilder::BuildSoloNavMesh(worldVertices, worldIndices, config, m_NavMesh, triangleAreas))
    {
        LOG_ERROR("[NavMesh] Bake failed.");
        return false;
    }

    InitQuery();
    m_IsBaked = (m_NavMesh != nullptr && m_NavQuery != nullptr);
    return m_IsBaked;
}

std::vector<glm::vec3> NavMeshSurfaceComponent::FindPath(const glm::vec3& start, const glm::vec3& end) const
{
    std::vector<glm::vec3> path;
    if (!m_NavQuery || !m_NavMesh || !m_IsBaked)
        return path;

    dtQueryFilter filter;
    filter.setIncludeFlags(0xffff);
    filter.setExcludeFlags(0);

    float extents[3] = { 2.0f, 4.0f, 2.0f };
    float spos[3] = { start.x, start.y, start.z };
    float epos[3] = { end.x, end.y, end.z };

    dtPolyRef startRef = 0;
    dtPolyRef endRef = 0;
    float nearestStart[3]{};
    float nearestEnd[3]{};

    m_NavQuery->findNearestPoly(spos, extents, &filter, &startRef, nearestStart);
    m_NavQuery->findNearestPoly(epos, extents, &filter, &endRef, nearestEnd);

    if (!startRef || !endRef)
        return path;

    static const int MAX_POLYS = 256;
    dtPolyRef polys[MAX_POLYS];
    int polyCount = 0;
    m_NavQuery->findPath(startRef, endRef, nearestStart, nearestEnd, &filter, polys, &polyCount, MAX_POLYS);

    if (polyCount > 0)
    {
        static const int MAX_STRAIGHT_PATH = 256;
        float straightPath[MAX_STRAIGHT_PATH * 3];
        unsigned char straightPathFlags[MAX_STRAIGHT_PATH];
        dtPolyRef straightPathPolys[MAX_STRAIGHT_PATH];
        int straightPathCount = 0;

        m_NavQuery->findStraightPath(
            nearestStart, nearestEnd, polys, polyCount,
            straightPath, straightPathFlags, straightPathPolys, &straightPathCount,
            MAX_STRAIGHT_PATH, 0);

        path.reserve(straightPathCount);
        for (int i = 0; i < straightPathCount; ++i)
        {
            path.push_back(glm::vec3(straightPath[i * 3], straightPath[i * 3 + 1], straightPath[i * 3 + 2]));
        }
    }

    return path;
}

bool NavMeshSurfaceComponent::SamplePosition(const glm::vec3& pos, glm::vec3& outPos, const glm::vec3& extents) const
{
    if (!m_NavQuery || !m_NavMesh || !m_IsBaked)
        return false;

    dtQueryFilter filter;
    filter.setIncludeFlags(0xffff);
    filter.setExcludeFlags(0);

    float p[3] = { pos.x, pos.y, pos.z };
    float ext[3] = { extents.x, extents.y, extents.z };
    float nearest[3]{};
    dtPolyRef polyRef = 0;

    dtStatus status = m_NavQuery->findNearestPoly(p, ext, &filter, &polyRef, nearest);
    if (dtStatusSucceed(status) && polyRef != 0)
    {
        outPos = glm::vec3(nearest[0], nearest[1], nearest[2]);
        return true;
    }
    return false;
}

bool NavMeshSurfaceComponent::Raycast(const glm::vec3& start, const glm::vec3& end, glm::vec3& outHit) const
{
    if (!m_NavQuery || !m_NavMesh || !m_IsBaked)
        return false;

    dtQueryFilter filter;
    filter.setIncludeFlags(0xffff);
    filter.setExcludeFlags(0);

    float extents[3] = { 2.0f, 4.0f, 2.0f };
    float spos[3] = { start.x, start.y, start.z };
    float epos[3] = { end.x, end.y, end.z };

    dtPolyRef startRef = 0;
    float nearestStart[3]{};
    m_NavQuery->findNearestPoly(spos, extents, &filter, &startRef, nearestStart);
    if (!startRef)
        return false;

    float t = 0.0f;
    float hitNormal[3]{};
    static const int MAX_POLYS = 128;
    dtPolyRef polys[MAX_POLYS];
    int polyCount = 0;

    dtStatus status = m_NavQuery->raycast(startRef, spos, epos, &filter, &t, hitNormal, polys, &polyCount, MAX_POLYS);
    if (dtStatusSucceed(status))
    {
        if (t <= 1.0f)
        {
            outHit = start + (end - start) * t;
            return true;
        }
    }
    return false;
}

std::vector<std::pair<glm::vec3, glm::vec3>> NavMeshSurfaceComponent::GetDebugLines() const
{
    std::vector<std::pair<glm::vec3, glm::vec3>> lines;
    if (!m_NavMesh || !m_IsBaked)
        return lines;

    const dtNavMesh* navMesh = m_NavMesh;
    for (int i = 0; i < navMesh->getMaxTiles(); ++i)
    {
        const dtMeshTile* tile = navMesh->getTile(i);
        if (!tile || !tile->header)
            continue;

        for (int p = 0; p < tile->header->polyCount; ++p)
        {
            const dtPoly* poly = &tile->polys[p];
            if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
                continue;

            for (int v = 0; v < poly->vertCount; ++v)
            {
                int nextV = (v + 1) % poly->vertCount;
                const float* v0 = &tile->verts[poly->verts[v] * 3];
                const float* v1 = &tile->verts[poly->verts[nextV] * 3];

                lines.push_back({
                    glm::vec3(v0[0], v0[1] + 0.05f, v0[2]),
                    glm::vec3(v1[0], v1[1] + 0.05f, v1[2])
                });
            }
        }
    }

    return lines;
}

nlohmann::json NavMeshSurfaceComponent::Serialize() const
{
    nlohmann::json j;
    j["cellSize"] = config.cellSize;
    j["cellHeight"] = config.cellHeight;
    j["agentHeight"] = config.agentHeight;
    j["agentRadius"] = config.agentRadius;
    j["agentMaxClimb"] = config.agentMaxClimb;
    j["agentMaxSlope"] = config.agentMaxSlope;
    j["regionMinSize"] = config.regionMinSize;
    j["regionMergeSize"] = config.regionMergeSize;
    j["edgeMaxLen"] = config.edgeMaxLen;
    j["edgeMaxError"] = config.edgeMaxError;
    j["vertsPerPoly"] = config.vertsPerPoly;
    j["detailSampleDist"] = config.detailSampleDist;
    j["detailSampleMaxError"] = config.detailSampleMaxError;
    j["filterLowHangingObstacles"] = config.filterLowHangingObstacles;

    j["autoBakeOnPlay"] = autoBakeOnPlay;
    j["drawDebugMesh"] = drawDebugMesh;
    j["debugColor"] = { debugColor.r, debugColor.g, debugColor.b, debugColor.a };
    return j;
}

void NavMeshSurfaceComponent::Deserialize(const nlohmann::json& j)
{
    config.cellSize = j.value("cellSize", 0.3f);
    config.cellHeight = j.value("cellHeight", 0.2f);
    config.agentHeight = j.value("agentHeight", 2.0f);
    config.agentRadius = j.value("agentRadius", 0.5f);
    config.agentMaxClimb = j.value("agentMaxClimb", 0.25f);
    config.agentMaxSlope = j.value("agentMaxSlope", 45.0f);
    config.regionMinSize = j.value("regionMinSize", 8.0f);
    config.regionMergeSize = j.value("regionMergeSize", 20.0f);
    config.edgeMaxLen = j.value("edgeMaxLen", 12.0f);
    config.edgeMaxError = j.value("edgeMaxError", 1.3f);
    config.vertsPerPoly = j.value("vertsPerPoly", 6.0f);
    config.detailSampleDist = j.value("detailSampleDist", 6.0f);
    config.detailSampleMaxError = j.value("detailSampleMaxError", 1.0f);
    config.filterLowHangingObstacles = j.value("filterLowHangingObstacles", false);

    autoBakeOnPlay = j.value("autoBakeOnPlay", true);
    drawDebugMesh = j.value("drawDebugMesh", true);

    if (j.contains("debugColor") && j["debugColor"].is_array() && j["debugColor"].size() == 4)
    {
        debugColor = glm::vec4(
            j["debugColor"][0].get<float>(),
            j["debugColor"][1].get<float>(),
            j["debugColor"][2].get<float>(),
            j["debugColor"][3].get<float>()
        );
    }
}

} // namespace rv