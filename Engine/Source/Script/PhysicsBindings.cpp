#include "rvelapch.h"
#include "sol/sol.hpp"
#include "PhysicsBindings.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/CharacterBodyComponent.h"
#include "Scene/Components/ColliderComponents.h"
#include "Scene/Components/PhysicsConstraintComponent.h"
#include "Scene/Components/UUIDComponent.h"
#include "Physics/CollisionInfo.h"
#include "Physics/UserData.h"
#include "Scene/Entity.h"
#include "Core/Engine.h"
#include "ComponentHandle.h"
#include "Math/RvelaMath.h"
#include "Utils/ProjectManager.h"

#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>

using RigidbodyHandle = rv::ComponentHandle<rv::RigidbodyComponent>;
using CharacterBodyHandle = rv::ComponentHandle<rv::CharacterBodyComponent>;
using CapsuleColliderHandle = rv::ComponentHandle<rv::CapsuleColliderComponent>;
using BoxColliderHandle = rv::ComponentHandle<rv::BoxColliderComponent>;
using SphereColliderHandle = rv::ComponentHandle<rv::SphereColliderComponent>;
using PhysicsConstraintHandle = rv::ComponentHandle<rv::PhysicsConstraintComponent>;

using namespace rv::Physics;

namespace {

struct ConstraintHandle
{
	uint64_t id = 0;

	bool IsValid() const {
		if (id == 0) return false;
		auto* engine = rv::Engine::Get();
		if (!engine) return false;
		return engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().IsConstraintEnabled(id);
	}

	void Destroy() {
		if (id != 0) {
			auto* engine = rv::Engine::Get();
			if (engine)
				engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().DestroyConstraint(id);
			id = 0;
		}
	}

	void SetEnabled(bool enabled) {
		if (id != 0) {
			auto* engine = rv::Engine::Get();
			if (engine)
				engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().SetConstraintEnabled(id, enabled);
		}
	}

	bool IsEnabled() const {
		if (id != 0) {
			auto* engine = rv::Engine::Get();
			if (engine)
				return engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().IsConstraintEnabled(id);
		}
		return false;
	}

	void SetHingeLimits(float minAngle, float maxAngle) {
		if (id != 0) {
			auto* engine = rv::Engine::Get();
			if (engine)
				engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().SetHingeLimits(id, minAngle, maxAngle);
		}
	}

	void SetHingeMotor(bool enabled, float targetVelocity, float maxTorque) {
		if (id != 0) {
			auto* engine = rv::Engine::Get();
			if (engine)
				engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().SetHingeMotor(id, enabled, targetVelocity, maxTorque);
		}
	}

	float GetHingeAngle() const {
		if (id != 0) {
			auto* engine = rv::Engine::Get();
			if (engine)
				return engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().GetHingeAngle(id);
		}
		return 0.0f;
	}

	void SetSliderLimits(float minLimit, float maxLimit) {
		if (id != 0) {
			auto* engine = rv::Engine::Get();
			if (engine)
				engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().SetSliderLimits(id, minLimit, maxLimit);
		}
	}

	void SetSliderMotor(bool enabled, float targetVelocity, float maxForce) {
		if (id != 0) {
			auto* engine = rv::Engine::Get();
			if (engine)
				engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().SetSliderMotor(id, enabled, targetVelocity, maxForce);
		}
	}

	float GetSliderPosition() const {
		if (id != 0) {
			auto* engine = rv::Engine::Get();
			if (engine)
				return engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().GetSliderPosition(id);
		}
		return 0.0f;
	}

