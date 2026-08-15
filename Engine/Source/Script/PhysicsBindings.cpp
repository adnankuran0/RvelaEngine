#include "rvelapch.h"
#include "sol/sol.hpp"
#include "PhysicsBindings.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/CharacterBodyComponent.h"
#include "Physics/CollisionInfo.h"
#include "Scene/Entity.h"
#include "Core/Engine.h"

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

    lua.new_usertype<RigidbodyComponent>("RigidbodyComponent",
        "motionType", sol::property(
            [=](RigidbodyComponent& c) { return GetPW().GetMotionType(&c); },
            [=](RigidbodyComponent& c, MotionType type) { GetPW().SetMotionType(&c, type); }
        ),
        "velocity", sol::property( 
            [=](RigidbodyComponent& c) { return GetPW().GetLinearVelocity(&c); },
            [=](RigidbodyComponent& c, const glm::vec3& v) { GetPW().SetLinearVelocity(&c, v); }
        ),
        "maxVelocity", sol::property(
            [=](RigidbodyComponent& c) { return GetPW().GetMaxLinearVelocity(&c); },
            [=](RigidbodyComponent& c, float max) { GetPW().SetMaxLinearVelocity(&c, max); }
        ),
        "angularVelocity", sol::property(
            [=](RigidbodyComponent& c) { return GetPW().GetAngularVelocity(&c); },
            [=](RigidbodyComponent& c, const glm::vec3& v) { GetPW().SetAngularVelocity(&c, v); }
        ),
        "maxAngularVelocity", sol::property(
            [=](RigidbodyComponent& c) { return GetPW().GetMaxAngularVelocity(&c); },
            [=](RigidbodyComponent& c, float max) { GetPW().SetMaxAngularVelocity(&c, max); }
        ),
        "friction", sol::property(
            [=](RigidbodyComponent& c) { return GetPW().GetFriction(&c); },
            [=](RigidbodyComponent& c, float f) { GetPW().SetFriction(&c, f); }
        ),
        "gravityFactor", sol::property(
            [=](RigidbodyComponent& c) { return GetPW().GetGravityFactor(&c); },
            [=](RigidbodyComponent& c, float g) { GetPW().SetGravityFactor(&c, g); }
        ),
        "restitution", sol::property(
            [=](RigidbodyComponent& c) { return GetPW().GetRestitution(&c); },
            [=](RigidbodyComponent& c, float r) { GetPW().SetRestitution(&c, r); }
        ),
        "isSensor", sol::property(
            [=](RigidbodyComponent& c) { return GetPW().IsSensor(&c); },
            [=](RigidbodyComponent& c, bool sensor) { GetPW().SetIsSensor(&c, sensor); }
        ),
        "centerOfMass", sol::property([=](RigidbodyComponent& c) { return GetPW().GetCenterOfMassPosition(&c); }),

        // write only (??)
        "position", sol::property([=](RigidbodyComponent& c, const glm::vec3& p) { GetPW().SetPosition(&c, p); }),
        "rotation", sol::property([=](RigidbodyComponent& c, const glm::quat& r) { GetPW().SetRotation(&c, r); }),

        "MoveKinematic", [=](RigidbodyComponent& c, const glm::vec3& p, const glm::quat& r, float dt) { GetPW().MoveKinematic(&c, p, r, dt); },
        "AddForce", [=](RigidbodyComponent& c, const glm::vec3& f) { GetPW().AddForce(&c, f); },
        "AddImpulse", [=](RigidbodyComponent& c, const glm::vec3& i) { GetPW().AddImpulse(&c, i); },
        "AddTorque", [=](RigidbodyComponent& c, const glm::vec3& t) { GetPW().AddTorque(&c, t); },
        "AddAngularImpulse", [=](RigidbodyComponent& c, const glm::vec3& i) { GetPW().AddAngularImpulse(&c, i); },
        "AddLinearVelocity", [=](RigidbodyComponent& c, const glm::vec3& v) { GetPW().AddLinearVelocity(&c, v); }
    );


    lua.new_usertype<CharacterBodyComponent>("CharacterBodyComponent",
        "position", sol::property(
            [=](CharacterBodyComponent& c) { return GetPW().GetCharacterPosition(&c); },
            [=](CharacterBodyComponent& c, const glm::vec3& p) { GetPW().SetCharacterPosition(&c, p); }
        ),
        "rotation", sol::property(
            [=](CharacterBodyComponent& c) { return GetPW().GetCharacterRotation(&c); },
            [=](CharacterBodyComponent& c, const glm::quat& r) { GetPW().SetCharacterRotation(&c, r); }
        ),
        "velocity", sol::property(
            [=](CharacterBodyComponent& c) { return GetPW().GetCharacterLinearVelocity(&c); },
            [=](CharacterBodyComponent& c, const glm::vec3& v) { GetPW().SetCharacterLinearVelocity(&c, v); }
        ),
        "up", sol::property(
            [=](CharacterBodyComponent& c) { return GetPW().GetCharacterUp(&c); },
            [=](CharacterBodyComponent& c, const glm::vec3& up) { GetPW().SetCharacterUp(&c, up); }
        ),
        "maxSlopeAngle", sol::property(
            [=](CharacterBodyComponent& c) { return GetPW().GetCharacterMaxSlopeAngle(&c); },
            [=](CharacterBodyComponent& c, float angle) { GetPW().SetCharacterMaxSlopeAngle(&c, angle); }
        ),

        // read only
        "isGrounded", sol::property([=](CharacterBodyComponent& c) { return GetPW().IsCharacterGrounded(&c); }),
        "isOnSteepGround", sol::property([=](CharacterBodyComponent& c) { return GetPW().IsCharacterOnSteepGround(&c); }),
        "isInAir", sol::property([=](CharacterBodyComponent& c) { return GetPW().IsCharacterInAir(&c); }),
        "isNotSupported", sol::property([=](CharacterBodyComponent& c) { return GetPW().IsCharacterNotSupported(&c); }),
        "centerOfMass", sol::property([=](CharacterBodyComponent& c) { return GetPW().GetCharacterCenterOfMassPosition(&c); }),
        "groundNormal", sol::property([=](CharacterBodyComponent& c) { return GetPW().GetCharacterGroundNormal(&c); }),
        "groundVelocity", sol::property([=](CharacterBodyComponent& c) { return GetPW().GetCharacterGroundVelocity(&c); }),
        "groundPosition", sol::property([=](CharacterBodyComponent& c) { return GetPW().GetCharacterGroundPosition(&c); })
    );


    lua.create_named_table("Physics",
        "Raycast", [=](const glm::vec3& origin, const glm::vec3& dir, float maxDistance, bool hitInside) {
            return GetPW().Raycast(origin, dir, maxDistance, hitInside);
        }
    );
}