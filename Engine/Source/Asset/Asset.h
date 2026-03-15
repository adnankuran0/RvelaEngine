#pragma once
#include <memory>
#include "AssetUUID.h"

namespace rv {

enum class AssetState
{
	UNLOADED,
	LOADING,
	LOADED,
	FAILED
};

class Asset
{
public:
	Asset(AssetUUID uuid) { m_UUID = uuid; }
	virtual ~Asset() = default;

	Asset(const Asset& other) = delete;
	Asset& operator=(const Asset& other) = delete;

	const AssetUUID& GetUUID() const { return m_UUID; }
	const std::vector<AssetUUID>& GetDependencies() const { return m_Dependencies; }
	void AddDependency(const AssetUUID& uuid) { m_Dependencies.push_back(uuid); }


protected:
	void SetState(AssetState state) { m_State = state; }
private:
	AssetUUID m_UUID;
	AssetState m_State = AssetState::UNLOADED;
	std::vector<AssetUUID> m_Dependencies;

};



}