	void SetDistanceRange(float minDistance, float maxDistance) {
		if (id != 0) {
			auto* engine = rv::Engine::Get();
			if (engine)
				engine->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld().SetDistanceRange(id, minDistance, maxDistance);
		}
	}
};

static void ExtractIgnoredEntities(const sol::object& obj, std::vector<entt::entity>& outIgnored)
{
	if (!obj.valid() || obj.is<sol::nil_t>()) return;

	if (obj.is<rv::Entity>())
	{
		rv::Entity e = obj.as<rv::Entity>();
		if (e.IsValid()) outIgnored.push_back(e.GetHandle());
		return;
	}
	if (obj.is<entt::entity>())
	{
		entt::entity e = obj.as<entt::entity>();
		if (e != entt::null) outIgnored.push_back(e);
		return;
	}
	if (obj.is<sol::table>())
	{
		sol::table t = obj.as<sol::table>();
		for (const auto& pair : t)
		{
			if (pair.second.is<rv::Entity>())
			{
				rv::Entity e = pair.second.as<rv::Entity>();
				if (e.IsValid()) outIgnored.push_back(e.GetHandle());
			}
			else if (pair.second.is<entt::entity>())
			{
				entt::entity e = pair.second.as<entt::entity>();
				if (e != entt::null) outIgnored.push_back(e);
			}
		}
	}
}

static RaycastOptions ParseRaycastOptions(sol::variadic_args va)
{
	RaycastOptions opts;
	if (va.size() == 0) return opts;

	sol::object first = va[0];
	if (first.is<sol::table>())
	{
		sol::table t = first.as<sol::table>();
		if (t["origin"].valid()) opts.origin = t["origin"];
		if (t["direction"].valid()) opts.direction = t["direction"];
		else if (t["dir"].valid()) opts.direction = t["dir"];

		if (t["maxDistance"].valid()) opts.maxDistance = t["maxDistance"];
		else if (t["maxDist"].valid()) opts.maxDistance = t["maxDist"];
		else if (t["distance"].valid()) opts.maxDistance = t["distance"];

		if (t["hitInside"].valid()) opts.hitInside = t["hitInside"];

		if (t["exclude"].valid()) ExtractIgnoredEntities(t["exclude"], opts.ignoredEntities);
		else if (t["ignore"].valid()) ExtractIgnoredEntities(t["ignore"], opts.ignoredEntities);
		else if (t["ignoredEntities"].valid()) ExtractIgnoredEntities(t["ignoredEntities"], opts.ignoredEntities);
		return opts;
	}

	if (va.size() >= 1 && va[0].is<glm::vec3>()) opts.origin = va[0].as<glm::vec3>();
	if (va.size() >= 2 && va[1].is<glm::vec3>()) opts.direction = va[1].as<glm::vec3>();
	if (va.size() >= 3 && va[2].is<float>()) opts.maxDistance = va[2].as<float>();
	if (va.size() >= 4 && va[3].is<bool>()) opts.hitInside = va[3].as<bool>();
	if (va.size() >= 5) ExtractIgnoredEntities(va[4], opts.ignoredEntities);

	return opts;
}

static rv::RigidbodyComponent* GetRigidbodyFromObject(const sol::object& obj)
{
	if (!obj.valid() || obj.is<sol::nil_t>()) return nullptr;
	if (obj.is<RigidbodyHandle>()) {
		auto h = obj.as<RigidbodyHandle>();
		return h.Get();
	}
	if (obj.is<rv::Entity>()) {
		auto e = obj.as<rv::Entity>();
		if (e.IsValid() && e.HasComponent<rv::RigidbodyComponent>())
			return &e.GetComponent<rv::RigidbodyComponent>();
	}
	return nullptr;
}

}

