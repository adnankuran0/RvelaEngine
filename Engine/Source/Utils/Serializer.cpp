#include "rvelapch.h"
#include "Serializer.h"


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

template<typename T>
void SerializeBinComponent(const T& comp, ComponentType type, std::vector<std::byte>& out, auto writeFunc)
{
	std::vector<std::byte> tempOut;
	writeFunc(comp, tempOut);  

	ComponentHeader header;
	header.type = static_cast<uint16_t>(type);
	header.size = static_cast<uint32_t>(tempOut.size());

	WriteToBuffer(out, header);
	out.insert(out.end(), tempOut.begin(), tempOut.end());
}

void SerializeBin_DirectionalLightComp(const DirectionalLightComponent& comp, std::vector<std::byte>& out)
{
	std::vector<std::byte> tempOut;

	WriteToBuffer(tempOut, comp.color.x);
	WriteToBuffer(tempOut, comp.color.y);
	WriteToBuffer(tempOut, comp.color.z);
	WriteToBuffer(tempOut, comp.intensity);
	WriteToBuffer(tempOut, comp.shadowBias);
	WriteToBuffer(tempOut, comp.blurRadius);

	uint8_t castShadows = comp.castShadows ? 1 : 0;
	WriteToBuffer(tempOut, castShadows);
	uint8_t reverseCullFace = comp.reverseCullFace ? 1 : 0;
	WriteToBuffer(tempOut, reverseCullFace);

	size_t payloadSize = tempOut.size();

	ComponentHeader header;
	header.type = static_cast<uint16_t>(ComponentType::DirectionalLight);
	header.size = static_cast<uint32_t>(payloadSize);
	WriteToBuffer(out, header);
	out.insert(out.end(), tempOut.begin(), tempOut.end());
}
void DeserializeBin_DirectionalLightComp(const std::byte*& cursor, DirectionalLightComponent& comp)
{
	ReadFromBuffer(cursor, comp.color.x);
	ReadFromBuffer(cursor, comp.color.y);
	ReadFromBuffer(cursor, comp.color.z);
	ReadFromBuffer(cursor, comp.intensity);
	ReadFromBuffer(cursor, comp.shadowBias);
	ReadFromBuffer(cursor, comp.blurRadius);

	uint8_t castShadows, reverseCull;
	ReadFromBuffer(cursor, castShadows);
	ReadFromBuffer(cursor, reverseCull);
	comp.castShadows = (castShadows != 0);
	comp.reverseCullFace = (reverseCull != 0);
}
void SerializeJson_DirectionalLightComp(const DirectionalLightComponent& comp, json& j)
{
	j["color"] = { comp.color.r,comp.color.g,comp.color.b };
	j["intensity"] = comp.intensity;
	j["castShadows"] = comp.castShadows;
}

void SerializeBin_MaterialComp(const MaterialComponent& comp, std::vector<std::byte>& out)
{
	std::vector<std::byte> tempOut;

	const UUIDv4::UUID& raw = comp.GetMaterialID().Raw();
	WriteBytesToBuffer(tempOut, reinterpret_cast<const char*>(&raw), sizeof(UUIDv4::UUID));

	size_t payloadSize = tempOut.size();

	ComponentHeader header;
	header.type = static_cast<uint16_t>(ComponentType::Material);
	header.size = static_cast<uint32_t>(payloadSize);
	WriteToBuffer(out, header);
	out.insert(out.end(), tempOut.begin(), tempOut.end());
}
void DeserializeBin_MaterialComp(const std::byte*& cursor, MaterialComponent& comp)
{
	UUIDv4::UUID raw;
	ReadBytesFromBuffer(cursor, reinterpret_cast<char*>(&raw), sizeof(UUIDv4::UUID));
	comp.SetMaterial(AssetUUID(raw));
}
void SerializeJson_MaterialComp(const MaterialComponent& comp, json& j)
{
	j["materialID"] = comp.GetMaterialID().ToString();
}

