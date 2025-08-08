#include "ModelImporter.h"

const aiScene* ModelImporter::LoadScene(const std::filesystem::path& path)
{
    m_Importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    const aiScene* scene = m_Importer.ReadFile(path.string(),
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality|
        aiProcess_GenBoundingBoxes);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "[MeshImporter] Failed to load scene: " << m_Importer.GetErrorString() << std::endl;
        return nullptr;
    }

    return scene;
}

void ModelImporter::ExtractTextures(const aiScene* scene,const std::filesystem::path& modelPath)
{
    std::unordered_set<std::string> uniqueTexturePaths;

    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* material = scene->mMaterials[i];

        for (int type = aiTextureType_NONE + 1; type <= aiTextureType_UNKNOWN; ++type)
        {
            int textureCount = material->GetTextureCount((aiTextureType)type);
            for (int texIndex = 0; texIndex < textureCount; ++texIndex)
            {
                aiString path;
                if (material->GetTexture((aiTextureType)type, texIndex, &path) == AI_SUCCESS)
                {
                    std::string texPath = path.C_Str();
                    std::filesystem::path resolved = ResolveRelativePath(texPath, modelPath);
                    uniqueTexturePaths.insert(resolved.string());
                }
            }
        }
    }

    for (const auto& path : uniqueTexturePaths)
    {
        AssetUUID uuid = m_TextureImporter.Import(path); 
        if (!uuid.IsValid()) continue;

        texturePathToUUID[path] = uuid;
    }
}

AssetUUID ModelImporter::Import(const std::filesystem::path& path)
{
    const aiScene* scene = LoadScene(path);
    if (!scene)
    {
        LOG_ERROR("Failed to load model scene: {}", path.string());
        return AssetUUID::Invalid();;
    }
    //ExtractTextures(scene, path);
	//bool resultMaterialImport = m_MaterialImporter.Import(path);
	bool resultMeshImport = m_MeshImporter.Import(path,scene);
	return AssetUUID::Invalid(); //TODO: This function must return prefab uuid
}