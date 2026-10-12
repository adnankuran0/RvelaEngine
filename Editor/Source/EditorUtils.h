#pragma once
#include "Asset/AssetUUID.h"
#include <vector>
#include <string>

namespace rv {

class Engine;

class EditorUtils
{
public:
	static void CreateScene(Engine& engine);
	static void OpenScene(Engine& engine);
	static bool SaveScene(Engine& engine);
	static bool SaveSceneAs(Engine& engine);
	static void OpenVSCodeWorkspace();
	static AssetUUID ReadUUIDFromMeta(const std::string& assetPath);
	static std::string GetAssetFileName(const AssetUUID& uuid);
	static void ClearAssetFileNameCache();

	struct MeshAssetInfo
	{
		AssetUUID uuid;
		std::string modelName;
		std::string meshName;
		std::string fullName;
	};
	static std::vector<MeshAssetInfo> GetAvailableMeshAssets(bool skeletal = false);

	struct MaterialAssetInfo
	{
		AssetUUID uuid;
		std::string name;
		std::string path;
	};
	static std::vector<MaterialAssetInfo> GetAvailableMaterialAssets();
};

}
