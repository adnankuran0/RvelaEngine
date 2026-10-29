#include "rvelapch.h"
#include "NavigationSystem.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components/NavMeshSurfaceComponent.h"
#include "Scene/Components/NavMeshAgentComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/CharacterBodyComponent.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/TagComponent.h"
#include "Physics/PhysicsSystem.h"
#include "Physics/PhysicsWorld.h"
#include "Renderer/DebugRenderer.h"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

namespace rv {

NavigationSystem::NavigationSystem(Scene& scene)
    : m_Scene(scene)
{
}

void NavigationSystem::OnStart()
{
    auto surface = GetActiveSurface();
    if (surface && surface->autoBakeOnPlay && !surface->IsBaked())
    {
        surface->Bake(m_Scene);
    }
}

void NavigationSystem::OnStop()
{
    auto view = m_Scene.GetRegistry().view<NavMeshAgentComponent>();
    for (auto entity : view)
    {
        auto& agent = view.get<NavMeshAgentComponent>(entity);
        agent.ResetPath();
    }
}

NavMeshSurfaceComponent* NavigationSystem::GetActiveSurface()
{
    auto view = m_Scene.GetRegistry().view<NavMeshSurfaceComponent>();
    for (auto entity : view)
    {
        if (m_Scene.HasComponent<TagComponent>(entity))
        {
            if (!m_Scene.GetComponent<TagComponent>(entity).isActive)
                continue;
        }
        return &view.get<NavMeshSurfaceComponent>(entity);
    }
    return nullptr;
}

std::vector<glm::vec3> NavigationSystem::FindPath(const glm::vec3& start, const glm::vec3& end)
{
    auto surface = GetActiveSurface();
    if (!surface || !surface->IsValid())
        return {};
    return surface->FindPath(start, end);
}

bool NavigationSystem::SamplePosition(const glm::vec3& pos, glm::vec3& outPos, const glm::vec3& extents)
{
    auto surface = GetActiveSurface();
    if (!surface || !surface->IsValid())
        return false;
    return surface->SamplePosition(pos, outPos, extents);
}

bool NavigationSystem::Raycast(const glm::vec3& start, const glm::vec3& end, glm::vec3& outHit)
{
    auto surface = GetActiveSurface();
    if (!surface || !surface->IsValid())
        return false;
    return surface->Raycast(start, end, outHit);
}

void NavigationSystem::Update(float dt)
{
    auto surface = GetActiveSurface();

    if (surface && surface->drawDebugMesh && surface->IsBaked())
    {
        const auto debugLines = surface->GetDebugLines();
        for (const auto& line : debugLines)
        {
            DebugRenderer::Get().DrawLine(line.first, line.second, surface->debugColor);
        }
    }

    if (!surface || !surface->IsValid())
        return;

    auto agentView = m_Scene.GetRegistry().view<TransformComponent, NavMeshAgentComponent>();
    for (auto entity : agentView)
    {
        if (m_Scene.HasComponent<TagComponent>(entity) && !m_Scene.GetComponent<TagComponent>(entity).isActive)
            continue;

        auto& transform = agentView.get<TransformComponent>(entity);
        auto& agent = agentView.get<NavMeshAgentComponent>(entity);

        glm::vec3 currentPos = transform.GetWorldPosition();

        if (agent.requestNewPath)
        {
            agent.requestNewPath = false;
            auto path = surface->FindPath(currentPos, agent.destination);
            if (path.size() >= 2)
            {
                agent.waypoints = std::move(path);
                agent.currentWaypointIndex = 1; // 0 is start position
                agent.hasPath = true;
                agent.isMoving = true;
            }
            else
            {
                agent.hasPath = false;
                agent.isMoving = false;
                agent.velocity = glm::vec3(0.0f);
            }
        }

        if (agent.drawDebugPath && agent.hasPath && !agent.waypoints.empty())
        {
            glm::vec3 p0 = currentPos + glm::vec3(0.0f, 0.2f, 0.0f);
            glm::vec3 p1 = agent.waypoints[agent.currentWaypointIndex] + glm::vec3(0.0f, 0.2f, 0.0f);
            DebugRenderer::Get().DrawLine(p0, p1, glm::vec4(1.0f, 0.9f, 0.1f, 1.0f));

            for (size_t i = agent.currentWaypointIndex; i + 1 < agent.waypoints.size(); ++i)
            {
                glm::vec3 from = agent.waypoints[i] + glm::vec3(0.0f, 0.2f, 0.0f);
                glm::vec3 to = agent.waypoints[i + 1] + glm::vec3(0.0f, 0.2f, 0.0f);
                DebugRenderer::Get().DrawLine(from, to, glm::vec4(0.2f, 0.8f, 1.0f, 1.0f));
            }
        }

        if (agent.isStopped || !agent.isMoving || !agent.hasPath)
            continue;

        if (agent.autoRepath)
        {
            agent.repathTimer += dt;
            if (agent.repathTimer >= agent.repathInterval)
            {
                agent.repathTimer = 0.0f;
                auto newPath = surface->FindPath(currentPos, agent.destination);
                if (newPath.size() >= 2)
                {
                    agent.waypoints = std::move(newPath);
                    agent.currentWaypointIndex = 1;
                }
            }
        }

        if (agent.currentWaypointIndex >= agent.waypoints.size())
        {
            agent.isMoving = false;
            agent.hasPath = false;
            agent.velocity = glm::vec3(0.0f);
            continue;
        }

        glm::vec3 targetWaypoint = agent.waypoints[agent.currentWaypointIndex];
        glm::vec3 toWaypoint = targetWaypoint - currentPos;
        float dist2D = glm::length(glm::vec2(toWaypoint.x, toWaypoint.z));

        constexpr float WAYPOINT_THRESHOLD = 0.35f;
        if (dist2D < WAYPOINT_THRESHOLD)
        {
            if (agent.currentWaypointIndex + 1 < agent.waypoints.size())
            {
                agent.currentWaypointIndex++;
                targetWaypoint = agent.waypoints[agent.currentWaypointIndex];
                toWaypoint = targetWaypoint - currentPos;
                dist2D = glm::length(glm::vec2(toWaypoint.x, toWaypoint.z));
            }
            else
            {
                // Reached final waypoint
                if (dist2D <= agent.stoppingDistance)
                {
                    agent.isMoving = false;
                    agent.hasPath = false;
                    agent.velocity = glm::vec3(0.0f);
                    continue;
                }
            }
        }

        glm::vec3 moveDir = glm::vec3(toWaypoint.x, 0.0f, toWaypoint.z);
        float len = glm::length(moveDir);
        if (len > 0.001f)
        {
            moveDir /= len;
            glm::vec3 desiredVelocity = moveDir * agent.speed;
            float accelRate = std::clamp(agent.acceleration * dt, 0.0f, 1.0f);
            agent.velocity = glm::mix(agent.velocity, desiredVelocity, accelRate);
        }
        else
        {
            agent.velocity = glm::vec3(0.0f);
        }

        if (agent.updateRotation && len > 0.01f)
        {
            glm::quat currentRot = transform.GetRotation();
            glm::quat targetRot = glm::quatLookAt(-moveDir, glm::vec3(0.0f, 1.0f, 0.0f));
            float rotStep = std::clamp(glm::radians(agent.angularSpeed) * dt, 0.0f, 1.0f);
            transform.SetRotation(glm::slerp(currentRot, targetRot, rotStep));
        }

        if (agent.updatePosition)
        {
            if (m_Scene.HasComponent<CharacterBodyComponent>(entity))
            {
                auto& cb = m_Scene.GetComponent<CharacterBodyComponent>(entity);
                m_Scene.GetPhysicsSystem().GetPhysicsWorld().SetCharacterLinearVelocity(&cb, agent.velocity);
            }
            else if (m_Scene.HasComponent<RigidbodyComponent>(entity))
            {
                auto& rb = m_Scene.GetComponent<RigidbodyComponent>(entity);
                if (rb.bodyType == Physics::MotionType::KINEMATIC || rb.bodyType == Physics::MotionType::DYNAMIC)
                {
                    glm::vec3 currentVel = m_Scene.GetPhysicsSystem().GetPhysicsWorld().GetLinearVelocity(&rb);
                    m_Scene.GetPhysicsSystem().GetPhysicsWorld().SetLinearVelocity(&rb, glm::vec3(agent.velocity.x, currentVel.y, agent.velocity.z));
                }
                else
                {
                    transform.Translate(agent.velocity * dt);
                }
            }
            else
            {
                transform.Translate(agent.velocity * dt);
            }
        }
    }
}

} // namespace rv