void SerializeBin_MeshComp(const MeshComponent& comp, std::vector<std::byte>& out)
{
	std::vector<std::byte> tempOut;

	const UUIDv4::UUID& raw = comp.GetMeshID().Raw();
	WriteBytesToBuffer(tempOut, reinterpret_cast<const char*>(&raw), sizeof(UUIDv4::UUID));

	size_t payloadSize = tempOut.size();

	ComponentHeader header;
	header.type = static_cast<uint16_t>(ComponentType::Mesh);
	header.size = static_cast<uint32_t>(payloadSize);
	WriteToBuffer(out, header);
	out.insert(out.end(), tempOut.begin(), tempOut.end());
}
void DeserializeBin_MeshComp(const std::byte*& cursor, MeshComponent& comp)
{
	UUIDv4::UUID raw;
	ReadBytesFromBuffer(cursor, reinterpret_cast<char*>(&raw), 16);
	comp.SetMesh(AssetUUID(raw));
}
void SerializeJson_MeshComp(const MeshComponent& comp, json& j)
{
	j["meshID"] = comp.GetMeshID().ToString();
}

void SerializeBin_MeshRendererComp(const MeshRendererComponent& comp, std::vector<std::byte>& out)
{
	std::vector<std::byte> tempOut;

	uint8_t castShadow = comp.IsCastShadow() ? 1 : 0;
	WriteToBuffer(tempOut, castShadow);
	uint8_t doubleSided = comp.IsDoubleSided() ? 1 : 0;
	WriteToBuffer(tempOut, doubleSided);

	size_t payloadSize = tempOut.size();
	ComponentHeader header;
	header.type = static_cast<uint16_t>(ComponentType::MeshRenderer);
	header.size = static_cast<uint32_t>(payloadSize);
	WriteToBuffer(out, header);
	out.insert(out.end(), tempOut.begin(), tempOut.end());
}
void DeserializeBin_MeshRendererComp(const std::byte*& cursor, MeshRendererComponent& comp)
{
	uint8_t castShadow, doubleSided;
	ReadFromBuffer(cursor, castShadow);
	ReadFromBuffer(cursor, doubleSided);

	comp.SetCastShadow(castShadow != 0);
	comp.SetDoubleSided(doubleSided != 0);
}
void SerializeJson_MeshRendererComp(const MeshRendererComponent& comp, json& j)
{
	j["isCastShadow"] = comp.IsCastShadow();
	j["isDoubleSided"] = comp.IsDoubleSided();
}

