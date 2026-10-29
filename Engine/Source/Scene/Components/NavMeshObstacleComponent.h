#pragma once

#include <nlohmann/json.hpp>

namespace rv {

struct NavMeshObstacleComponent
{
public:
    bool affectNavMesh = true;
    bool isWalkable = false;

    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);
};

} // namespace rv
