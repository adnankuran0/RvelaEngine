#pragma once
#include "Assets/AssetUUID.h"

struct PrefabComponent
{
	PrefabComponent() = default;
	PrefabComponent(const AssetUUID& prefabID) { SetPrefabID(prefabID); }

	inline AssetUUID GetPrefabID() const noexcept { return prefabUUID; }
	inline void SetPrefabID(const AssetUUID& prefabID) { prefabUUID = prefabID; }
private:
	AssetUUID prefabUUID;
};