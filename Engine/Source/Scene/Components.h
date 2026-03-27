#pragma once

#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/TagComponent.h"
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/SceneTreeComponent.h"
#include "Scene/Components/UUIDComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/DirectionalLightComponent.h"
#include "Scene/Components/SpotLightComponent.h"
#include "Scene/Components/PrefabComponent.h"
#include "Scene/Components/ScriptComponent.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/CharacterBodyComponent.h"
#include "Scene/Components/ColliderComponents.h"
#include "Scene/Components/AudioEmitterComponent.h"

namespace rv {

enum class ComponentType
{
	Transform,
	Tag, 
	MeshRenderer,
	Mesh, 
	SceneTree, 
	UUID, 
	Material, 
	PointLight,
	DirectionalLight,
	SpotLight,
	Prefab,
	Script,
	Camera,
	Rigidbody,
	CharacterBody,
	BoxCollider,
	SphereCollider,
	CapsuleCollider,
	CylinderCollider,
	MeshCollider,
	ConvexHullCollider,
	AudioEmitterComponent
};

}