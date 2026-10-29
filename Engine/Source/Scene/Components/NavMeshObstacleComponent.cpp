#include "rvelapch.h"
#include "NavMeshObstacleComponent.h"

namespace rv {

nlohmann::json NavMeshObstacleComponent::Serialize() const
{
    nlohmann::json j;
    j["affectNavMesh"] = affectNavMesh;
    j["isWalkable"] = isWalkable;
    return j;
}

void NavMeshObstacleComponent::Deserialize(const nlohmann::json& j)
{
    affectNavMesh = j.value("affectNavMesh", true);
    isWalkable = j.value("isWalkable", false);
}

} // namespace rv
