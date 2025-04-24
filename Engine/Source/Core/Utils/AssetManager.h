#pragma once
#include "Scene/Components.h"
#include "assimp/scene.h"
#include <filesystem>
namespace fs = std::filesystem;
struct MeshData
{
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	unsigned int indexCount;
	std::string name;
	std::string materialPath;
	uint16_t meshIndex;
};

class AssetManager
{
public:
	static std::vector<MeshData> LoadModel(const std::string& path);
	static MeshData LoadMesh(const std::string& modelPath, uint32_t meshIndex);
private:
	static void LoadMaterials(const aiScene* scene, std::unordered_map<unsigned int, std::string>& materials, const std::string& modelPath);
	static fs::path FindTexturePath(const std::string& modelPath, const aiString& texPath);
};