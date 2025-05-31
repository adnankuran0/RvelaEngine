#pragma once
#include "Scene/Components.h"
#include "assimp/scene.h"
#include <filesystem>
#include <Scene/BoundingBox.h>

namespace fs = std::filesystem;
struct MeshData
{
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	unsigned int indexCount;
	std::string name;
	Path materialPath;
	uint16_t meshIndex;
	BoundingBox localAABB;

};

class AssetManager
{
public:
	static std::vector<MeshData> LoadModel(const Path path);
	static MeshData LoadMesh(const Path modelPath, uint32_t meshIndex);
private:
	static void LoadMaterials(const aiScene* scene, std::unordered_map<unsigned int, std::string>& materials, const Path modelPath);
	static fs::path FindTexturePath(const Path modelPath, const aiString& texPath);
};