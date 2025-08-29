#pragma once
#include "Utils/ISerializable.h"
#include "Scene/Components.h"
#include "glm/glm.hpp"
#include <cstddef>
#include <vector>
#include <string>
#include <type_traits>

class Serializer
{
public:
	static void SaveToFile(const ISerializable& obj, const std::string& path);
	static void LoadFromFile(ISerializable& obj, const std::string& path);
};


template<typename T>
inline void WriteToBuffer(std::vector<std::byte>& buffer, const T& value)
{
	static_assert(std::is_trivially_copyable_v<T>, "WriteToBuffer requires trivially copyable types");
	size_t oldSize = buffer.size();
	buffer.resize(oldSize + sizeof(T));
	std::memcpy(buffer.data() + oldSize, &value, sizeof(T));
}

inline void WriteBytesToBuffer(std::vector<std::byte>& buffer, const char* data, size_t size)
{
	size_t oldSize = buffer.size();
	buffer.resize(oldSize + size);
	std::memcpy(buffer.data() + oldSize, data, size);
}

template<typename T>
inline void ReadFromBuffer(const std::byte*& cursor, T& value)
{
	std::memcpy(&value, cursor, sizeof(T));
	cursor += sizeof(T);
}

inline void ReadBytesFromBuffer(const std::byte*& cursor, char* outData, size_t size)
{
	std::memcpy(outData, cursor, size);
	cursor += size;
}

#pragma pack(push, 1) 
struct ComponentHeader
{
	uint16_t type;
	uint32_t size;
};
#pragma pack(pop)

void SerializeBin_DirectionalLightComp(const DirectionalLightComponent& comp, std::vector<std::byte>& out);
void DeserializeBin_DirectionalLightComp(const std::byte*& cursor, DirectionalLightComponent& comp);
void SerializeJson_DirectionalLightComp(const DirectionalLightComponent& comp, json& j);

void SerializeBin_MaterialComp(const MaterialComponent& comp, std::vector<std::byte>& out);
void DeserializeBin_MaterialComp(const std::byte*& cursor, MaterialComponent& comp);
void SerializeJson_MaterialComp(const MaterialComponent& comp, json& j);

void SerializeBin_MeshComp(const MeshComponent& comp, std::vector<std::byte>& out);
void DeserializeBin_MeshComp(const std::byte*& cursor, MeshComponent& comp);
void SerializeJson_MeshComp(const MeshComponent& comp, json& j);

void SerializeBin_MeshRendererComp(const MeshRendererComponent& comp, std::vector<std::byte>& out);
void DeserializeBin_MeshRendererComp(const std::byte*& cursor, MeshRendererComponent& comp);
void SerializeJson_MeshRendererComp(const MeshRendererComponent& comp, json& j);

void SerializeBin_PointLightComp(const PointLightComponent& comp, std::vector<std::byte>& out);
void DeserializeBin_PointLightComp(const std::byte*& cursor, PointLightComponent& comp);
void SerializeJson_PointLightComp(const PointLightComponent& comp, json& j);

void SerializeBin_SceneTreeComp(const SceneTreeComponent& comp, std::vector<std::byte>& out);
void DeserializeBin_SceneTreeComp(const std::byte*& cursor, SceneTreeComponent& comp);
void SerializeJson_SceneTreeComp(const SceneTreeComponent& comp, json& j);

void SerializeBin_SpotLightComp(const SpotLightComponent& comp, std::ostream& out);
void SerializeJson_SpotLightComp(const SpotLightComponent& comp);

void SerializeBin_TagComp(const TagComponent& comp, std::vector<std::byte>& out);
void DeserializeBin_TagComp(const std::byte*& cursor, TagComponent& comp);
void SerializeJson_TagComp(const TagComponent& comp, json& j);

void SerializeBin_TransformComp(const TransformComponent& comp, std::vector<std::byte>& out);
void DeserializeBin_TransformComp(const std::byte*& cursor, TransformComponent& comp);
void SerializeJson_TransformComp(const TransformComponent& comp, json& j);

void SerializeBin_UUIDComp(const UUIDComponent& comp, std::vector<std::byte>& out);
void DeserializeBin_UUIDComp(const std::byte*& cursor, UUIDComponent& comp);
void SerializeJson_UUIDComp(const UUIDComponent& comp, json& j);


