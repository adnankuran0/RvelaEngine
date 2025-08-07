#pragma once
#include "AssetImporter.h"
#include "Assets/MeshAsset.h"
#include "Assets/Asset.h"
#include "Assimp/Importer.hpp"
#include "Assimp/scene.h"

class MeshImporter : public IAssetImporter
{
public:
	bool Import(const std::filesystem::path& path) override;
private:
	const aiScene* LoadScene(const std::filesystem::path& path);
	std::vector<Vertex> ProcessVertices(aiMesh* mesh);
	std::vector<unsigned int> ProcessIndices(aiMesh* mesh);
	MeshMeta CreateMeshMeta(const std::filesystem::path& path, aiMesh* mesh);
	Assimp::Importer m_Importer;
};
