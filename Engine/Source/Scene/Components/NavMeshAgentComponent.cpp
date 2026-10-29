#include "rvelapch.h"
#include "NavMeshAgentComponent.h"

namespace rv {

void NavMeshAgentComponent::SetDestination(const glm::vec3& target)
{
    destination = target;
    isStopped = false;
    requestNewPath = true;
    repathTimer = 0.0f;
}

void NavMeshAgentComponent::Stop()
{
    isStopped = true;
    velocity = glm::vec3(0.0f);
}

void NavMeshAgentComponent::Resume()
{
    isStopped = false;
}

void NavMeshAgentComponent::ResetPath()
{
    waypoints.clear();
    currentWaypointIndex = 0;
    hasPath = false;
    isMoving = false;
    velocity = glm::vec3(0.0f);
    requestNewPath = false;
}

float NavMeshAgentComponent::RemainingDistance() const
{
    if (!hasPath || currentWaypointIndex >= waypoints.size())
        return 0.0f;

    float dist = 0.0f;
    for (size_t i = currentWaypointIndex; i + 1 < waypoints.size(); ++i)
    {
        dist += glm::distance(waypoints[i], waypoints[i + 1]);
    }
    return dist;
}

bool NavMeshAgentComponent::HasReachedDestination() const
{
    return hasPath && !isMoving && !requestNewPath;
}

nlohmann::json NavMeshAgentComponent::Serialize() const
{
    nlohmann::json j;
    j["speed"] = speed;
    j["angularSpeed"] = angularSpeed;
    j["acceleration"] = acceleration;
    j["stoppingDistance"] = stoppingDistance;
    j["radius"] = radius;
    j["height"] = height;
    j["autoRepath"] = autoRepath;
    j["repathInterval"] = repathInterval;
    j["updateRotation"] = updateRotation;
    j["updatePosition"] = updatePosition;
    j["drawDebugPath"] = drawDebugPath;
    return j;
}

void NavMeshAgentComponent::Deserialize(const nlohmann::json& j)
{
    speed = j.value("speed", 3.5f);
    angularSpeed = j.value("angularSpeed", 360.0f);
    acceleration = j.value("acceleration", 8.0f);
    stoppingDistance = j.value("stoppingDistance", 0.5f);
    radius = j.value("radius", 0.5f);
    height = j.value("height", 2.0f);
    autoRepath = j.value("autoRepath", true);
    repathInterval = j.value("repathInterval", 0.5f);
    updateRotation = j.value("updateRotation", true);
    updatePosition = j.value("updatePosition", true);
    drawDebugPath = j.value("drawDebugPath", true);
}

} // namespace rv
