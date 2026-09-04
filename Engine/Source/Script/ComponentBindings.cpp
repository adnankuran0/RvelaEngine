#include "rvelapch.h"
#include "ComponentBindings.h"
#include "Scene/Entity.h"
#include "ComponentHandle.h"

using namespace rv;

using TransformHandle = ComponentHandle<TransformComponent>;
using CameraHandle = ComponentHandle<CameraComponent>;
using DirectionalLightHandle = ComponentHandle<DirectionalLightComponent>;
using RigidbodyHandle = ComponentHandle<RigidbodyComponent>;
using PointLightHandle = ComponentHandle<PointLightComponent>;
using MeshRendererHandle = ComponentHandle<MeshRendererComponent>;
using MaterialHandle = ComponentHandle<MaterialComponent>;
using ParticleEmitterHandle = ComponentHandle<ParticleEmitterComponent>;

void LuaBindings::RegisterComponents(sol::state& lua)
{
    lua.new_usertype<TransformHandle>("TransformComponent",
        "IsValid", &TransformHandle::IsValid,
        "position", sol::property(
            [](TransformHandle& h) { return h.Get() ? h.Get()->GetPosition() : glm::vec3(0.0f); },
            [](TransformHandle& h, const glm::vec3& pos) { if (auto* c = h.Get()) c->SetPosition(pos); }
        ),
        "worldPosition", sol::property([](TransformHandle& h) { return h.Get() ? h.Get()->GetWorldPosition() : glm::vec3(0.0f); }),
        "rotation", sol::property(
            [](TransformHandle& h) { return h.Get() ? h.Get()->GetRotation() : glm::quat(); },
            [](TransformHandle& h, const glm::quat& rot) { if (auto* c = h.Get()) c->SetRotation(rot); }
        ),
        "worldRotation", sol::property([](TransformHandle& h) { return h.Get() ? h.Get()->GetWorldRotation() : glm::quat(); }),
        "eulerRotation", sol::property(
            [](TransformHandle& h) { return h.Get() ? h.Get()->GetEulerRotation() : glm::vec3(0.0f); },
            [](TransformHandle& h, const glm::vec3& euler) { if (auto* c = h.Get()) c->SetEulerRotation(euler); }
        ),
        "scale", sol::property(
            [](TransformHandle& h) { return h.Get() ? h.Get()->GetScale() : glm::vec3(1.0f); },
            [](TransformHandle& h, const glm::vec3& scl) { if (auto* c = h.Get()) c->SetScale(scl); }
        ),
        "worldScale", sol::property([](TransformHandle& h) { return h.Get() ? h.Get()->GetWorldScale() : glm::vec3(1.0f); }),
        "forward", sol::property([](TransformHandle& h) { return h.Get() ? h.Get()->GetForward() : glm::vec3(0.0f); }),
        "up", sol::property([](TransformHandle& h) { return h.Get() ? h.Get()->GetUp() : glm::vec3(0.0f); }),
        "right", sol::property([](TransformHandle& h) { return h.Get() ? h.Get()->GetRight() : glm::vec3(0.0f); }),

        "Translate", [](TransformHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) c->Translate(v); },
        "LookAt", [](TransformHandle& h, const glm::vec3& p, const glm::vec3& up) { if (auto* c = h.Get()) c->LookAt(p, up); }
    );

    lua.new_usertype<CameraHandle>("CameraComponent",
        "IsValid", &CameraHandle::IsValid,
        "fov", sol::property(
            [](CameraHandle& h) { return h.Get() ? (double)h.Get()->GetFOV() : 0.0; },
            [](CameraHandle& h, float fov) { if (auto* c = h.Get()) c->SetFOV(fov); }
        )
    );

    lua.new_usertype<DirectionalLightHandle>("DirectionalLightComponent",
        "IsValid", &DirectionalLightHandle::IsValid,
        "color", sol::property(
            [](DirectionalLightHandle& h) { return h.Get() ? h.Get()->color : glm::vec3(0.0f); },
            [](DirectionalLightHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) c->color = v; }
        ),
        "intensity", sol::property(
            [](DirectionalLightHandle& h) { return h.Get() ? h.Get()->intensity : 0.0f; },
            [](DirectionalLightHandle& h, float v) { if (auto* c = h.Get()) c->intensity = v; }
        ),
        "shadowBias", sol::property(
            [](DirectionalLightHandle& h) { return h.Get() ? h.Get()->shadowBias : 0.0f; },
            [](DirectionalLightHandle& h, float v) { if (auto* c = h.Get()) c->shadowBias = v; }
        ),
        "blurRadius", sol::property(
            [](DirectionalLightHandle& h) { return h.Get() ? h.Get()->blurRadius : 0.0f; },
            [](DirectionalLightHandle& h, float v) { if (auto* c = h.Get()) c->blurRadius = v; }
        ),
        "castShadows", sol::property(
            [](DirectionalLightHandle& h) { return h.Get() ? h.Get()->castShadows : false; },
            [](DirectionalLightHandle& h, bool v) { if (auto* c = h.Get()) c->castShadows = v; }
        ),
        "reverseCullFace", sol::property(
            [](DirectionalLightHandle& h) { return h.Get() ? h.Get()->reverseCullFace : false; },
            [](DirectionalLightHandle& h, bool v) { if (auto* c = h.Get()) c->reverseCullFace = v; }
        )
    );

    lua.new_usertype<RigidbodyHandle>("RigidbodyComponent",
        "IsValid", &RigidbodyHandle::IsValid,
        "mass", sol::property(
            [](RigidbodyHandle& h) { return h.Get() ? h.Get()->mass : 0.0f; },
            [](RigidbodyHandle& h, float v) { if (auto* c = h.Get()) c->mass = v; }
        )
    );

    lua.new_usertype<PointLightHandle>("PointLightComponent",
        "IsValid", &PointLightHandle::IsValid,
        "color", sol::property(
            [](PointLightHandle& h) { return h.Get() ? h.Get()->color : glm::vec3(0.0f); },
            [](PointLightHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) c->color = v; }
        ),
        "intensity", sol::property(
            [](PointLightHandle& h) { return h.Get() ? h.Get()->intensity : 0.0f; },
            [](PointLightHandle& h, float v) { if (auto* c = h.Get()) c->intensity = v; }
        ),
        "radius", sol::property(
            [](PointLightHandle& h) { return h.Get() ? h.Get()->radius : 0.0f; },
            [](PointLightHandle& h, float v) { if (auto* c = h.Get()) c->radius = v; }
        ),
        "falloff", sol::property(
            [](PointLightHandle& h) { return h.Get() ? h.Get()->falloff : 0.0f; },
            [](PointLightHandle& h, float v) { if (auto* c = h.Get()) c->falloff = v; }
        ),
        "castShadows", sol::property(
            [](PointLightHandle& h) { return h.Get() ? h.Get()->castShadows : false; },
            [](PointLightHandle& h, bool v) { if (auto* c = h.Get()) c->castShadows = v; }
        ),
        "reverseCullFace", sol::property(
            [](PointLightHandle& h) { return h.Get() ? h.Get()->reverseCullFace : false; },
            [](PointLightHandle& h, bool v) { if (auto* c = h.Get()) c->reverseCullFace = v; }
        ),
        "shadowBias", sol::property(
            [](PointLightHandle& h) { return h.Get() ? h.Get()->shadowBias : 0.0f; },
            [](PointLightHandle& h, float v) { if (auto* c = h.Get()) c->shadowBias = v; }
        ),
        "blurRadius", sol::property(
            [](PointLightHandle& h) { return h.Get() ? h.Get()->blurRadius : 0.0f; },
            [](PointLightHandle& h, float v) { if (auto* c = h.Get()) c->blurRadius = v; }
        )
    );

    lua.new_usertype<MeshRendererHandle>("MeshRendererComponent",
        "IsValid", &MeshRendererHandle::IsValid,
        "castShadow", sol::property(
            [](MeshRendererHandle& h) { return h.Get() ? h.Get()->IsCastShadow() : false; },
            [](MeshRendererHandle& h, bool v) { if (auto* c = h.Get()) c->SetCastShadow(v); }
        )
    );

    lua.new_usertype<MaterialHandle>("MaterialComponent",
        "IsValid", &MaterialHandle::IsValid,
        "albedoColor", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetAlbedoColor() : glm::vec4(1.0f); },
            [](MaterialHandle& h, const glm::vec4& v) { if (auto* c = h.Get()) c->SetAlbedoColor(v); }
        ),
        "emissiveColor", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetEmissiveColor() : glm::vec3(0.0f); },
            [](MaterialHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) c->SetEmissiveColor(v); }
        ),
        "emissiveIntensity", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetEmissiveIntensity() : 0.0f; },
            [](MaterialHandle& h, float v) { if (auto* c = h.Get()) c->SetEmissiveIntensity(v); }
        ),
        "metallic", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetMetallic() : 0.0f; },
            [](MaterialHandle& h, float v) { if (auto* c = h.Get()) c->SetMetallic(v); }
        ),
        "specular", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetSpecular() : 0.0f; },
            [](MaterialHandle& h, float v) { if (auto* c = h.Get()) c->SetSpecular(v); }
        ),
        "roughness", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetRoughness() : 0.0f; },
            [](MaterialHandle& h, float v) { if (auto* c = h.Get()) c->SetRoughness(v); }
        ),
        "ao", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetAO() : 0.0f; },
            [](MaterialHandle& h, float v) { if (auto* c = h.Get()) c->SetAO(v); }
        ),
        "normalScale", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetNormalScale() : 1.0f; },
            [](MaterialHandle& h, float v) { if (auto* c = h.Get()) c->SetNormalScale(v); }
        ),
        "heightScale", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetHeightScale() : 1.0f; },
            [](MaterialHandle& h, float v) { if (auto* c = h.Get()) c->SetHeightScale(v); }
        ),
        "uvScale", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetUVScale() : glm::vec2(1.0f); },
            [](MaterialHandle& h, const glm::vec2& v) { if (auto* c = h.Get()) c->SetUVScale(v); }
        ),
        "uvOffset", sol::property(
            [](MaterialHandle& h) { return h.Get() ? h.Get()->GetUVOffset() : glm::vec2(0.0f); },
            [](MaterialHandle& h, const glm::vec2& v) { if (auto* c = h.Get()) c->SetUVOffset(v); }
        )
    );

    lua.new_enum<ParticleEmitterShape>("ParticleEmitterShape", {
        { "Point", ParticleEmitterShape::Point },
        { "Sphere", ParticleEmitterShape::Sphere },
        { "SphereSurface", ParticleEmitterShape::SphereSurface },
        { "Box", ParticleEmitterShape::Box}
        });

    lua.new_usertype<ParticleEmitterHandle>("ParticleEmitterComponent",
        "IsValid", &ParticleEmitterHandle::IsValid,
        "emitting", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->emitting : false; },
            [](ParticleEmitterHandle& h, bool v) { if (auto* c = h.Get()) c->emitting = v; }
        ),
        "amount", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->amount : 0; },
            [](ParticleEmitterHandle& h, int v) { if (auto* c = h.Get()) c->amount = v; }
        ),
        "lifetime", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->lifetime : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->lifetime = v; }
        ),
        "oneShot", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->oneShot : false; },
            [](ParticleEmitterHandle& h, bool v) { if (auto* c = h.Get()) c->oneShot = v; }
        ),
        "speedScale", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->speedScale : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->speedScale = v; }
        ),
        "explosiveness", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->explosiveness : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->explosiveness = v; }
        ),
        "randomness", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->randomness : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->randomness = v; }
        ),
        "lifetimeRandomness", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->lifetimeRandomness : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->lifetimeRandomness = v; }
        ),
        "localCoords", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->localCoords : false; },
            [](ParticleEmitterHandle& h, bool v) { if (auto* c = h.Get()) c->localCoords = v; }
        ),
        "emitterShape", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->emitterShape : ParticleEmitterShape::Point; },
            [](ParticleEmitterHandle& h, ParticleEmitterShape v) { if (auto* c = h.Get()) c->emitterShape = v; }
        ),
        "shapeDimensions", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->shapeDimensions : glm::vec3(0.0f); },
            [](ParticleEmitterHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) c->shapeDimensions = v; }
        ),
        "direction", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->direction : glm::vec3(0.0f); },
            [](ParticleEmitterHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) c->direction = v; }
        ),
        "spread", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->spread : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->spread = v; }
        ),
        "gravity", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->gravity : glm::vec3(0.0f); },
            [](ParticleEmitterHandle& h, const glm::vec3& v) { if (auto* c = h.Get()) c->gravity = v; }
        ),
        "linearVelocityMin", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->linearVelocityMin : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->linearVelocityMin = v; }
        ),

        "linearVelocityMax", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->linearVelocityMax : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->linearVelocityMax = v; }
        ),

        "angularVelocityMin", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->angularVelocityMin : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->angularVelocityMin = v; }
        ),

        "angularVelocityMax", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->angularVelocityMax : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->angularVelocityMax = v; }
        ),
        "rotationMin", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->rotationMin : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->rotationMin = v; }
        ),
        "rotationMax", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->rotationMax : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->rotationMax = v; }
        ),
        "linearAccelMin", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->linearAccelMin : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->linearAccelMin = v; }
        ),

        "linearAccelMax", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->linearAccelMax : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->linearAccelMax = v; }
        ),
        "dampingMin", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->dampingMin : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->dampingMin = v; }
        ),
        "dampingMax", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->dampingMax : 0.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->dampingMax = v; }
        ),
        "scaleMin", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->scaleMin : 1.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->scaleMin = v; }
        ),

        "scaleMax", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->scaleMax : 1.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->scaleMax = v; }
        ),

        "scaleEnd", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->scaleEnd : 1.0f; },
            [](ParticleEmitterHandle& h, float v) { if (auto* c = h.Get()) c->scaleEnd = v; }
        ),
        "startColor", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->startColor : glm::vec4(1.0f); },
            [](ParticleEmitterHandle& h, const glm::vec4& v) { if (auto* c = h.Get()) c->startColor = v; }
        ),
        "endColor", sol::property(
            [](ParticleEmitterHandle& h) { return h.Get() ? h.Get()->endColor : glm::vec4(1.0f); },
            [](ParticleEmitterHandle& h, const glm::vec4& v) { if (auto* c = h.Get()) c->endColor = v; }
        )
    );
}