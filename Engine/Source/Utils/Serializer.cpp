#include "rvelapch.h"
#include "Serializer.h"
#include "Scene/Components.h"
#include "glm/glm.hpp"

void Serializer::SaveToFile(const ISerializable& obj, const std::string& path)
{
	try
	{
		std::filesystem::create_directories(std::filesystem::path(path).parent_path());

		std::ofstream file(path);
		if (!file.is_open())
		{
			LOG_ERROR("File can not be opened: {}", path);
			return;
		}

		std::string data = obj.Serialize();
		file << data;
		file.close();
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Serializer::SaveToFile error: {}", e.what());
	}
}
void Serializer::LoadFromFile(ISerializable& obj, const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		LOG_ERROR("File can not be opened: {}", path);
	}
	std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	obj.Deserialize(str.c_str());
}

void SerializeBin_DirectionalLightComp(const DirectionalLightComponent& comp, std::ostream& out)
{
	out.write(reinterpret_cast<const char*>(&comp.color), sizeof(glm::vec3));
	out.write(reinterpret_cast<const char*>(&comp.intensity), sizeof(float));
	out.write(reinterpret_cast<const char*>(&comp.shadowBias), sizeof(float));
	out.write(reinterpret_cast<const char*>(&comp.blurRadius), sizeof(float));
	out.write(reinterpret_cast<const char*>(&comp.castShadows), sizeof(bool));
	out.write(reinterpret_cast<const char*>(&comp.reverseCullFace), sizeof(bool));
}
void SerializeJson_DirectionalLightComp(const DirectionalLightComponent& comp, json& j)
{
	j["color"] = { comp.color.r,comp.color.g,comp.color.b };
	j["intensity"] = comp.intensity;
	j["castShadows"] = comp.castShadows;
}

void SerializeBin_MaterialComp(const MaterialComponent& comp, std::ostream& out)
{

}
void SerializeJson_MaterialComp(const MaterialComponent& comp)
{

}

void SerializeBin_MeshComp(const MeshComponent& comp, std::ostream& out)
{

}
void SerializeJson_MeshComp(const MeshComponent& comp)
{

}

void SerializeBin_MeshRendererComp(const MeshRendererComponent& comp, std::ostream& out)
{

}
void SerializeJson_MeshRendererComp(const MeshRendererComponent& comp)
{

}

void SerializeBin_PointLightComp(const PointLightComponent& comp, std::ostream& out)
{

}
void SerializeJson_PointLightComp(const PointLightComponent& comp)
{

}

void SerializeBin_SceneTreeComp(const SceneTreeComponent& comp, std::ostream& out)
{

}
void SerializeJson_SceneTreeComp(const SceneTreeComponent& comp)
{

}

void SerializeBin_SpotLightComp(const SpotLightComponent& comp, std::ostream& out)
{

}
void SerializeJson_SpotLightComp(const SpotLightComponent& comp)
{

}

void SerializeBin_TagComp(const TagComponent& comp, std::ostream& out)
{

}
void SerializeJson_TagComp(const TagComponent& comp)
{

}

void SerializeBin_TransformComp(const TransformComponent& comp, std::ostream& out)
{
	glm::vec3 pos = comp.GetPosition();
	glm::vec3 euler = comp.GetEulerRotation();
	glm::vec3 scale = comp.GetScale();
	bool lockScale = comp.IsScaleRatioLocked();

	out.write(reinterpret_cast<const char*>(&pos), sizeof(glm::vec3));
	out.write(reinterpret_cast<const char*>(&euler), sizeof(glm::vec3));
	out.write(reinterpret_cast<const char*>(&scale), sizeof(glm::vec3));
	out.write(reinterpret_cast<const char*>(&lockScale), sizeof(bool));
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

void SerializeBin_UUIDComp(const UUIDComponent& comp, std::ostream& out)
{

}
void SerializeJson_UUIDComp(const UUIDComponent& comp)
{

}