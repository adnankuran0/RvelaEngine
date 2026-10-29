#pragma once

#include <vector>
#include <utility>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include "Navigation/NavMeshBuilder.h"

class dtNavMesh;
class dtNavMeshQuery;

namespace rv {

class Scene;

struct NavMeshSurfaceComponent
{
public:
    NavMeshBuildConfig config;

    bool autoBakeOnPlay = true;
    bool drawDebugMesh = true;
    glm::vec4 debugColor{ 0.1f, 0.85f, 0.2f, 0.7f };

    NavMeshSurfaceComponent();
    ~NavMeshSurfaceComponent();

    NavMeshSurfaceComponent(const NavMeshSurfaceComponent& other);
    NavMeshSurfaceComponent& operator=(const NavMeshSurfaceComponent& other);
    NavMeshSurfaceComponent(NavMeshSurfaceComponent&& other) noexcept;
    NavMeshSurfaceComponent& operator=(NavMeshSurfaceComponent&& other) noexcept;

    bool Bake(Scene& scene);
    void Clear();

    bool IsValid() const { return m_NavMesh != nullptr && m_NavQuery != nullptr && m_IsBaked; }
    bool IsBaked() const { return m_IsBaked; }

    std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& end) const;
    bool SamplePosition(const glm::vec3& pos, glm::vec3& outPos, const glm::vec3& extents = glm::vec3(2.0f, 4.0f, 2.0f)) const;
    bool Raycast(const glm::vec3& start, const glm::vec3& end, glm::vec3& outHit) const;

    std::vector<std::pair<glm::vec3, glm::vec3>> GetDebugLines() const;

    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);

    dtNavMesh* GetNavMesh() const { return m_NavMesh; }
    dtNavMeshQuery* GetNavQuery() const { return m_NavQuery; }

private:
    void InitQuery();
    void Cleanup();

    dtNavMesh* m_NavMesh = nullptr;
    dtNavMeshQuery* m_NavQuery = nullptr;
    bool m_IsBaked = false;
};

} // namespace rv
