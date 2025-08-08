#pragma once
#include "AssetImporter.h"
#include "MeshImporter.h"
#include "Assimp/Importer.hpp"
#include "Assimp/scene.h"
#include <assimp/postprocess.h>
#include "TextureImporter.h"

class ModelImporter : public IAssetImporter
{
	AssetUUID Import(const std::filesystem::path& path) override;
private:
	const aiScene* LoadScene(const std::filesystem::path& path);
	void ExtractTextures(const aiScene* scene, const std::filesystem::path& modelPath);
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
	TextureImporter m_TextureImporter;
	std::unordered_map<uint32_t, AssetUUID> materialIndexToUUID;
	std::unordered_map<std::string, AssetUUID> texturePathToUUID;
};

