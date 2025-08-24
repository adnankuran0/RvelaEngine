#pragma once
#include "Utils/ISerializable.h"
#include "Scene/Components.h"
#include "glm/glm.hpp"

class Serializer
{
public:
	static void SaveToFile(const ISerializable& obj, const std::string& path);
	static void LoadFromFile(ISerializable& obj, const std::string& path);
};

template<typename T>
inline void WriteToBuffer(std::vector<uint8_t>& buffer, const T& value)
{
	const uint8_t* data = reinterpret_cast<const uint8_t*>(&value);
	buffer.insert(buffer.end(), data, data + sizeof(T));
}

inline void WriteBytesToBuffer(std::vector<uint8_t>& buffer, const char* data, size_t size)
{
	buffer.insert(buffer.end(), data, data + size);
}

void SerializeBin_DirectionalLightComp(const DirectionalLightComponent& comp, std::vector<uint8_t>& out)
{
	out.push_back(static_cast<uint8_t>(ComponentType::DirectionalLight));
	out.push_back(static_cast<uint8_t>(sizeof(comp)));

	WriteToBuffer(out, comp.color.x);
	WriteToBuffer(out, comp.color.y);
	WriteToBuffer(out, comp.color.z);
	WriteToBuffer(out, comp.intensity);
	WriteToBuffer(out, comp.shadowBias);
	WriteToBuffer(out, comp.blurRadius);
	WriteToBuffer(out, comp.castShadows);
	WriteToBuffer(out, comp.reverseCullFace);
}
void SerializeJson_DirectionalLightComp(const DirectionalLightComponent& comp, json& j)
{
	j["color"] = { comp.color.r,comp.color.g,comp.color.b };
	j["intensity"] = comp.intensity;
	j["castShadows"] = comp.castShadows;
}

void SerializeBin_MaterialComp(const MaterialComponent& comp, std::vector<uint8_t>& out)
{
	out.push_back(static_cast<uint8_t>(ComponentType::Material));
	out.push_back(static_cast<uint8_t>(sizeof(comp)));

	WriteToBuffer(out, comp.GetMaterialID());
}
void SerializeJson_MaterialComp(const MaterialComponent& comp, json& j)
{
	j["materialID"] = comp.GetMaterialID().ToString();
}

void SerializeBin_MeshComp(const MeshComponent& comp, std::vector<uint8_t>& out)
{
	out.push_back(static_cast<uint8_t>(ComponentType::Mesh));
	out.push_back(static_cast<uint8_t>(sizeof(comp)));

	WriteToBuffer(out, comp.GetMeshID());
}
void SerializeJson_MeshComp(const MeshComponent& comp, json& j)
{
	j["meshID"] = comp.GetMeshID().ToString();
}

void SerializeBin_MeshRendererComp(const MeshRendererComponent& comp, std::vector<uint8_t>& out)
{
	out.push_back(static_cast<uint8_t>(ComponentType::MeshRenderer));
	out.push_back(static_cast<uint8_t>(sizeof(comp)));

	bool castShadow = comp.IsCastShadow();
	bool doubleSided = comp.IsDoubleSided();
	WriteToBuffer(out, castShadow);
	WriteToBuffer(out, doubleSided);
}
void SerializeJson_MeshRendererComp(const MeshRendererComponent& comp, json& j)
{
	j["isCastShadow"] = comp.IsCastShadow();
	j["isDoubleSided"] = comp.IsDoubleSided();
}

