#include "rvelapch.h"
#include "sol/sol.hpp"
#include "PhysicsBindings.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/CharacterBodyComponent.h"
#include "Physics/CollisionInfo.h"
#include "Scene/Entity.h"
#include "Core/Engine.h" // ...

using namespace rv::Physics;

void rv::LuaBindings::RegisterPhysicsAPI(sol::state& lua)
{
    lua.new_enum<MotionType>("MotionType", {
    { "Static",    MotionType::STATIC    },
    { "Kinematic", MotionType::KINEMATIC },
    { "Dynamic",   MotionType::DYNAMIC   }
        });

    lua.new_usertype<CollisionInfo>("CollisionInfo",
        "point", &CollisionInfo::point,
        "normal", &CollisionInfo::normal,
        "other", sol::property(
            [](CollisionInfo& ci) -> Entity& { return ci.other; }
        )
    );

    lua.new_usertype<RaycastResult>("RaycastResult",
        "hit", &RaycastResult::hit,
        "distance", &RaycastResult::distance,
        "point", &RaycastResult::point,
        "normal", &RaycastResult::normal,
        "entity", sol::property(
            [](RaycastResult& r) -> Entity { return Entity(r.entity,&Engine::Get()->GetActiveScene()); } // TODO: this is bad
        )
    );

    lua.new_usertype<PhysicsWorld>("Physics",
        "SetPosition", &PhysicsWorld::SetPosition,
        "SetRotation", &PhysicsWorld::SetRotation,
        "MoveKinematic", &PhysicsWorld::MoveKinematic,

        "SetMotionType", &PhysicsWorld::SetMotionType,
        "GetMotionType", &PhysicsWorld::GetMotionType,

        "AddForce", &PhysicsWorld::AddForce,
        "AddImpulse", &PhysicsWorld::AddImpulse,
        "AddTorque", &PhysicsWorld::AddTorque,
        "AddAngularImpulse", &PhysicsWorld::AddAngularImpulse,

        "AddLinearVelocity", &PhysicsWorld::AddLinearVelocity,
        "SetLinearVelocity", &PhysicsWorld::SetLinearVelocity,
        "GetLinearVelocity", &PhysicsWorld::GetLinearVelocity,
        "SetMaxLinearVelocity", &PhysicsWorld::SetMaxLinearVelocity,
        "GetMaxLinearVelocity", &PhysicsWorld::GetMaxLinearVelocity,

        "SetAngularVelocity", &PhysicsWorld::SetAngularVelocity,
        "GetAngularVelocity", &PhysicsWorld::GetAngularVelocity,
        "SetMaxAngularVelocity", &PhysicsWorld::SetMaxAngularVelocity,
        "GetMaxAngularVelocity", &PhysicsWorld::GetMaxAngularVelocity,

        "GetCenterOfMassPosition", &PhysicsWorld::GetCenterOfMassPosition,

        "SetFriction", &PhysicsWorld::SetFriction,
        "GetFriction", &PhysicsWorld::GetFriction,
        "SetGravityFactor", &PhysicsWorld::SetGravityFactor,
        "GetGravityFactor", &PhysicsWorld::GetGravityFactor,
        "SetRestitution", &PhysicsWorld::SetRestitution,
        "GetRestitution", &PhysicsWorld::GetRestitution,

        "SetIsSensor", &PhysicsWorld::SetIsSensor,
        "IsSensor", &PhysicsWorld::IsSensor,

        "IsCharacterGrounded", &PhysicsWorld::IsCharacterGrounded,
        "IsCharacterOnSteepGround", &PhysicsWorld::IsCharacterOnSteepGround,
        "IsCharacterInAir", &PhysicsWorld::IsCharacterInAir,
        "IsCharacterNotSupported", &PhysicsWorld::IsCharacterNotSupported,

        "GetCharacterPosition", &PhysicsWorld::GetCharacterPosition,
        "SetCharacterPosition", &PhysicsWorld::SetCharacterPosition,
        "GetCharacterRotation", &PhysicsWorld::GetCharacterRotation,
        "SetCharacterRotation", &PhysicsWorld::SetCharacterRotation,
        "GetCharacterCenterOfMassPosition", &PhysicsWorld::GetCharacterCenterOfMassPosition,

        "GetCharacterLinearVelocity", &PhysicsWorld::GetCharacterLinearVelocity,
        "SetCharacterLinearVelocity", &PhysicsWorld::SetCharacterLinearVelocity,

        "GetCharacterUp", &PhysicsWorld::GetCharacterUp,
        "SetCharacterUp", &PhysicsWorld::SetCharacterUp,
        "GetCharacterMaxSlopeAngle", &PhysicsWorld::GetCharacterMaxSlopeAngle,
        "SetCharacterMaxSlopeAngle", &PhysicsWorld::SetCharacterMaxSlopeAngle,

        "GetCharacterGroundNormal", &PhysicsWorld::GetCharacterGroundNormal,
        "GetCharacterGroundVelocity", &PhysicsWorld::GetCharacterGroundVelocity,
        "GetCharacterGroundPosition", &PhysicsWorld::GetCharacterGroundPosition,

        "Raycast", & PhysicsWorld::Raycast
    );
}