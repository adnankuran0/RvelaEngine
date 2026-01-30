#pragma once
#include "Assets/AssetUUID.h"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

struct PrefabComponent
{
	PrefabComponent() = default;
	PrefabComponent(const AssetUUID& prefabID) { SetPrefabID(prefabID); }

	inline AssetUUID GetPrefabID() const noexcept { return prefabUUID; }
	inline void SetPrefabID(const AssetUUID& prefabID) { prefabUUID = prefabID; }

	json Serialize() const;
	void Deserialize(const json& j);
private:
	AssetUUID prefabUUID;
};