void SerializeBin_PointLightComp(const PointLightComponent& comp, std::vector<uint8_t>& out)
{
	out.push_back(static_cast<uint8_t>(ComponentType::PointLight));
	out.push_back(static_cast<uint8_t>(sizeof(comp)));

	WriteToBuffer(out, comp.color.x);
	WriteToBuffer(out, comp.color.y);
	WriteToBuffer(out, comp.color.z);
	WriteToBuffer(out, comp.intensity);
	WriteToBuffer(out, comp.radius);
	WriteToBuffer(out, comp.falloff);
	WriteToBuffer(out, comp.blurRadius);
	WriteToBuffer(out, comp.shadowBias);
	WriteToBuffer(out, comp.castShadows);
	WriteToBuffer(out, comp.reverseCullFace);
}
void SerializeJson_PointLightComp(const PointLightComponent& comp, json& j)
{
	j["color"] = { comp.color.r, comp.color.g, comp.color.b };
	j["intensity"] = comp.intensity;
	j["radius"] = comp.radius;
	j["falloff"] = comp.falloff;
	j["blurRadius"] = comp.blurRadius;
	j["shadowBias"] = comp.shadowBias;
	j["castShadows"] = comp.castShadows;
	j["reverseCullFace"] = comp.reverseCullFace;
}

void SerializeBin_SceneTreeComp(const SceneTreeComponent& comp, std::vector<uint8_t>& out)
{
	out.push_back(static_cast<uint8_t>(ComponentType::SceneTree));

	WriteToBuffer(out, comp.parentUUID);
}

void SerializeJson_SceneTreeComp(const SceneTreeComponent& comp, json& j)
{
	j["parentUUID"] = comp.parentUUID;
	j["childrenUUIDs"] = comp.childrenUUIDs;
}

void SerializeBin_SpotLightComp(const SpotLightComponent& comp, std::ostream& out)
{

}
void SerializeJson_SpotLightComp(const SpotLightComponent& comp)
{

}

void SerializeBin_TagComp(const TagComponent& comp, std::vector<uint8_t>& out)
{
	out.push_back(static_cast<uint8_t>(ComponentType::Tag));
	out.push_back(static_cast<uint8_t>(sizeof(comp)));

	size_t len = comp.tag.size();
	WriteToBuffer(out, len);
	WriteBytesToBuffer(out, comp.tag.data(), len);
}
void SerializeJson_TagComp(const TagComponent& comp, json& j)
{
	j["tag"] = comp.tag;
}

void SerializeBin_TransformComp(const TransformComponent& comp, std::vector<uint8_t>& out)
{
	out.push_back(static_cast<uint8_t>(ComponentType::Transform));
	out.push_back(static_cast<uint8_t>(sizeof(comp)));

	glm::vec3 pos = comp.GetPosition();
	glm::vec3 euler = comp.GetEulerRotation();
	glm::vec3 scale = comp.GetScale();
	bool lockScale = comp.IsScaleRatioLocked();

	WriteToBuffer(out, pos.x); WriteToBuffer(out, pos.y); WriteToBuffer(out, pos.z);
	WriteToBuffer(out, euler.x); WriteToBuffer(out, euler.y); WriteToBuffer(out, euler.z);
	WriteToBuffer(out, scale.x); WriteToBuffer(out, scale.y); WriteToBuffer(out, scale.z);
	WriteToBuffer(out, lockScale);
}
void SerializeJson_TransformComp(const TransformComponent& comp, json& j)
{
	glm::vec3 pos = comp.GetPosition();
	glm::vec3 euler = comp.GetEulerRotation();
	glm::vec3 scale = comp.GetScale();
	bool lockScale = comp.IsScaleRatioLocked();

	j["position"] = { pos.x, pos.y, pos.z };
	j["rotation"] = { euler.x, euler.y, euler.z };
	j["scale"] = { scale.x, scale.y, scale.z };
	j["lockScaleRatio"] = lockScale;
}

void SerializeBin_UUIDComp(const UUIDComponent& comp, std::vector<uint8_t>& out)
{
	out.push_back(static_cast<uint8_t>(ComponentType::UUID));
	out.push_back(static_cast<uint8_t>(sizeof(comp)));

	WriteToBuffer(out, comp.uuid);
}
void SerializeJson_UUIDComp(const UUIDComponent& comp, json& j)
{
	j["uuid"] = comp.uuid;
}