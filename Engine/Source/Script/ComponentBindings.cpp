#include "rvelapch.h"
#include "ComponentBindings.h"
#include "Scene/Entity.h"

using namespace rv;

void LuaBindings::RegisterComponents(sol::state& lua)
{
    lua.new_usertype<TransformComponent>("TransformComponent",
        "GetPosition", &TransformComponent::GetPosition,
        "SetPosition", &TransformComponent::SetPosition,
        "GetWorldPosition", &TransformComponent::GetWorldPosition,
        "Translate", &TransformComponent::Translate,
        "GetRotation", &TransformComponent::GetRotation,
        "GetWorldRotation", &TransformComponent::GetWorldRotation,
        "SetRotation", &TransformComponent::SetRotation,
        "GetEulerRotation", &TransformComponent::GetEulerRotation,
        "SetEulerRotation", &TransformComponent::SetEulerRotation,
        "GetScale", &TransformComponent::GetScale,
        "GetWorldScale", &TransformComponent::GetWorldScale,
        "SetScale", &TransformComponent::SetScale,
        "GetForward", &TransformComponent::GetForward,
        "GetUp", &TransformComponent::GetUp,
        "GetRight", &TransformComponent::GetRight,
        "LookAt", &TransformComponent::LookAt
    );

    lua.new_usertype<CameraComponent>("CameraComponent",
        "GetFOV", [](CameraComponent& c) { return (double)c.GetFOV(); },
        "SetFOV", &CameraComponent::SetFOV
    );

    lua.new_usertype<rv::DirectionalLightComponent>("DirectionalLightComponent",
        "color", &rv::DirectionalLightComponent::color,
        "intensity", &rv::DirectionalLightComponent::intensity,
        "shadowBias", &rv::DirectionalLightComponent::shadowBias,
        "blurRadius", &rv::DirectionalLightComponent::blurRadius,
        "castShadows", &rv::DirectionalLightComponent::castShadows,
        "reverseCullFace", &rv::DirectionalLightComponent::reverseCullFace
    );

    // TODO:
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
        "IsCastShadow", &rv::MeshRendererComponent::IsCastShadow,
        "SetCastShadow", &rv::MeshRendererComponent::SetCastShadow,
        "IsDoubleSided", &rv::MeshRendererComponent::IsDoubleSided,
        "SetDoubleSided", &rv::MeshRendererComponent::SetDoubleSided
    );

    lua.new_usertype<rv::MaterialComponent>("MaterialComponent",
        "GetAlbedoColor", &rv::MaterialComponent::GetAlbedoColor,
        "SetAlbedoColor", &rv::MaterialComponent::SetAlbedoColor,
        "GetEmmisiveColor", &rv::MaterialComponent::GetEmmisiveColor,
        "SetEmmisiveColor", &rv::MaterialComponent::SetEmmisiveColor,
        "GetEmmisiveIntensity", &rv::MaterialComponent::GetEmmisiveIntensity,
        "SetEmmisiveIntensity", &rv::MaterialComponent::SetEmmisiveIntensity,

        "GetMetallic", &rv::MaterialComponent::GetMetallic,
        "SetMetallic", &rv::MaterialComponent::SetMetallic,
        "GetSpecular", &rv::MaterialComponent::GetSpecular,
        "SetSpecular", &rv::MaterialComponent::SetSpecular,
        "GetRoughness", &rv::MaterialComponent::GetRoughness,
        "SetRoughness", &rv::MaterialComponent::SetRoughness,
        "GetAO", &rv::MaterialComponent::GetAO,
        "SetAO", &rv::MaterialComponent::SetAO,
        "GetNormalScale", &rv::MaterialComponent::GetNormalScale,
        "SetNormalScale", &rv::MaterialComponent::SetNormalScale,
        "GetHeightScale", &rv::MaterialComponent::GetHeightScale,
        "SetHeightScale", &rv::MaterialComponent::SetHeightScale,
        "GetUVScale", &rv::MaterialComponent::GetUVScale,
        "SetUVScale", &rv::MaterialComponent::SetUVScale,
        "GetUVOffset", &rv::MaterialComponent::GetUVOffset,
        "SetUVOffset", &rv::MaterialComponent::SetUVOffset
    );
}