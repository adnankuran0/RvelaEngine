#pragma once
#include "Assets/Asset.h"
#include "TextureImporter.h"
#include "ModelImporter.h"

class AssetImporterRegistry
{
public:
	AssetImporterRegistry()
	{
		RegisterImporter({ ".png", ".jpg", ".jpeg", ".tga" }, std::make_shared<TextureImporter>());
		RegisterImporter({ ".gltf", ".fbx", ".obj", ".glb" }, std::make_shared<ModelImporter>());
	}

	AssetUUID Import(const std::filesystem::path& assetPath)
	{
		std::string extension = assetPath.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); 
		auto it = m_ImporterMap.find(extension);
		if (it != m_ImporterMap.end())
		{
			AssetUUID uuid = it->second->Import(assetPath);
			AssetRegistry::ScanAssets();
			return uuid;
		}
		return AssetUUID::Invalid();
	}

private:
	AssetImporterRegistry(const AssetImporterRegistry&) = delete;
	AssetImporterRegistry& operator=(const AssetImporterRegistry&) = delete;

	void RegisterImporter(const std::vector<std::string>& extensions, std::shared_ptr<IAssetImporter> importer)
	{
		for (const auto& ext : extensions)
			m_ImporterMap[ext] = importer;
	}

private:
	std::unordered_map<std::string, std::shared_ptr<IAssetImporter>> m_ImporterMap;
};
