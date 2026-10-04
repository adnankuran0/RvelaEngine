#pragma once
#include "Asset/AssetUUID.h"
#include "../nlohmann/json.hpp"

namespace rv {

using json = nlohmann::json;

struct PrefabComponent
{
	PrefabComponent() = default;
	PrefabComponent(const AssetUUID& prefabID) : prefabUUID(prefabID) {}
	PrefabComponent(const AssetUUID& prefabID, const json& overrides) : prefabUUID(prefabID), m_Overrides(overrides) {}

	inline AssetUUID GetPrefabID() const noexcept { return prefabUUID; }
	inline void SetPrefabID(const AssetUUID& prefabID) { prefabUUID = prefabID; }

	inline const json& GetOverrides() const noexcept { return m_Overrides; }
	inline void SetOverrides(const json& overrides) { m_Overrides = overrides; }
	inline bool HasOverrides() const noexcept { return m_Overrides.is_array() && !m_Overrides.empty(); }
	inline void ClearOverrides() { m_Overrides = json::array(); }

	json Serialize() const;
	void Deserialize(const json& j);
private:
	AssetUUID prefabUUID;
	json m_Overrides = json::array();
};

}