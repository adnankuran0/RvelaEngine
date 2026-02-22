#include "rvelapch.h"
#include "Asset.h"
#include "AssetUUID.h"

using namespace rv;

Asset::Asset(std::unique_ptr<AssetMeta> assetMeta)
{
	m_Meta = std::move(assetMeta);
}

AssetUUID Asset::GetUUID() const
{
	return m_Meta.get()->uuid;
}

void Asset::SetUUID(const AssetUUID& uuid)
{ 
	m_Meta.get()->uuid = uuid; 
}

AssetType Asset::GetAssetType() const
{ 
	return m_Meta.get()->type; 
}

