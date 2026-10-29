#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace rv {

struct NavMeshAgentComponent
{
public:
    float speed = 3.5f;
    float angularSpeed = 360.0f; // dps
    float acceleration = 8.0f;
    float stoppingDistance = 0.5f;
    float radius = 0.5f;
    float height = 2.0f;

    // Behavior flags
    bool autoRepath = true;
    float repathInterval = 0.5f;
    bool updateRotation = true;
    bool updatePosition = true;
    bool drawDebugPath = true;

    // runtime
    glm::vec3 destination{ 0.0f };
    glm::vec3 velocity{ 0.0f };
    std::vector<glm::vec3> waypoints;
    size_t currentWaypointIndex = 0;
    bool isMoving = false;
    bool hasPath = false;
    bool isStopped = false;
    float repathTimer = 0.0f;
    bool requestNewPath = false;

    NavMeshAgentComponent() = default;

    void SetDestination(const glm::vec3& target);
    void Stop();
    void Resume();
    void ResetPath();

    float RemainingDistance() const;
    bool HasReachedDestination() const;

    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);
};

} // namespace rv
