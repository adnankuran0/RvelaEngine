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

struct ComponentHeader
{
	uint16_t type;
	uint32_t size;
};

void SerializeBin_DirectionalLightComp(const DirectionalLightComponent& comp, std::vector<std::byte>& out);
void SerializeJson_DirectionalLightComp(const DirectionalLightComponent& comp, json& j);

void SerializeBin_MaterialComp(const MaterialComponent& comp, std::vector<std::byte>& out);
void SerializeJson_MaterialComp(const MaterialComponent& comp, json& j);

void SerializeBin_MeshComp(const MeshComponent& comp, std::vector<std::byte>& out);
void SerializeJson_MeshComp(const MeshComponent& comp, json& j);

void SerializeBin_MeshRendererComp(const MeshRendererComponent& comp, std::vector<std::byte>& out);
void SerializeJson_MeshRendererComp(const MeshRendererComponent& comp, json& j);

void SerializeBin_PointLightComp(const PointLightComponent& comp, std::vector<std::byte>& out);
void SerializeJson_PointLightComp(const PointLightComponent& comp, json& j);

void SerializeBin_SceneTreeComp(const SceneTreeComponent& comp, std::vector<std::byte>& out);
void SerializeJson_SceneTreeComp(const SceneTreeComponent& comp, json& j);

void SerializeBin_SpotLightComp(const SpotLightComponent& comp, std::ostream& out);
void SerializeJson_SpotLightComp(const SpotLightComponent& comp);

void SerializeBin_TagComp(const TagComponent& comp, std::vector<std::byte>& out);
void SerializeJson_TagComp(const TagComponent& comp, json& j);

void SerializeBin_TransformComp(const TransformComponent& comp, std::vector<std::byte>& out);
void SerializeJson_TransformComp(const TransformComponent& comp, json& j);

void SerializeBin_UUIDComp(const UUIDComponent& comp, std::vector<std::byte>& out);
void SerializeJson_UUIDComp(const UUIDComponent& comp, json& j);


