#include "rvelapch.h"
#include "ComponentBindings.h"
#include "Scene/Entity.h"

using namespace rv;

void LuaBindings::RegisterComponents(sol::state& lua)
{
    lua.new_usertype<TransformComponent>("TransformComponent",
        "position", sol::property(
            &TransformComponent::GetPosition,
            &TransformComponent::SetPosition
        ),
        "worldPosition", sol::property(&TransformComponent::GetWorldPosition),
        "rotation", sol::property(
            &TransformComponent::GetRotation,
            &TransformComponent::SetRotation
        ),
        "worldRotation", sol::property(&TransformComponent::GetWorldRotation),
        "eulerRotation", sol::property(
            &TransformComponent::GetEulerRotation,
            &TransformComponent::SetEulerRotation
        ),
        "scale", sol::property(
            &TransformComponent::GetScale,
            &TransformComponent::SetScale
        ),
        "worldScale", sol::property(&TransformComponent::GetWorldScale),
        "forward", sol::property(&TransformComponent::GetForward),
        "up", sol::property(&TransformComponent::GetUp),
        "right", sol::property(&TransformComponent::GetRight),

        "Translate", &TransformComponent::Translate,
        "LookAt", &TransformComponent::LookAt
    );

    lua.new_usertype<CameraComponent>("CameraComponent",
        "fov", sol::property(
            [](CameraComponent& c) { return (double)c.GetFOV(); },
            &CameraComponent::SetFOV
        )
    );

    lua.new_usertype<rv::DirectionalLightComponent>("DirectionalLightComponent",
        "color", &rv::DirectionalLightComponent::color,
        "intensity", &rv::DirectionalLightComponent::intensity,
        "shadowBias", &rv::DirectionalLightComponent::shadowBias,
        "blurRadius", &rv::DirectionalLightComponent::blurRadius,
        "castShadows", &rv::DirectionalLightComponent::castShadows,
        "reverseCullFace", &rv::DirectionalLightComponent::reverseCullFace
    );

    lua.new_usertype<rv::RigidbodyComponent>("RigidbodyComponent",
        "mass", &rv::RigidbodyComponent::mass
    );

    lua.new_usertype<rv::PointLightComponent>("PointLightComponent",
        "color", &rv::PointLightComponent::color,
        "intensity", &rv::PointLightComponent::intensity,
        "radius", &rv::PointLightComponent::radius,
        "falloff", &rv::PointLightComponent::falloff,
        "castShadows", &rv::PointLightComponent::castShadows,
        "reverseCullFace", &rv::PointLightComponent::reverseCullFace,
        "shadowBias", &rv::PointLightComponent::shadowBias,
        "blurRadius", &rv::PointLightComponent::blurRadius
    );

    lua.new_usertype<rv::MeshRendererComponent>("MeshRendererComponent",
        "castShadow", sol::property(
            &rv::MeshRendererComponent::IsCastShadow,
            &rv::MeshRendererComponent::SetCastShadow
        )
    );

    lua.new_usertype<rv::MaterialComponent>("MaterialComponent",
        "albedoColor", sol::property(
            &rv::MaterialComponent::GetAlbedoColor,
            &rv::MaterialComponent::SetAlbedoColor
        ),
        "emissiveColor", sol::property(
            &rv::MaterialComponent::GetEmissiveColor,
            &rv::MaterialComponent::SetEmissiveColor
        ),
        "emissiveIntensity", sol::property(
            &rv::MaterialComponent::GetEmissiveIntensity,
            &rv::MaterialComponent::SetEmissiveIntensity
        ),
        "metallic", sol::property(
            &rv::MaterialComponent::GetMetallic,
            &rv::MaterialComponent::SetMetallic
        ),
        "specular", sol::property(
            &rv::MaterialComponent::GetSpecular,
            &rv::MaterialComponent::SetSpecular
        ),
        "roughness", sol::property(
            &rv::MaterialComponent::GetRoughness,
            &rv::MaterialComponent::SetRoughness
        ),
        "ao", sol::property(
            &rv::MaterialComponent::GetAO,
            &rv::MaterialComponent::SetAO
        ),
        "normalScale", sol::property(
            &rv::MaterialComponent::GetNormalScale,
            &rv::MaterialComponent::SetNormalScale
        ),
        "heightScale", sol::property(
            &rv::MaterialComponent::GetHeightScale,
            &rv::MaterialComponent::SetHeightScale
        ),
        "uvScale", sol::property(
            &rv::MaterialComponent::GetUVScale,
            &rv::MaterialComponent::SetUVScale
        ),
        "uvOffset", sol::property(
            &rv::MaterialComponent::GetUVOffset,
            &rv::MaterialComponent::SetUVOffset
        )
    );

    lua.new_enum<rv::ParticleEmitterShape>("ParticleEmitterShape", {
        { "Point", rv::ParticleEmitterShape::Point },
        { "Sphere", rv::ParticleEmitterShape::Sphere },
        { "SphereSurface", rv::ParticleEmitterShape::SphereSurface },
        { "Box", rv::ParticleEmitterShape::Box}
        });

    lua.new_usertype<rv::ParticleEmitterComponent>("ParticleEmitterComponent",
        "emitting", &rv::ParticleEmitterComponent::emitting,
        "amount", &rv::ParticleEmitterComponent::amount,

        "lifetime", & rv::ParticleEmitterComponent::lifetime,
        "oneShot", & rv::ParticleEmitterComponent::oneShot,
        "speedScale", & rv::ParticleEmitterComponent::speedScale,
        "explosiveness", & rv::ParticleEmitterComponent::explosiveness,
        "randomness", & rv::ParticleEmitterComponent::randomness,
        "lifetimeRandomness", & rv::ParticleEmitterComponent::lifetimeRandomness,

        "localCoords", & rv::ParticleEmitterComponent::localCoords,

        "emitterShape", & rv::ParticleEmitterComponent::emitterShape,
        "shapeDimensions", & rv::ParticleEmitterComponent::shapeDimensions,

        "direction", & rv::ParticleEmitterComponent::direction,
        "spread", & rv::ParticleEmitterComponent::spread,
        "gravity", & rv::ParticleEmitterComponent::gravity,

        "linearVelocityMin", & rv::ParticleEmitterComponent::linearVelocityMin,
        "linearVelocityMax", & rv::ParticleEmitterComponent::linearVelocityMax,
        "angularVelocityMin", & rv::ParticleEmitterComponent::angularVelocityMin,
        "angularVelocityMax", & rv::ParticleEmitterComponent::angularVelocityMax,
        "rotationMin", & rv::ParticleEmitterComponent::rotationMin,
        "rotationMax", & rv::ParticleEmitterComponent::rotationMax,

        "linearAccelMin", & rv::ParticleEmitterComponent::linearAccelMin,
        "linearAccelMax", & rv::ParticleEmitterComponent::linearAccelMax,

        "dampingMin", & rv::ParticleEmitterComponent::dampingMin,
        "dampingMax", & rv::ParticleEmitterComponent::dampingMax,

        "scaleMin", & rv::ParticleEmitterComponent::scaleMin,
        "scaleMax", & rv::ParticleEmitterComponent::scaleMax,
        "scaleEnd", & rv::ParticleEmitterComponent::scaleEnd,

        "startColor", & rv::ParticleEmitterComponent::startColor,
        "endColor", & rv::ParticleEmitterComponent::endColor
    );
}