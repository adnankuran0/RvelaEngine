#pragma once
#include "Asset/AssetUUID.h"
#include "Scene/Entity.h"
#include <vector>
#include <string>
#include <filesystem>

namespace rv {

class Engine;
class Scene;

class EditorUtils
{
public:
	static void CreateScene(Engine& engine);
	static void OpenScene(Engine& engine);
	static bool SaveScene(Engine& engine);
	static bool SaveSceneAs(Engine& engine);
	static void OpenVSCodeWorkspace();
	static std::filesystem::path FindRuntimeExecutable();
	static bool LaunchRuntime(const std::filesystem::path& projectPath, std::string* outError = nullptr);
	static bool LaunchActiveProject(Engine& engine, std::string* outError = nullptr);
	static AssetUUID ReadUUIDFromMeta(const std::string& assetPath);
	static std::string GetAssetFileName(const AssetUUID& uuid);
	static void ClearAssetFileNameCache();
	static void AlignSelectedWithView(Engine& engine);
	static void SnapSelectedToFloor(Engine& engine, bool alignWithNormal = false);
	static bool HasTransformSelection(Engine& engine);
	static Entity LoadPrimitive(Scene& scene, const std::string& primitiveMeshName);

	static uint32_t BakeStaticShadows(Scene& scene);
	static uint32_t GetStaticShadowLightCount(Scene& scene);

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
