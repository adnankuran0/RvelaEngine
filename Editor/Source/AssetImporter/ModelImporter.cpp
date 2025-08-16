#include "ModelImporter.h"
#include <Assets/MaterialAsset.h>

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
        if (uuid.IsValid())
        {
            texturePathToUUID[path] = uuid;
        }
    }
}

void ModelImporter::ExtractMaterials(const aiScene* scene, const std::filesystem::path& modelPath)
{
    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* aiMat = scene->mMaterials[i];

        std::string matFileName = modelPath.stem().string() + "_Mat" + std::to_string(i) + ".rmat";
        std::filesystem::path matFilePath = modelPath.parent_path() / matFileName;

        Ref<MaterialAsset> material = m_MaterialImporter.CreateMaterialAsset(matFilePath.string());

        aiString texPath;
        aiColor4D aiColor;

        if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
        {
            auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath).string();
            if (texturePathToUUID.contains(resolved))
            {
                material->albedoTextureUUID = texturePathToUUID.at(resolved);
                material->useAlbedoMap = true;
            }
        }
        else
        {
            aiColor4D aiColor;
            if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &aiColor) == AI_SUCCESS)
            {
                material->albedoColor = glm::vec3(aiColor.r, aiColor.g, aiColor.b);
            }
            else if (aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &aiColor) == AI_SUCCESS)
            {
                material->albedoColor = glm::vec3(aiColor.r, aiColor.g, aiColor.b);
            }
        }

        if (aiMat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_HEIGHT, 0, &texPath) == AI_SUCCESS )
        {
            auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath).string();
            if (texturePathToUUID.contains(resolved))
            {
                material->normalTextureUUID = texturePathToUUID.at(resolved);
                material->useNormalMap = true;
            }
        }
        if (aiMat->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS)
        {
            auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath).string();
            if (texturePathToUUID.contains(resolved))
            {
                material->metallicTextureUUID = texturePathToUUID.at(resolved);
                material->useMetallicMap = true;
            }
        }
        else
        {
            ai_real value;
            if (aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &value) == AI_SUCCESS)
                material->metallic = static_cast<float>(value);
        }

        if (aiMat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_SHININESS, 0, &texPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_UNKNOWN, 0, &texPath) == AI_SUCCESS) 
        {
            auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath).string();
            if (texturePathToUUID.contains(resolved))
            {
                material->roughnessTextureUUID = texturePathToUUID.at(resolved);
                material->useRoughnessMap = true;
            }
        }
        else
        {
            ai_real value;
            if (aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &value) == AI_SUCCESS)
                material->roughness = static_cast<float>(value);
        }

        if (aiMat->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texPath) == AI_SUCCESS)
        {
            auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath).string();
            if (texturePathToUUID.contains(resolved))
            {
                material->aoTextureUUID = texturePathToUUID.at(resolved);
                material->useAOMap = true;
            }
        }

        aiUVTransform uvTransform;
        if (aiMat->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, 0), uvTransform) == AI_SUCCESS)
        {
            material->UVScale = glm::vec2(uvTransform.mScaling.x, uvTransform.mScaling.y);
            material->UVOffset = glm::vec2(uvTransform.mTranslation.x, uvTransform.mTranslation.y);
        }

        material->Serialize();
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
    ExtractTextures(scene, path);
    ExtractMaterials(scene, path);
	m_MeshImporter.Import(path,scene);
	return AssetUUID::Invalid(); //TODO: This function must return prefab uuid
}