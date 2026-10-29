#include "rvelapch.h"
#include "NavMeshBindings.h"
#include "ComponentHandle.h"
#include "Core/Engine.h"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Scene/Components/NavMeshAgentComponent.h"
#include "Scene/Components/NavMeshSurfaceComponent.h"
#include "Scene/Components/NavMeshObstacleComponent.h"
#include "Navigation/NavigationSystem.h"

namespace rv::LuaBindings {

using NavMeshAgentHandle = rv::ComponentHandle<rv::NavMeshAgentComponent>;
using NavMeshSurfaceHandle = rv::ComponentHandle<rv::NavMeshSurfaceComponent>;
using NavMeshObstacleHandle = rv::ComponentHandle<rv::NavMeshObstacleComponent>;

static NavigationSystem* GetNavSystem()
{
    auto* engine = Engine::Get();
    if (!engine) return nullptr;
    return &engine->GetActiveScene().GetNavigationSystem();
}

void RegisterNavigationAPI(sol::state& lua)
{
    lua.new_usertype<NavMeshAgentHandle>("NavMeshAgentComponent",
        "IsValid", &NavMeshAgentHandle::IsValid,

        "speed", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->speed : 0.0f; },
            [](NavMeshAgentHandle& h, float v) { if (auto* c = h.Get()) c->speed = v; }
        ),
        "angularSpeed", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->angularSpeed : 0.0f; },
            [](NavMeshAgentHandle& h, float v) { if (auto* c = h.Get()) c->angularSpeed = v; }
        ),
        "acceleration", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->acceleration : 0.0f; },
            [](NavMeshAgentHandle& h, float v) { if (auto* c = h.Get()) c->acceleration = v; }
        ),
        "stoppingDistance", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->stoppingDistance : 0.0f; },
            [](NavMeshAgentHandle& h, float v) { if (auto* c = h.Get()) c->stoppingDistance = v; }
        ),
        "radius", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->radius : 0.0f; },
            [](NavMeshAgentHandle& h, float v) { if (auto* c = h.Get()) c->radius = v; }
        ),
        "height", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->height : 0.0f; },
            [](NavMeshAgentHandle& h, float v) { if (auto* c = h.Get()) c->height = v; }
        ),
        "autoRepath", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->autoRepath : false; },
            [](NavMeshAgentHandle& h, bool v) { if (auto* c = h.Get()) c->autoRepath = v; }
        ),
        "repathInterval", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->repathInterval : 0.5f; },
            [](NavMeshAgentHandle& h, float v) { if (auto* c = h.Get()) c->repathInterval = v; }
        ),
        "updateRotation", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->updateRotation : false; },
            [](NavMeshAgentHandle& h, bool v) { if (auto* c = h.Get()) c->updateRotation = v; }
        ),
        "updatePosition", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->updatePosition : false; },
            [](NavMeshAgentHandle& h, bool v) { if (auto* c = h.Get()) c->updatePosition = v; }
        ),
        "drawDebugPath", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->drawDebugPath : false; },
            [](NavMeshAgentHandle& h, bool v) { if (auto* c = h.Get()) c->drawDebugPath = v; }
        ),
        "isMoving", sol::property([](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->isMoving : false; }),
        "hasPath", sol::property([](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->hasPath : false; }),
        "isStopped", sol::property([](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->isStopped : false; }),
        "destination", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->destination : glm::vec3(0.0f); },
            [](NavMeshAgentHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) c->SetDestination(v); }
        ),
        "velocity", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->velocity : glm::vec3(0.0f); }
        ),
        "remainingDistance", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->RemainingDistance() : 0.0f; }
        ),
        "reachedDestination", sol::property(
            [](NavMeshAgentHandle& h) { return h.Get() ? h.Get()->HasReachedDestination() : false; }
        ),

        "SetDestination", [](NavMeshAgentHandle& h, const glm::vec3& target) {
            if (auto* c = h.Get()) c->SetDestination(target);
        },
        "Stop", [](NavMeshAgentHandle& h) {
            if (auto* c = h.Get()) c->Stop();
        },
        "Resume", [](NavMeshAgentHandle& h) {
            if (auto* c = h.Get()) c->Resume();
        },
        "ResetPath", [](NavMeshAgentHandle& h) {
            if (auto* c = h.Get()) c->ResetPath();
        }
    );

    lua.new_usertype<NavMeshSurfaceHandle>("NavMeshSurfaceComponent",
        "IsValid", &NavMeshSurfaceHandle::IsValid,
        "isBaked", sol::property([](NavMeshSurfaceHandle& h) { return h.Get() ? h.Get()->IsBaked() : false; }),
        "autoBakeOnPlay", sol::property(
            [](NavMeshSurfaceHandle& h) { return h.Get() ? h.Get()->autoBakeOnPlay : false; },
            [](NavMeshSurfaceHandle& h, bool v) { if (auto* c = h.Get()) c->autoBakeOnPlay = v; }
        ),
        "drawDebugMesh", sol::property(
            [](NavMeshSurfaceHandle& h) { return h.Get() ? h.Get()->drawDebugMesh : false; },
            [](NavMeshSurfaceHandle& h, bool v) { if (auto* c = h.Get()) c->drawDebugMesh = v; }
        ),
        "Bake", [](NavMeshSurfaceHandle& h) -> bool {
            if (auto* c = h.Get()) {
                if (h.entity.GetScene()) return c->Bake(*h.entity.GetScene());
            }
            return false;
        },
        "Clear", [](NavMeshSurfaceHandle& h) {
            if (auto* c = h.Get()) c->Clear();
        },
        "FindPath", [](NavMeshSurfaceHandle& h, const glm::vec3& start, const glm::vec3& end) -> std::vector<glm::vec3> {
            if (auto* c = h.Get()) return c->FindPath(start, end);
            return {};
        },
        "SamplePosition", [](NavMeshSurfaceHandle& h, const glm::vec3& pos, sol::optional<glm::vec3> extents) -> sol::optional<glm::vec3> {
            if (auto* c = h.Get()) {
                glm::vec3 ext = extents.value_or(glm::vec3(2.0f, 4.0f, 2.0f));
                glm::vec3 outPos;
                if (c->SamplePosition(pos, outPos, ext)) return outPos;
            }
            return sol::nullopt;
        },
        "Raycast", [](NavMeshSurfaceHandle& h, const glm::vec3& start, const glm::vec3& end) -> sol::optional<glm::vec3> {
            if (auto* c = h.Get()) {
                glm::vec3 hit;
                if (c->Raycast(start, end, hit)) return hit;
            }
            return sol::nullopt;
        }
    );

    lua.new_usertype<NavMeshObstacleHandle>("NavMeshObstacleComponent",
        "IsValid", &NavMeshObstacleHandle::IsValid,
        "affectNavMesh", sol::property(
            [](NavMeshObstacleHandle& h) { return h.Get() ? h.Get()->affectNavMesh : true; },
            [](NavMeshObstacleHandle& h, bool v) { if (auto* c = h.Get()) c->affectNavMesh = v; }
        ),
        "isWalkable", sol::property(
            [](NavMeshObstacleHandle& h) { return h.Get() ? h.Get()->isWalkable : true; },
            [](NavMeshObstacleHandle& h, bool v) { if (auto* c = h.Get()) c->isWalkable = v; }
        )
    );

    auto navMeshTable = lua.create_named_table("NavMesh");

    navMeshTable["CalculatePath"] = [](const glm::vec3& start, const glm::vec3& end) -> std::vector<glm::vec3> {
        auto* nav = GetNavSystem();
        if (nav) return nav->FindPath(start, end);
        return {};
    };

    navMeshTable["SamplePosition"] = [](const glm::vec3& pos, sol::optional<glm::vec3> extents) -> sol::optional<glm::vec3> {
        auto* nav = GetNavSystem();
        if (nav) {
            glm::vec3 ext = extents.value_or(glm::vec3(2.0f, 4.0f, 2.0f));
            glm::vec3 outPos;
            if (nav->SamplePosition(pos, outPos, ext)) return outPos;
        }
        return sol::nullopt;
    };

    navMeshTable["Raycast"] = [](const glm::vec3& start, const glm::vec3& end) -> sol::optional<glm::vec3> {
        auto* nav = GetNavSystem();
        if (nav) {
            glm::vec3 hit;
            if (nav->Raycast(start, end, hit)) return hit;
        }
        return sol::nullopt;
    };

    navMeshTable["Bake"] = []() -> bool {
        auto* engine = Engine::Get();
        if (!engine) return false;
        auto& scene = engine->GetActiveScene();
        auto surface = scene.GetNavigationSystem().GetActiveSurface();
        if (surface) return surface->Bake(scene);
        return false;
    };

    navMeshTable["Clear"] = []() {
        auto* nav = GetNavSystem();
        if (nav) {
            auto surface = nav->GetActiveSurface();
            if (surface) surface->Clear();
        }
    };
}

}
