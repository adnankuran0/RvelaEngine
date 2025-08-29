#pragma once
#include "AssetImporter.h"
#include "MeshImporter.h"
#include "Assimp/Importer.hpp"
#include "Assimp/scene.h"
#include <assimp/postprocess.h>
#include "TextureImporter.h"
#include "MaterialImporter.h"
#include "entt/entt.h"
#include <Scene/Scene.h>
#include "Scene/Entity.h"


class ModelImporter : public IAssetImporter
{
	AssetUUID Import(const std::filesystem::path& path) override;
private:
	const aiScene* LoadScene(const std::filesystem::path& path);
	void ExtractTextures(const aiScene* scene, const std::filesystem::path& modelPath);
	void ExtractMaterials(const aiScene* scene, const std::filesystem::path& modelPath);
	AssetUUID ConstructPrefab(const aiScene* scene, const std::filesystem::path& modelPath);
	entt::entity ProcessNode(aiNode* node, const aiScene* modelScene, entt::entity parent, Scene& scene);
	void AttachMeshToEntity(unsigned int meshIndex, aiNode* node, const aiScene* modelScene, Scene& scene,entt::entity);
	void SetTransformForEntity(aiNode* node, Scene& scene, entt::entity e);

	inline std::filesystem::path ResolveRelativePath(const std::string& texPath, const std::filesystem::path& modelPath)
	{
		std::filesystem::path textureRelPath = texPath;
		textureRelPath = textureRelPath.lexically_normal(); 
		if (textureRelPath.is_absolute())
			return textureRelPath; 

		return (modelPath.parent_path() / textureRelPath).lexically_normal();
	}

	Assimp::Importer m_Importer;
	MeshImporter m_MeshImporter;
	MaterialImporter m_MaterialImporter;
	TextureImporter m_TextureImporter;
	std::unordered_map<uint32_t, AssetUUID> materialIndexToUUID;
	std::unordered_map<std::string, AssetUUID> texturePathToUUID;
	std::unordered_map<unsigned int, AssetUUID> meshIndextoUUID;

};