void rv::LuaBindings::RegisterPhysicsAPI(sol::state& lua)
{
	lua.new_enum<MotionType>("MotionType", {
		{ "Static",    MotionType::STATIC    },
		{ "Kinematic", MotionType::KINEMATIC },
		{ "Dynamic",   MotionType::DYNAMIC   }
	});

	lua.new_enum<ConstraintType>("ConstraintType", {
		{ "Fixed",    ConstraintType::Fixed },
		{ "Point",    ConstraintType::Point },
		{ "Distance", ConstraintType::Distance },
		{ "Hinge",    ConstraintType::Hinge },
		{ "Slider",   ConstraintType::Slider },
		{ "Cone",     ConstraintType::Cone }
	});

	lua.new_enum<ConstraintSpace>("ConstraintSpace", {
		{ "WorldSpace", ConstraintSpace::WorldSpace },
		{ "LocalSpace", ConstraintSpace::LocalSpace }
	});

	lua.new_usertype<CollisionInfo>("CollisionInfo",
		"point", sol::property([](CollisionInfo& ci) { return ci.collision.point; }),
		"normal", sol::property([](CollisionInfo& ci) { return ci.collision.normal; }),
		"other", sol::property([](CollisionInfo& ci) -> Entity& { return ci.other; }),
		"isTrigger", sol::property([](CollisionInfo& ci) { return ci.isTrigger; })
	);

	lua.new_usertype<RaycastResult>("RaycastResult",
		"hit", &RaycastResult::hit,
		"distance", &RaycastResult::distance,
		"fraction", &RaycastResult::fraction,
		"point", &RaycastResult::point,
		"normal", &RaycastResult::normal,
		"bodyId", &RaycastResult::bodyId,
		"entity", sol::property(
			[](RaycastResult& r) -> Entity { return Entity(r.entity, &Engine::Get()->GetActiveScene()); }
		)
	);
	lua["ShapeCastResult"] = lua["RaycastResult"];

	lua.new_usertype<ConstraintHandle>("ConstraintHandle",
		"id", &ConstraintHandle::id,
		"IsValid", &ConstraintHandle::IsValid,
		"Destroy", &ConstraintHandle::Destroy,
		"enabled", sol::property(&ConstraintHandle::IsEnabled, &ConstraintHandle::SetEnabled),
		"SetEnabled", &ConstraintHandle::SetEnabled,
		"IsEnabled", &ConstraintHandle::IsEnabled,
		"SetHingeLimits", &ConstraintHandle::SetHingeLimits,
		"SetHingeMotor", &ConstraintHandle::SetHingeMotor,
		"GetHingeAngle", &ConstraintHandle::GetHingeAngle,
		"SetSliderLimits", &ConstraintHandle::SetSliderLimits,
		"SetSliderMotor", &ConstraintHandle::SetSliderMotor,
		"GetSliderPosition", &ConstraintHandle::GetSliderPosition,
		"SetDistanceRange", &ConstraintHandle::SetDistanceRange
	);

	auto GetPW = []() -> PhysicsWorld& {
		return Engine::Get()->GetActiveScene().GetPhysicsSystem().GetPhysicsWorld();
	};

	lua.new_usertype<RigidbodyHandle>("RigidbodyComponent",
		"IsValid", &RigidbodyHandle::IsValid,
		"mass", sol::property(
			[](RigidbodyHandle& h) { return h.Get() ? h.Get()->mass : 0.0f; },
			[](RigidbodyHandle& h, float mass) { if (auto* c = h.Get()) c->mass = mass; }
		),
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
		"eulerRotation", sol::property(
			[](RigidbodyHandle& h) { return h.Get() && h.entity.HasComponent<TransformComponent>() ? h.entity.GetComponent<TransformComponent>().GetEulerRotation() : glm::vec3(0.0f); },
			[=](RigidbodyHandle& h, const glm::vec3& euler) { if (auto* c = h.Get()) GetPW().SetRotation(c, math::EulerToQuat(euler)); }
		),

		"collisionLayer", sol::property(
			[](RigidbodyHandle& h) { return h.Get() ? h.Get()->collisionFilter.layer : 0u; },
			[=](RigidbodyHandle& h, uint32_t l) {
				if (auto* c = h.Get()) {
					c->collisionFilter.layer = l;
					if (!c->RuntimeBodyID.IsInvalid()) {
						if (auto* ud = reinterpret_cast<rv::Physics::UserData*>(GetPW().BodyInterface().GetUserData(c->RuntimeBodyID)))
							ud->filter.layer = l;
					}
				}
			}
		),
		"collisionMask", sol::property(
			[](RigidbodyHandle& h) { return h.Get() ? h.Get()->collisionFilter.mask : 0u; },
			[=](RigidbodyHandle& h, uint32_t m) {
				if (auto* c = h.Get()) {
					c->collisionFilter.mask = m;
					if (!c->RuntimeBodyID.IsInvalid()) {
						if (auto* ud = reinterpret_cast<rv::Physics::UserData*>(GetPW().BodyInterface().GetUserData(c->RuntimeBodyID)))
							ud->filter.mask = m;
					}
				}
			}
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
		"groundPosition", sol::property([=](CharacterBodyHandle& h) { return h.Get() ? GetPW().GetCharacterGroundPosition(h.Get()) : glm::vec3(0.0f); }),
		"collisionLayer", sol::property(
			[](CharacterBodyHandle& h) { return h.Get() ? h.Get()->collisionFilter.layer : 0u; },
			[](CharacterBodyHandle& h, uint32_t l) {
				if (auto* c = h.Get()) {
					c->collisionFilter.layer = l;
					if (c->character) {
						if (auto* ud = reinterpret_cast<rv::Physics::UserData*>(c->character->GetUserData()))
							ud->filter.layer = l;
					}
				}
			}
		),
		"collisionMask", sol::property(
			[](CharacterBodyHandle& h) { return h.Get() ? h.Get()->collisionFilter.mask : 0u; },
			[](CharacterBodyHandle& h, uint32_t m) {
				if (auto* c = h.Get()) {
					c->collisionFilter.mask = m;
					if (c->character) {
						if (auto* ud = reinterpret_cast<rv::Physics::UserData*>(c->character->GetUserData()))
							ud->filter.mask = m;
					}
				}
			}
		)
	);

	lua.new_usertype<CapsuleColliderHandle>("CapsuleColliderComponent",
		"IsValid", &CapsuleColliderHandle::IsValid,
		"radius", sol::property(
			[](CapsuleColliderHandle& h) { return h.Get() ? h.Get()->GetRadius() : 0.0f; },
			[](CapsuleColliderHandle& h, float r) { if (auto* c = h.Get()) c->SetRadius(r); }
		),
		"halfHeight", sol::property(
			[](CapsuleColliderHandle& h) { return h.Get() ? h.Get()->GetHalfHeight() : 0.0f; },
			[](CapsuleColliderHandle& h, float hh) { if (auto* c = h.Get()) c->SetHalfHeight(hh); }
		),
		"offset", sol::property(
			[](CapsuleColliderHandle& h) { return h.Get() ? h.Get()->GetOffset() : glm::vec3(0.0f); },
			[](CapsuleColliderHandle& h, const glm::vec3& o) { if (auto* c = h.Get()) c->SetOffset(o); }
		)
	);

	lua.new_usertype<BoxColliderHandle>("BoxColliderComponent",
		"IsValid", &BoxColliderHandle::IsValid,
		"size", sol::property(
			[](BoxColliderHandle& h) { return h.Get() ? h.Get()->GetSize() : glm::vec3(0.0f); },
			[](BoxColliderHandle& h, const glm::vec3& s) { if (auto* c = h.Get()) c->SetSize(s); }
		),
		"offset", sol::property(
			[](BoxColliderHandle& h) { return h.Get() ? h.Get()->GetOffset() : glm::vec3(0.0f); },
			[](BoxColliderHandle& h, const glm::vec3& o) { if (auto* c = h.Get()) c->SetOffset(o); }
		)
	);

	lua.new_usertype<SphereColliderHandle>("SphereColliderComponent",
		"IsValid", &SphereColliderHandle::IsValid,
		"radius", sol::property(
			[](SphereColliderHandle& h) { return h.Get() ? h.Get()->GetRadius() : 0.0f; },
			[](SphereColliderHandle& h, float r) { if (auto* c = h.Get()) c->SetRadius(r); }
		),
		"offset", sol::property(
			[](SphereColliderHandle& h) { return h.Get() ? h.Get()->GetOffset() : glm::vec3(0.0f); },
			[](SphereColliderHandle& h, const glm::vec3& o) { if (auto* c = h.Get()) c->SetOffset(o); }
		)
	);

	lua.new_usertype<PhysicsConstraintHandle>("PhysicsConstraintComponent",
		"IsValid", &PhysicsConstraintHandle::IsValid,
		"type", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->type : ConstraintType::Fixed; },
			[](PhysicsConstraintHandle& h, ConstraintType t) { if (auto* c = h.Get()) { c->type = t; c->SetDirty(); } }
		),
		"space", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->space : ConstraintSpace::WorldSpace; },
			[](PhysicsConstraintHandle& h, ConstraintSpace s) { if (auto* c = h.Get()) { c->space = s; c->SetDirty(); } }
		),
		"enabled", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->enabled : false; },
			[](PhysicsConstraintHandle& h, bool e) {
				if (auto* c = h.Get()) {
					c->enabled = e;
					if (c->runtimeConstraint) c->runtimeConstraint->SetEnabled(e);
				}
			}
		),
		"autoDetectPoints", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->autoDetectPoints : false; },
			[](PhysicsConstraintHandle& h, bool a) { if (auto* c = h.Get()) { c->autoDetectPoints = a; c->SetDirty(); } }
		),
		"anchorA", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->anchorA : glm::vec3(0.0f); },
			[](PhysicsConstraintHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) { c->anchorA = v; c->SetDirty(); } }
		),
		"anchorB", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->anchorB : glm::vec3(0.0f); },
			[](PhysicsConstraintHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) { c->anchorB = v; c->SetDirty(); } }
		),
		"axisA", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->axisA : glm::vec3(0.0f, 1.0f, 0.0f); },
			[](PhysicsConstraintHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) { c->axisA = v; c->SetDirty(); } }
		),
		"axisB", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->axisB : glm::vec3(0.0f, 1.0f, 0.0f); },
			[](PhysicsConstraintHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) { c->axisB = v; c->SetDirty(); } }
		),
		"minLimit", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->minLimit : 0.0f; },
			[](PhysicsConstraintHandle& h, float v) { if (auto* c = h.Get()) { c->minLimit = v; c->SetDirty(); } }
		),
		"maxLimit", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->maxLimit : 0.0f; },
			[](PhysicsConstraintHandle& h, float v) { if (auto* c = h.Get()) { c->maxLimit = v; c->SetDirty(); } }
		),
		"springFrequency", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->springFrequency : 0.0f; },
			[](PhysicsConstraintHandle& h, float v) { if (auto* c = h.Get()) { c->springFrequency = v; c->SetDirty(); } }
		),
		"springDamping", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->springDamping : 0.0f; },
			[](PhysicsConstraintHandle& h, float v) { if (auto* c = h.Get()) { c->springDamping = v; c->SetDirty(); } }
		),
		"maxFriction", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->maxFriction : 0.0f; },
			[](PhysicsConstraintHandle& h, float v) { if (auto* c = h.Get()) { c->maxFriction = v; c->SetDirty(); } }
		),
		"motorEnabled", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->motorEnabled : false; },
			[](PhysicsConstraintHandle& h, bool v) { if (auto* c = h.Get()) { c->motorEnabled = v; c->SetDirty(); } }
		),
		"motorTargetVelocity", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->motorTargetVelocity : 0.0f; },
			[](PhysicsConstraintHandle& h, float v) { if (auto* c = h.Get()) { c->motorTargetVelocity = v; c->SetDirty(); } }
		),
		"motorMaxForce", sol::property(
			[](PhysicsConstraintHandle& h) { return h.Get() ? h.Get()->motorMaxForce : 0.0f; },
			[](PhysicsConstraintHandle& h, float v) { if (auto* c = h.Get()) { c->motorMaxForce = v; c->SetDirty(); } }
		),
		"SetConnectedEntity", [](PhysicsConstraintHandle& h, sol::optional<Entity> target) {
			if (auto* c = h.Get()) {
				if (target.has_value() && target.value().IsValid() && target.value().HasComponent<UUIDComponent>())
					c->connectedEntityUUID = target.value().GetComponent<UUIDComponent>().uuid;
				else
					c->connectedEntityUUID = 0;
				c->SetDirty();
			}
		},
		"GetConnectedEntity", [](PhysicsConstraintHandle& h) -> sol::optional<Entity> {
			if (auto* c = h.Get()) {
				if (c->connectedEntityUUID != 0 && h.entity.GetScene()) {
					auto& reg = h.entity.GetScene()->GetRegistry();
					auto view = reg.view<UUIDComponent>();
					for (auto e : view) {
						if (view.get<UUIDComponent>(e).uuid == c->connectedEntityUUID)
							return Entity(e, h.entity.GetScene());
					}
				}
			}
			return sol::nullopt;
		},
		"GetHingeAngle", [](PhysicsConstraintHandle& h) -> float {
			if (auto* c = h.Get(); c && c->runtimeConstraint) {
				if (auto* hinge = dynamic_cast<JPH::HingeConstraint*>(c->runtimeConstraint.GetPtr()))
					return hinge->GetCurrentAngle();
			}
			return 0.0f;
		},
		"GetSliderPosition", [](PhysicsConstraintHandle& h) -> float {
			if (auto* c = h.Get(); c && c->runtimeConstraint) {
				if (auto* slider = dynamic_cast<JPH::SliderConstraint*>(c->runtimeConstraint.GetPtr()))
					return slider->GetCurrentPosition();
			}
			return 0.0f;
		}
	);

	auto physicsTable = lua.create_named_table("Physics");

	physicsTable["Raycast"] = [=](sol::variadic_args va) -> RaycastResult {
		RaycastOptions opts = ParseRaycastOptions(va);
		return GetPW().Raycast(opts);
	};

	physicsTable["RaycastAll"] = [=](sol::variadic_args va) -> std::vector<RaycastResult> {
		RaycastOptions opts = ParseRaycastOptions(va);
		return GetPW().RaycastAll(opts);
	};

	physicsTable["SphereCast"] = [=](sol::variadic_args va) -> ShapeCastResult {
		glm::vec3 start(0.0f);
		float radius = 0.5f;
		glm::vec3 dir(0.0f, -1.0f, 0.0f);
		float maxDist = 1000.0f;
		bool hitInside = false;
		std::vector<entt::entity> ignored;

		if (va.size() > 0 && va[0].is<sol::table>()) {
			sol::table t = va[0].as<sol::table>();
			if (t["start"].valid()) start = t["start"];
			else if (t["origin"].valid()) start = t["origin"];
			if (t["radius"].valid()) radius = t["radius"];
			if (t["direction"].valid()) dir = t["direction"];
			else if (t["dir"].valid()) dir = t["dir"];
			if (t["maxDistance"].valid()) maxDist = t["maxDistance"];
			else if (t["maxDist"].valid()) maxDist = t["maxDist"];
			if (t["hitInside"].valid()) hitInside = t["hitInside"];
			if (t["exclude"].valid()) ExtractIgnoredEntities(t["exclude"], ignored);
			else if (t["ignore"].valid()) ExtractIgnoredEntities(t["ignore"], ignored);
		}
		else {
			if (va.size() >= 1 && va[0].is<glm::vec3>()) start = va[0].as<glm::vec3>();
			if (va.size() >= 2 && va[1].is<float>()) radius = va[1].as<float>();
			if (va.size() >= 3 && va[2].is<glm::vec3>()) dir = va[2].as<glm::vec3>();
			if (va.size() >= 4 && va[3].is<float>()) maxDist = va[3].as<float>();
			if (va.size() >= 5 && va[4].is<bool>()) hitInside = va[4].as<bool>();
			if (va.size() >= 6) ExtractIgnoredEntities(va[5], ignored);
		}
		return GetPW().SphereCast(start, radius, dir, maxDist, hitInside, ignored);
	};

	physicsTable["SphereCastAll"] = [=](sol::variadic_args va) -> std::vector<ShapeCastResult> {
		glm::vec3 start(0.0f);
		float radius = 0.5f;
		glm::vec3 dir(0.0f, -1.0f, 0.0f);
		float maxDist = 1000.0f;
		bool hitInside = false;
		std::vector<entt::entity> ignored;

		if (va.size() > 0 && va[0].is<sol::table>()) {
			sol::table t = va[0].as<sol::table>();
			if (t["start"].valid()) start = t["start"];
			else if (t["origin"].valid()) start = t["origin"];
			if (t["radius"].valid()) radius = t["radius"];
			if (t["direction"].valid()) dir = t["direction"];
			else if (t["dir"].valid()) dir = t["dir"];
			if (t["maxDistance"].valid()) maxDist = t["maxDistance"];
			else if (t["maxDist"].valid()) maxDist = t["maxDist"];
			if (t["hitInside"].valid()) hitInside = t["hitInside"];
			if (t["exclude"].valid()) ExtractIgnoredEntities(t["exclude"], ignored);
			else if (t["ignore"].valid()) ExtractIgnoredEntities(t["ignore"], ignored);
		}
		else {
			if (va.size() >= 1 && va[0].is<glm::vec3>()) start = va[0].as<glm::vec3>();
			if (va.size() >= 2 && va[1].is<float>()) radius = va[1].as<float>();
			if (va.size() >= 3 && va[2].is<glm::vec3>()) dir = va[2].as<glm::vec3>();
			if (va.size() >= 4 && va[3].is<float>()) maxDist = va[3].as<float>();
			if (va.size() >= 5 && va[4].is<bool>()) hitInside = va[4].as<bool>();
			if (va.size() >= 6) ExtractIgnoredEntities(va[5], ignored);
		}
		return GetPW().SphereCastAll(start, radius, dir, maxDist, hitInside, ignored);
	};

	physicsTable["BoxCast"] = [=](sol::variadic_args va) -> ShapeCastResult {
		glm::vec3 start(0.0f);
		glm::vec3 halfExtent(0.5f);
		glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 dir(0.0f, -1.0f, 0.0f);
		float maxDist = 1000.0f;
		bool hitInside = false;
		std::vector<entt::entity> ignored;

		if (va.size() > 0 && va[0].is<sol::table>()) {
			sol::table t = va[0].as<sol::table>();
			if (t["start"].valid()) start = t["start"];
			else if (t["origin"].valid()) start = t["origin"];
			if (t["halfExtent"].valid()) halfExtent = t["halfExtent"];
			else if (t["size"].valid()) halfExtent = t["size"].get<glm::vec3>() * 0.5f;
			if (t["rotation"].valid()) rot = t["rotation"];
			else if (t["rot"].valid()) rot = t["rot"];
			if (t["direction"].valid()) dir = t["direction"];
			else if (t["dir"].valid()) dir = t["dir"];
			if (t["maxDistance"].valid()) maxDist = t["maxDistance"];
			else if (t["maxDist"].valid()) maxDist = t["maxDist"];
			if (t["hitInside"].valid()) hitInside = t["hitInside"];
			if (t["exclude"].valid()) ExtractIgnoredEntities(t["exclude"], ignored);
			else if (t["ignore"].valid()) ExtractIgnoredEntities(t["ignore"], ignored);
		}
		else {
			if (va.size() >= 1 && va[0].is<glm::vec3>()) start = va[0].as<glm::vec3>();
			if (va.size() >= 2 && va[1].is<glm::vec3>()) halfExtent = va[1].as<glm::vec3>();
			if (va.size() >= 3 && va[2].is<glm::quat>()) rot = va[2].as<glm::quat>();
			if (va.size() >= 4 && va[3].is<glm::vec3>()) dir = va[3].as<glm::vec3>();
			if (va.size() >= 5 && va[4].is<float>()) maxDist = va[4].as<float>();
			if (va.size() >= 6 && va[5].is<bool>()) hitInside = va[5].as<bool>();
			if (va.size() >= 7) ExtractIgnoredEntities(va[6], ignored);
		}
		return GetPW().BoxCast(start, halfExtent, rot, dir, maxDist, hitInside, ignored);
	};

	physicsTable["BoxCastAll"] = [=](sol::variadic_args va) -> std::vector<ShapeCastResult> {
		glm::vec3 start(0.0f);
		glm::vec3 halfExtent(0.5f);
		glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 dir(0.0f, -1.0f, 0.0f);
		float maxDist = 1000.0f;
		bool hitInside = false;
		std::vector<entt::entity> ignored;

		if (va.size() > 0 && va[0].is<sol::table>()) {
			sol::table t = va[0].as<sol::table>();
			if (t["start"].valid()) start = t["start"];
			else if (t["origin"].valid()) start = t["origin"];
			if (t["halfExtent"].valid()) halfExtent = t["halfExtent"];
			else if (t["size"].valid()) halfExtent = t["size"].get<glm::vec3>() * 0.5f;
			if (t["rotation"].valid()) rot = t["rotation"];
			else if (t["rot"].valid()) rot = t["rot"];
			if (t["direction"].valid()) dir = t["direction"];
			else if (t["dir"].valid()) dir = t["dir"];
			if (t["maxDistance"].valid()) maxDist = t["maxDistance"];
			else if (t["maxDist"].valid()) maxDist = t["maxDist"];
			if (t["hitInside"].valid()) hitInside = t["hitInside"];
			if (t["exclude"].valid()) ExtractIgnoredEntities(t["exclude"], ignored);
			else if (t["ignore"].valid()) ExtractIgnoredEntities(t["ignore"], ignored);
		}
		else {
			if (va.size() >= 1 && va[0].is<glm::vec3>()) start = va[0].as<glm::vec3>();
			if (va.size() >= 2 && va[1].is<glm::vec3>()) halfExtent = va[1].as<glm::vec3>();
			if (va.size() >= 3 && va[2].is<glm::quat>()) rot = va[2].as<glm::quat>();
			if (va.size() >= 4 && va[3].is<glm::vec3>()) dir = va[3].as<glm::vec3>();
			if (va.size() >= 5 && va[4].is<float>()) maxDist = va[4].as<float>();
			if (va.size() >= 6 && va[5].is<bool>()) hitInside = va[5].as<bool>();
			if (va.size() >= 7) ExtractIgnoredEntities(va[6], ignored);
		}
		return GetPW().BoxCastAll(start, halfExtent, rot, dir, maxDist, hitInside, ignored);
	};

	physicsTable["CapsuleCast"] = [=](sol::variadic_args va) -> ShapeCastResult {
		glm::vec3 start(0.0f);
		float radius = 0.5f;
		float halfHeight = 1.0f;
		glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 dir(0.0f, -1.0f, 0.0f);
		float maxDist = 1000.0f;
		bool hitInside = false;
		std::vector<entt::entity> ignored;

		if (va.size() > 0 && va[0].is<sol::table>()) {
			sol::table t = va[0].as<sol::table>();
			if (t["start"].valid()) start = t["start"];
			else if (t["origin"].valid()) start = t["origin"];
			if (t["radius"].valid()) radius = t["radius"];
			if (t["halfHeight"].valid()) halfHeight = t["halfHeight"];
			if (t["rotation"].valid()) rot = t["rotation"];
			else if (t["rot"].valid()) rot = t["rot"];
			if (t["direction"].valid()) dir = t["direction"];
			else if (t["dir"].valid()) dir = t["dir"];
			if (t["maxDistance"].valid()) maxDist = t["maxDistance"];
			else if (t["maxDist"].valid()) maxDist = t["maxDist"];
			if (t["hitInside"].valid()) hitInside = t["hitInside"];
			if (t["exclude"].valid()) ExtractIgnoredEntities(t["exclude"], ignored);
			else if (t["ignore"].valid()) ExtractIgnoredEntities(t["ignore"], ignored);
		}
		else {
			if (va.size() >= 1 && va[0].is<glm::vec3>()) start = va[0].as<glm::vec3>();
			if (va.size() >= 2 && va[1].is<float>()) radius = va[1].as<float>();
			if (va.size() >= 3 && va[2].is<float>()) halfHeight = va[2].as<float>();
			if (va.size() >= 4 && va[3].is<glm::quat>()) rot = va[3].as<glm::quat>();
			if (va.size() >= 5 && va[4].is<glm::vec3>()) dir = va[4].as<glm::vec3>();
			if (va.size() >= 6 && va[5].is<float>()) maxDist = va[5].as<float>();
			if (va.size() >= 7 && va[6].is<bool>()) hitInside = va[6].as<bool>();
			if (va.size() >= 8) ExtractIgnoredEntities(va[7], ignored);
		}
		return GetPW().CapsuleCast(start, radius, halfHeight, rot, dir, maxDist, hitInside, ignored);
	};

	physicsTable["CapsuleCastAll"] = [=](sol::variadic_args va) -> std::vector<ShapeCastResult> {
		glm::vec3 start(0.0f);
		float radius = 0.5f;
		float halfHeight = 1.0f;
		glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 dir(0.0f, -1.0f, 0.0f);
		float maxDist = 1000.0f;
		bool hitInside = false;
		std::vector<entt::entity> ignored;

		if (va.size() > 0 && va[0].is<sol::table>()) {
			sol::table t = va[0].as<sol::table>();
			if (t["start"].valid()) start = t["start"];
			else if (t["origin"].valid()) start = t["origin"];
			if (t["radius"].valid()) radius = t["radius"];
			if (t["halfHeight"].valid()) halfHeight = t["halfHeight"];
			if (t["rotation"].valid()) rot = t["rotation"];
			else if (t["rot"].valid()) rot = t["rot"];
			if (t["direction"].valid()) dir = t["direction"];
			else if (t["dir"].valid()) dir = t["dir"];
			if (t["maxDistance"].valid()) maxDist = t["maxDistance"];
			else if (t["maxDist"].valid()) maxDist = t["maxDist"];
			if (t["hitInside"].valid()) hitInside = t["hitInside"];
			if (t["exclude"].valid()) ExtractIgnoredEntities(t["exclude"], ignored);
			else if (t["ignore"].valid()) ExtractIgnoredEntities(t["ignore"], ignored);
		}
		else {
			if (va.size() >= 1 && va[0].is<glm::vec3>()) start = va[0].as<glm::vec3>();
			if (va.size() >= 2 && va[1].is<float>()) radius = va[1].as<float>();
			if (va.size() >= 3 && va[2].is<float>()) halfHeight = va[2].as<float>();
			if (va.size() >= 4 && va[3].is<glm::quat>()) rot = va[3].as<glm::quat>();
			if (va.size() >= 5 && va[4].is<glm::vec3>()) dir = va[4].as<glm::vec3>();
			if (va.size() >= 6 && va[5].is<float>()) maxDist = va[5].as<float>();
			if (va.size() >= 7 && va[6].is<bool>()) hitInside = va[6].as<bool>();
			if (va.size() >= 8) ExtractIgnoredEntities(va[7], ignored);
		}
		return GetPW().CapsuleCastAll(start, radius, halfHeight, rot, dir, maxDist, hitInside, ignored);
	};

	physicsTable["CreateFixedConstraint"] = [=](sol::object bodyAObj, sol::optional<sol::object> bodyBObj, sol::optional<bool> autoDetect) -> ConstraintHandle {
		auto* a = GetRigidbodyFromObject(bodyAObj);
		auto* b = bodyBObj.has_value() ? GetRigidbodyFromObject(bodyBObj.value()) : nullptr;
		uint64_t id = GetPW().CreateFixedConstraint(a, b, autoDetect.value_or(true));
		return ConstraintHandle{ id };
	};

	physicsTable["CreatePointConstraint"] = [=](sol::object bodyAObj, sol::optional<sol::object> bodyBObj, sol::optional<glm::vec3> pivot, sol::optional<bool> worldSpace) -> ConstraintHandle {
		auto* a = GetRigidbodyFromObject(bodyAObj);
		auto* b = bodyBObj.has_value() ? GetRigidbodyFromObject(bodyBObj.value()) : nullptr;
		uint64_t id = GetPW().CreatePointConstraint(a, b, pivot.value_or(glm::vec3(0.0f)), worldSpace.value_or(true));
		return ConstraintHandle{ id };
	};

	physicsTable["CreateDistanceConstraint"] = [=](sol::object bodyAObj, sol::optional<sol::object> bodyBObj,
		sol::optional<glm::vec3> pointA, sol::optional<glm::vec3> pointB,
		sol::optional<float> minDistance, sol::optional<float> maxDistance,
		sol::optional<bool> worldSpace) -> ConstraintHandle {
		auto* a = GetRigidbodyFromObject(bodyAObj);
		auto* b = bodyBObj.has_value() ? GetRigidbodyFromObject(bodyBObj.value()) : nullptr;
		uint64_t id = GetPW().CreateDistanceConstraint(a, b,
			pointA.value_or(glm::vec3(0.0f)), pointB.value_or(glm::vec3(0.0f)),
			minDistance.value_or(-1.0f), maxDistance.value_or(-1.0f),
			worldSpace.value_or(true));
		return ConstraintHandle{ id };
	};

	physicsTable["CreateHingeConstraint"] = [=](sol::object bodyAObj, sol::optional<sol::object> bodyBObj,
		sol::optional<glm::vec3> pivot, sol::optional<glm::vec3> axis,
		sol::optional<float> minAngle, sol::optional<float> maxAngle,
		sol::optional<bool> worldSpace) -> ConstraintHandle {
		auto* a = GetRigidbodyFromObject(bodyAObj);
		auto* b = bodyBObj.has_value() ? GetRigidbodyFromObject(bodyBObj.value()) : nullptr;
		uint64_t id = GetPW().CreateHingeConstraint(a, b,
			pivot.value_or(glm::vec3(0.0f)), axis.value_or(glm::vec3(0.0f, 1.0f, 0.0f)),
			minAngle.value_or(-3.14159265f), maxAngle.value_or(3.14159265f),
			worldSpace.value_or(true));
		return ConstraintHandle{ id };
	};

	physicsTable["CreateSliderConstraint"] = [=](sol::object bodyAObj, sol::optional<sol::object> bodyBObj,
		sol::optional<glm::vec3> sliderAxis,
		sol::optional<float> minLimit, sol::optional<float> maxLimit,
		sol::optional<bool> worldSpace) -> ConstraintHandle {
		auto* a = GetRigidbodyFromObject(bodyAObj);
		auto* b = bodyBObj.has_value() ? GetRigidbodyFromObject(bodyBObj.value()) : nullptr;
		uint64_t id = GetPW().CreateSliderConstraint(a, b,
			sliderAxis.value_or(glm::vec3(1.0f, 0.0f, 0.0f)),
			minLimit.value_or(-100.0f), maxLimit.value_or(100.0f),
			worldSpace.value_or(true));
		return ConstraintHandle{ id };
	};

	physicsTable["CreateConeConstraint"] = [=](sol::object bodyAObj, sol::optional<sol::object> bodyBObj,
		sol::optional<glm::vec3> pivot, sol::optional<glm::vec3> axis,
		sol::optional<float> halfAngle, sol::optional<bool> worldSpace) -> ConstraintHandle {
		auto* a = GetRigidbodyFromObject(bodyAObj);
		auto* b = bodyBObj.has_value() ? GetRigidbodyFromObject(bodyBObj.value()) : nullptr;
		uint64_t id = GetPW().CreateConeConstraint(a, b,
			pivot.value_or(glm::vec3(0.0f)), axis.value_or(glm::vec3(0.0f, 1.0f, 0.0f)),
			halfAngle.value_or(0.785398f), worldSpace.value_or(true));
		return ConstraintHandle{ id };
	};

	physicsTable["DestroyConstraint"] = [=](sol::object constraintObj) -> bool {
		if (constraintObj.is<ConstraintHandle>()) {
			ConstraintHandle h = constraintObj.as<ConstraintHandle>();
			return GetPW().DestroyConstraint(h.id);
		}
		if (constraintObj.is<uint64_t>()) {
			return GetPW().DestroyConstraint(constraintObj.as<uint64_t>());
		}
		return false;
	};

	physicsTable["GetLayerName"] = [](uint32_t layer) -> std::string {
		auto activeProj = rv::ProjectManager::GetActiveProject();
		if (!activeProj) return "";
		if (layer == 0) return activeProj->GetSettings().GetPhysicsLayerNameByBit(0);
		return activeProj->GetSettings().GetPhysicsLayerName(layer);
	};

	physicsTable["GetLayerMask"] = [](sol::variadic_args va) -> uint32_t {
		auto activeProj = rv::ProjectManager::GetActiveProject();
		if (!activeProj || va.size() == 0) return 0;
		uint32_t mask = 0;
		for (auto item : va) {
			if (item.is<std::string>()) {
				mask |= activeProj->GetSettings().GetPhysicsLayerMask(item.as<std::string>());
			} else if (item.is<uint32_t>()) {
				uint32_t l = item.as<uint32_t>();
				if (l >= 1 && l <= 32) mask |= (1u << (l - 1));
				else if (l == 0) mask |= 1u;
			}
		}
		return mask;
	};
}
