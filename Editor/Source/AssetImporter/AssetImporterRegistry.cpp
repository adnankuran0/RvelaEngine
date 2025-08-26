#include "AssetImporterRegistry.h"
#include <Assets/AssetRegistry.h>

AssetImporterRegistry::AssetImporterRegistry()
{
	RegisterImporter({ ".png", ".jpg", ".jpeg", ".tga" }, std::make_shared<TextureImporter>());
	RegisterImporter({ ".gltf", ".fbx", ".obj", ".glb" }, std::make_shared<ModelImporter>());
}

AssetUUID AssetImporterRegistry::Import(const std::filesystem::path& assetPath) noexcept
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