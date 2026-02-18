#include "ModelImporter.h"

#include "Assimp/scene.h"
#include "assimp/postprocess.h"

#include "Assets/PrefabAsset.h"
#include "PrefabImporter.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

namespace rv {

const aiScene* ModelImporter::LoadScene(const std::filesystem::path& path)
{
    m_Importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    m_Importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.0f);
    m_Importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ALL_GEOMETRY_LAYERS, false);

    const aiScene* scene = m_Importer.ReadFile(path.string(),
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality|
        aiProcess_GenBoundingBoxes
        );

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
        materialIndexToUUID[i] = material->GetUUID();
    }
}

AssetUUID ModelImporter::ConstructPrefab(const aiScene* scene, const std::filesystem::path& modelPath)
{
    Scene prefabScene;
    entt::entity rootEntity = ProcessNode(scene->mRootNode, scene, entt::null, prefabScene);
    prefabScene.UpdateHierarchy();

    std::filesystem::path prefabPath = modelPath;
    prefabPath.replace_extension(".rprefab");

    Ref<PrefabAsset> prefab = PrefabImporter::CreatePrefabAsset(prefabPath.string(), prefabScene, rootEntity);
    return prefab->GetUUID();
}

entt::entity ModelImporter::ProcessNode(aiNode* node, const aiScene* modelScene, entt::entity parent, Scene& scene)
{
    std::string nodeName = node->mName.C_Str();
    entt::entity e = scene.CreateEntity(nodeName).GetHandle();

    SetTransformForEntity(node, scene, e);

    if (parent != entt::null)
    {
        scene.SetParent(e, parent);
    }

    if (node->mNumMeshes == 1)
    {
        AttachMeshToEntity(0,node, modelScene, scene,e);
    }
    else if (node->mNumMeshes > 1)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            entt::entity childEntity = scene.CreateEntity(nodeName + "_" + std::to_string(i)).GetHandle();
            SetTransformForEntity(node, scene, childEntity);
            AttachMeshToEntity(i,node, modelScene, scene, childEntity);
            scene.SetParent(childEntity, e);
        }
    }

    //Process childs recursively
    for (unsigned int i = 0; i < node->mNumChildren; i++)
        ProcessNode(node->mChildren[i], modelScene, e, scene);

    return e;
}

void ModelImporter::AttachMeshToEntity(unsigned int meshIndex, aiNode* node, const aiScene* modelScene, Scene& scene,entt::entity e)
{
    aiMesh* mesh = modelScene->mMeshes[node->mMeshes[meshIndex]]; 

    auto meshIt = meshIndextoUUID.find(node->mMeshes[meshIndex]); 
    if (meshIt == meshIndextoUUID.end()) {
        LOG_DEBUG("Cant find uuid for mesh index");
        //continue;
    }
    AssetUUID meshUUID = meshIt->second;

    auto matIt = materialIndexToUUID.find(mesh->mMaterialIndex);
    if (matIt == materialIndexToUUID.end()) {
        LOG_DEBUG("Cant find uuid for material index");
    }
    AssetUUID matUUID = matIt->second;

    scene.AddComponent<MeshComponent>(e, meshUUID);
    Ref<MeshAsset> meshAsset = scene.GetComponent<MeshComponent>(e).GetMesh();
    scene.AddComponent<MeshRendererComponent>(e, meshAsset);
    scene.AddComponent<MaterialComponent>(e, matUUID);
}

void ModelImporter::SetTransformForEntity(aiNode* node, Scene& scene, entt::entity e)
{
    auto& tc = scene.GetComponent<TransformComponent>(e);
    glm::mat4 transform = math::ConvertToGlmMatrix(node->mTransformation);
    glm::vec3 pos, scale, rotDeg;
    math::DecomposeToEulerAngles(transform, scale, rotDeg, pos);
    tc.SetPosition(pos);
    tc.SetScale(scale);
    tc.SetEulerRotation(rotDeg);
    scene.UpdateHierarchy();
}

AssetUUID ModelImporter::Import(const std::filesystem::path& path)
{
    const aiScene* scene = LoadScene(path);
    if (!scene)
    {
        LOG_ERROR("Failed to load model scene: {}", path.string());
        return AssetUUID::Invalid();
    }
    ExtractTextures(scene, path);
    ExtractMaterials(scene, path);
	m_MeshImporter.Import(scene,path, meshIndextoUUID);
    AssetRegistry::ScanAssets();
	return ConstructPrefab(scene,path); 
}

}