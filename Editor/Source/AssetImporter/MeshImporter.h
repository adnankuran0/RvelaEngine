#pragma once
#include "AssetImporter.h"
#include "Assets/MeshAsset.h"
#include "Assets/Asset.h"
#include "Assimp/Importer.hpp"
#include "Assimp/scene.h"

class MeshImporter 
{
public:
	bool Import(const aiScene* scene, const std::filesystem::path& path, std::unordered_map<unsigned int, AssetUUID>& meshMap);
private:
	std::vector<Vertex> ProcessVertices(aiMesh* mesh);
	std::vector<unsigned int> ProcessIndices(aiMesh* mesh);
	MeshMeta CreateMeshMeta(const std::filesystem::path& path, aiMesh* mesh);
};