void SerializeBin_PointLightComp(const PointLightComponent& comp, std::vector<std::byte>& out)
{
	std::vector<std::byte> tempOut;

	WriteToBuffer(tempOut, comp.color.x);
	WriteToBuffer(tempOut, comp.color.y);
	WriteToBuffer(tempOut, comp.color.z);
	WriteToBuffer(tempOut, comp.intensity);
	WriteToBuffer(tempOut, comp.radius);
	WriteToBuffer(tempOut, comp.falloff);
	WriteToBuffer(tempOut, comp.blurRadius);
	WriteToBuffer(tempOut, comp.shadowBias);
	uint8_t castShadows = comp.castShadows ? 1 : 0;
	WriteToBuffer(tempOut, castShadows);
	uint8_t reverseCullFace = comp.reverseCullFace ? 1 : 0;
	WriteToBuffer(tempOut, reverseCullFace);

	size_t payloadSize = tempOut.size();

	ComponentHeader header;
	header.type = static_cast<uint16_t>(ComponentType::PointLight);
	header.size = static_cast<uint32_t>(payloadSize);
	WriteToBuffer(out, header);
	out.insert(out.end(), tempOut.begin(), tempOut.end());
}
void DeserializeBin_PointLightComp(const std::byte*& cursor, PointLightComponent& comp)
{
	ReadFromBuffer(cursor, comp.color.x);
	ReadFromBuffer(cursor, comp.color.y);
	ReadFromBuffer(cursor, comp.color.z);
	ReadFromBuffer(cursor, comp.intensity);
	ReadFromBuffer(cursor, comp.radius);
	ReadFromBuffer(cursor, comp.falloff);
	ReadFromBuffer(cursor, comp.blurRadius);
	ReadFromBuffer(cursor, comp.shadowBias);

	uint8_t castShadows, reverseCull;
	ReadFromBuffer(cursor, castShadows);
	ReadFromBuffer(cursor, reverseCull);
	comp.castShadows = (castShadows != 0);
	comp.reverseCullFace = (reverseCull != 0);
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

void SerializeBin_SceneTreeComp(const SceneTreeComponent& comp, std::vector<std::byte>& out)
{
	std::vector<std::byte> tempOut;

	WriteToBuffer(tempOut, comp.parentUUID);

	size_t payloadSize = tempOut.size();

	ComponentHeader header;
	header.type = static_cast<uint16_t>(ComponentType::SceneTree);
	header.size = static_cast<uint32_t>(payloadSize);
	WriteToBuffer(out, header);
	out.insert(out.end(), tempOut.begin(), tempOut.end());
}
void DeserializeBin_SceneTreeComp(const std::byte*& cursor, SceneTreeComponent& comp)
{
	ReadFromBuffer(cursor, comp.parentUUID);
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

void SerializeBin_TagComp(const TagComponent& comp, std::vector<std::byte>& out)
{
	std::vector<std::byte> tempOut;

	uint32_t len = static_cast<uint32_t>(comp.tag.size());
	WriteToBuffer(tempOut, len);
	WriteBytesToBuffer(tempOut, comp.tag.data(), len);

	size_t payloadSize = tempOut.size();

	ComponentHeader header;
	header.type = static_cast<uint16_t>(ComponentType::Tag);
	header.size = static_cast<uint32_t>(payloadSize);
	WriteToBuffer(out, header);
	out.insert(out.end(), tempOut.begin(), tempOut.end());
}
void DeserializeBin_TagComp(const std::byte*& cursor, TagComponent& comp)
{
	uint32_t len;
	ReadFromBuffer(cursor, len);

	comp.tag.resize(len);
	ReadBytesFromBuffer(cursor, comp.tag.data(), len);
}
void SerializeJson_TagComp(const TagComponent& comp, json& j)
{
	j["tag"] = comp.tag;
}

void SerializeBin_TransformComp(const TransformComponent& comp, std::vector<std::byte>& out)
{

	glm::vec3 pos = comp.GetPosition();
	glm::vec3 euler = comp.GetEulerRotation();
	glm::vec3 scale = comp.GetScale();
	uint8_t lockScale = comp.IsScaleRatioLocked() ? 1 : 0;


	std::vector<std::byte> tempOut;

	WriteToBuffer(tempOut, pos.x);   WriteToBuffer(tempOut, pos.y);	  WriteToBuffer(tempOut, pos.z);
	WriteToBuffer(tempOut, euler.x); WriteToBuffer(tempOut, euler.y); WriteToBuffer(tempOut, euler.z);
	WriteToBuffer(tempOut, scale.x); WriteToBuffer(tempOut, scale.y); WriteToBuffer(tempOut, scale.z);
	WriteToBuffer(tempOut, lockScale);

	size_t payloadSize = tempOut.size();

	ComponentHeader header;
	header.type = static_cast<uint16_t>(ComponentType::Transform);
	header.size = static_cast<uint32_t>(payloadSize);
	WriteToBuffer(out, header);
	out.insert(out.end(), tempOut.begin(), tempOut.end());
}
void DeserializeBin_TransformComp(const std::byte*& cursor, TransformComponent& comp)
{
	glm::vec3 pos, euler, scale;
	uint8_t lockScale;

	ReadFromBuffer(cursor, pos.x); ReadFromBuffer(cursor, pos.y); ReadFromBuffer(cursor, pos.z);
	ReadFromBuffer(cursor, euler.x); ReadFromBuffer(cursor, euler.y); ReadFromBuffer(cursor, euler.z);
	ReadFromBuffer(cursor, scale.x); ReadFromBuffer(cursor, scale.y); ReadFromBuffer(cursor, scale.z);
	ReadFromBuffer(cursor, lockScale);

	comp.SetPosition(pos);
	comp.SetEulerRotation(euler);
	comp.SetScale(scale);
	comp.SetLockScaleRatio(lockScale != 0);
	comp.SetDirty();
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

void SerializeBin_UUIDComp(const UUIDComponent& comp, std::vector<std::byte>& out)
{
	std::vector<std::byte> tempOut;

	WriteToBuffer(tempOut, comp.uuid);

	size_t payloadSize = tempOut.size();

	ComponentHeader header;
	header.type = static_cast<uint16_t>(ComponentType::UUID);
	header.size = static_cast<uint32_t>(payloadSize);
	WriteToBuffer(out, header);
	out.insert(out.end(), tempOut.begin(), tempOut.end());
}
void DeserializeBin_UUIDComp(const std::byte*& cursor, UUIDComponent& comp)
{
	ReadFromBuffer(cursor, comp.uuid);
}
void SerializeJson_UUIDComp(const UUIDComponent& comp, json& j)
{
	j["uuid"] = comp.uuid;
}

