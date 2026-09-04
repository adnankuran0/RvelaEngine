#include "rvelapch.h"
#include "sol/sol.hpp"
#include "PhysicsBindings.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/CharacterBodyComponent.h"
#include "Physics/CollisionInfo.h"
#include "Scene/Entity.h"
#include "Core/Engine.h"
#include "ComponentHandle.h"

using RigidbodyHandle = rv::ComponentHandle<rv::RigidbodyComponent>;
using CharacterBodyHandle = rv::ComponentHandle<rv::CharacterBodyComponent>;

using namespace rv::Physics;


void rv::LuaBindings::RegisterPhysicsAPI(sol::state& lua)
{
    lua.new_enum<MotionType>("MotionType", {
        { "Static",    MotionType::STATIC    },
        { "Kinematic", MotionType::KINEMATIC },
        { "Dynamic",   MotionType::DYNAMIC   }
        });

    lua.new_usertype<CollisionInfo>("CollisionInfo",
        "point", sol::property([](CollisionInfo& ci) { return ci.collision.point; }),
        "normal", sol::property([](CollisionInfo& ci) { return ci.collision.normal; }),
        "other", sol::property([](CollisionInfo& ci) -> Entity& { return ci.other; })
    );

    lua.new_usertype<RaycastResult>("RaycastResult",
        "hit", &RaycastResult::hit,
        "distance", &RaycastResult::distance,
        "point", &RaycastResult::point,
        "normal", &RaycastResult::normal,
        "entity", sol::property(
            [](RaycastResult& r) -> Entity { return Entity(r.entity, &Engine::Get()->GetActiveScene()); }
        )
    );

    auto GetPW = []() -> PhysicsWorld& {
        return Engine::Get()->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld();
        };

    lua.new_usertype<RigidbodyHandle>("RigidbodyComponent",
        "IsValid", &RigidbodyHandle::IsValid,
        "motionType", sol::property(
            [=](RigidbodyHandle& h) { return h.Get() ? GetPW().GetMotionType(h.Get()) : MotionType::STATIC; },
            [=](RigidbodyHandle& h, MotionType type) { if (auto* c = h.Get()) GetPW().SetMotionType(c, type); }
        ),
        "velocity", sol::property(
            [=](RigidbodyHandle& h) { return h.Get() ? GetPW().GetLinearVelocity(h.Get()) : glm::vec3(0.0f); },
            [=](RigidbodyHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) GetPW().SetLinearVelocity(c, v); }
        ),
        "maxVelocity", sol::property(
            [=](RigidbodyHandle& h) { return h.Get() ? GetPW().GetMaxLinearVelocity(h.Get()) : 0.0f; },
            [=](RigidbodyHandle& h, float max) { if (auto* c = h.Get()) GetPW().SetMaxLinearVelocity(c, max); }
        ),
        "angularVelocity", sol::property(
            [=](RigidbodyHandle& h) { return h.Get() ? GetPW().GetAngularVelocity(h.Get()) : glm::vec3(0.0f); },
            [=](RigidbodyHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) GetPW().SetAngularVelocity(c, v); }
        ),
        "maxAngularVelocity", sol::property(
            [=](RigidbodyHandle& h) { return h.Get() ? GetPW().GetMaxAngularVelocity(h.Get()) : 0.0f; },
            [=](RigidbodyHandle& h, float max) { if (auto* c = h.Get()) GetPW().SetMaxAngularVelocity(c, max); }
        ),
        "friction", sol::property(
            [=](RigidbodyHandle& h) { return h.Get() ? GetPW().GetFriction(h.Get()) : 0.0f; },
            [=](RigidbodyHandle& h, float f) { if (auto* c = h.Get()) GetPW().SetFriction(c, f); }
        ),
        "gravityFactor", sol::property(
            [=](RigidbodyHandle& h) { return h.Get() ? GetPW().GetGravityFactor(h.Get()) : 0.0f; },
            [=](RigidbodyHandle& h, float g) { if (auto* c = h.Get()) GetPW().SetGravityFactor(c, g); }
        ),
        "restitution", sol::property(
            [=](RigidbodyHandle& h) { return h.Get() ? GetPW().GetRestitution(h.Get()) : 0.0f; },
            [=](RigidbodyHandle& h, float r) { if (auto* c = h.Get()) GetPW().SetRestitution(c, r); }
        ),
        "isSensor", sol::property(
            [=](RigidbodyHandle& h) { return h.Get() ? GetPW().IsSensor(h.Get()) : false; },
            [=](RigidbodyHandle& h, bool sensor) { if (auto* c = h.Get()) GetPW().SetIsSensor(c, sensor); }
        ),
        "centerOfMass", sol::property([=](RigidbodyHandle& h) { return h.Get() ? GetPW().GetCenterOfMassPosition(h.Get()) : glm::vec3(0.0f); }),

        "position", sol::property(
            [](RigidbodyHandle& h) { return h.Get() && h.entity.HasComponent<TransformComponent>() ? h.entity.GetComponent<TransformComponent>().GetPosition() : glm::vec3(0.0f); },
            [=](RigidbodyHandle& h, const glm::vec3& p) { if (auto* c = h.Get()) GetPW().SetPosition(c, p); }
        ),
        "rotation", sol::property(
            [](RigidbodyHandle& h) { return h.Get() && h.entity.HasComponent<TransformComponent>() ? h.entity.GetComponent<TransformComponent>().GetRotation() : glm::quat(); },
            [=](RigidbodyHandle& h, const glm::quat& r) { if (auto* c = h.Get()) GetPW().SetRotation(c, r); }
        ),

        "MoveKinematic", [=](RigidbodyHandle& h, const glm::vec3& p, const glm::quat& r, float dt) { if (auto* c = h.Get()) GetPW().MoveKinematic(c, p, r, dt); },
        "AddForce", [=](RigidbodyHandle& h, const glm::vec3& f) { if (auto* c = h.Get()) GetPW().AddForce(c, f); },
        "AddImpulse", [=](RigidbodyHandle& h, const glm::vec3& i) { if (auto* c = h.Get()) GetPW().AddImpulse(c, i); },
        "AddTorque", [=](RigidbodyHandle& h, const glm::vec3& t) { if (auto* c = h.Get()) GetPW().AddTorque(c, t); },
        "AddAngularImpulse", [=](RigidbodyHandle& h, const glm::vec3& i) { if (auto* c = h.Get()) GetPW().AddAngularImpulse(c, i); },
        "AddLinearVelocity", [=](RigidbodyHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) GetPW().AddLinearVelocity(c, v); }
    );


    lua.new_usertype<CharacterBodyHandle>("CharacterBodyComponent",
        "IsValid", &CharacterBodyHandle::IsValid,
        "position", sol::property(
            [=](CharacterBodyHandle& h) { return h.Get() ? GetPW().GetCharacterPosition(h.Get()) : glm::vec3(0.0f); },
            [=](CharacterBodyHandle& h, const glm::vec3& p) { if (auto* c = h.Get()) GetPW().SetCharacterPosition(c, p); }
        ),
        "rotation", sol::property(
            [=](CharacterBodyHandle& h) { return h.Get() ? GetPW().GetCharacterRotation(h.Get()) : glm::quat(); },
            [=](CharacterBodyHandle& h, const glm::quat& r) { if (auto* c = h.Get()) GetPW().SetCharacterRotation(c, r); }
        ),
        "velocity", sol::property(
            [=](CharacterBodyHandle& h) { return h.Get() ? GetPW().GetCharacterLinearVelocity(h.Get()) : glm::vec3(0.0f); },
            [=](CharacterBodyHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) GetPW().SetCharacterLinearVelocity(c, v); }
        ),
        "up", sol::property(
            [=](CharacterBodyHandle& h) { return h.Get() ? GetPW().GetCharacterUp(h.Get()) : glm::vec3(0.0f); },
            [=](CharacterBodyHandle& h, const glm::vec3& up) { if (auto* c = h.Get()) GetPW().SetCharacterUp(c, up); }
        ),
        "maxSlopeAngle", sol::property(
            [=](CharacterBodyHandle& h) { return h.Get() ? GetPW().GetCharacterMaxSlopeAngle(h.Get()) : 0.0f; },
            [=](CharacterBodyHandle& h, float angle) { if (auto* c = h.Get()) GetPW().SetCharacterMaxSlopeAngle(c, angle); }
        ),

        "isGrounded", sol::property([=](CharacterBodyHandle& h) { return h.Get() ? GetPW().IsCharacterGrounded(h.Get()) : false; }),
        "isOnSteepGround", sol::property([=](CharacterBodyHandle& h) { return h.Get() ? GetPW().IsCharacterOnSteepGround(h.Get()) : false; }),
        "isInAir", sol::property([=](CharacterBodyHandle& h) { return h.Get() ? GetPW().IsCharacterInAir(h.Get()) : false; }),
        "isNotSupported", sol::property([=](CharacterBodyHandle& h) { return h.Get() ? GetPW().IsCharacterNotSupported(h.Get()) : false; }),
        "centerOfMass", sol::property([=](CharacterBodyHandle& h) { return h.Get() ? GetPW().GetCharacterCenterOfMassPosition(h.Get()) : glm::vec3(0.0f); }),
        "groundNormal", sol::property([=](CharacterBodyHandle& h) { return h.Get() ? GetPW().GetCharacterGroundNormal(h.Get()) : glm::vec3(0.0f); }),
        "groundVelocity", sol::property([=](CharacterBodyHandle& h) { return h.Get() ? GetPW().GetCharacterGroundVelocity(h.Get()) : glm::vec3(0.0f); }),
        "groundPosition", sol::property([=](CharacterBodyHandle& h) { return h.Get() ? GetPW().GetCharacterGroundPosition(h.Get()) : glm::vec3(0.0f); })
    );


    lua.create_named_table("Physics",
        "Raycast", [=](const glm::vec3& origin, const glm::vec3& dir, float maxDistance, bool hitInside) {
            return GetPW().Raycast(origin, dir, maxDistance, hitInside);
        }
    );
}