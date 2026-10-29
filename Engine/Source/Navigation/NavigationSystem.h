#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <entt/entt.h>

namespace rv {

class Scene;
struct NavMeshSurfaceComponent;

class NavigationSystem
{
public:
    explicit NavigationSystem(Scene& scene);
    ~NavigationSystem() = default;

    void OnStart();
    void OnStop();
    void Update(float dt);

    NavMeshSurfaceComponent* GetActiveSurface();

    std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& end);
    bool SamplePosition(const glm::vec3& pos, glm::vec3& outPos, const glm::vec3& extents = glm::vec3(2.0f, 4.0f, 2.0f));
    bool Raycast(const glm::vec3& start, const glm::vec3& end, glm::vec3& outHit);

private:
    Scene& m_Scene;
};

} // namespace rv
