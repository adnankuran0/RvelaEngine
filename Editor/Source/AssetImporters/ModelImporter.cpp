#include "rvelapch.h"
#include "ModelImporter.h"
#include "PrefabImporter.h"
#include "Asset/AssetRegistry.h"
#include "Asset/Types/MeshAsset.h"
#include "Asset/Types/MaterialAsset.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Core/Log.h"
#include "Math/RvelaMath.h"
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>
#include <nlohmann/json.hpp>
#include "MaterialSerializer.h"

using namespace rv;
using json = nlohmann::json;

std::string ModelImporter::SanitizeFilename(const std::string& name)
{
    std::string result = name;
    for (char& c : result)
        if (c == ':' || c == '/' || c == '\\' || c == '?' ||
            c == '*' || c == '"' || c == '<' || c == '>' || c == '|')
            c = '_';
    return result;
}

std::filesystem::path ModelImporter::ResolveRelativePath(
    const std::string& texPath,
    const std::filesystem::path& modelPath) const
{
    std::filesystem::path p = std::filesystem::path(texPath).lexically_normal();
    if (p.is_absolute()) return p;
    return (modelPath.parent_path() / p).lexically_normal();
}

ModelImportSettings ModelImporter::ParseSettings(const std::string& settingsJson)
{
    ModelImportSettings s;
    if (settingsJson.empty() || settingsJson == "{}") return s;

    auto j = json::parse(settingsJson, nullptr, false);
    if (j.is_discarded()) return s;

    s.scale = j.value("scale", 1.0f);
    s.importNormals = j.value("importNormals", true);
    s.importTangents = j.value("importTangents", true);
    s.importUVs = j.value("importUVs", true);
    s.importTextures = j.value("importTextures", true);
    s.importMaterials = j.value("importMaterials", true);
    s.generatePrefab = j.value("generatePrefab", true);
    return s;
}

std::filesystem::path ModelImporter::GetCachePath(
    const std::filesystem::path& sourcePath,
    const AssetMeta& meta,
    const std::filesystem::path& cacheRoot) const
{
    return cacheRoot / (meta.uuid.ToString() + ".rprefab");
}

bool ModelImporter::Import(const std::filesystem::path& sourcePath,
    const std::filesystem::path& outCachePath,
    const std::string& settingsJson)
{
    auto& manager = AssetManager::Get();
    auto settings = ParseSettings(settingsJson);
    auto result = ImportModel(sourcePath, manager.GetRegistry(), settings);
    return result.prefabUUID.IsValid();
}

ModelImportResult ModelImporter::ImportModel(
    const std::filesystem::path& sourcePath,
    AssetRegistry& registry,
    const ModelImportSettings& settings)
{
    ModelImportResult result;

    const aiScene* scene = LoadScene(sourcePath, settings);
    if (!scene) { LOG_ERROR("Failed to load: {}", sourcePath.string()); return result; }

    if (settings.importTextures)
        ExtractTextures(scene, sourcePath, registry, result);

    if (settings.importMaterials)
        ExtractMaterials(scene, sourcePath, registry, result);

    ExtractMeshes(scene, sourcePath, registry, result);

    MergeAndSaveSubAssets(scene, sourcePath, registry, result);

    if (settings.generatePrefab)
        result.prefabUUID = ConstructPrefab(scene, sourcePath, registry, result);

    return result;
}

void ModelImporter::MergeAndSaveSubAssets(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    const ModelImportResult& result)
{
    AssetMeta parentMeta = registry.GetOrCreateMeta(modelPath);

    auto ensureSubAsset = [&](const SubAssetEntry& entry) {
        for (auto& sub : parentMeta.subAssets)
            if (sub.type == entry.type && sub.index == entry.index)
            {
                sub.uuid = entry.uuid;
                sub.sourceHash = entry.sourceHash;
                return;
            }
        parentMeta.subAssets.push_back(entry);
        };

    for (auto& [meshIndex, meshUUID] : result.meshUUIDs)
    {
        SubAssetEntry entry;
        entry.uuid = meshUUID;
        entry.name = scene->mMeshes[meshIndex]->mName.C_Str();
        if (entry.name.empty())
            entry.name = modelPath.stem().string() + "_Mesh" + std::to_string(meshIndex);
        entry.type = "Mesh";
        entry.index = meshIndex;
        entry.sourceHash = IAssetImporter::HashFile(modelPath)
            ^ (uint64_t(meshIndex) * 2654435761ULL);
        ensureSubAsset(entry);
    }

    for (auto& [matIndex, matUUID] : result.materialUUIDs)
    {
        SubAssetEntry entry;
        entry.uuid = matUUID;
        entry.name = modelPath.stem().string() + "_Mat" + std::to_string(matIndex);
        entry.type = "Material";
        entry.index = matIndex;
        entry.sourceHash = 0; 
        ensureSubAsset(entry);
    }

    registry.SaveMeta(modelPath, parentMeta);
}

const aiScene* ModelImporter::LoadScene(
    const std::filesystem::path& path,
    const ModelImportSettings& settings)
{
    m_Importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    m_Importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, settings.scale);
    m_Importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ALL_GEOMETRY_LAYERS, false);

    unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_GenBoundingBoxes;

    if (settings.importNormals)  flags |= aiProcess_GenSmoothNormals;
    if (settings.importTangents) flags |= aiProcess_CalcTangentSpace;

    const aiScene* scene = m_Importer.ReadFile(path.string(), flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_ERROR("Assimp error: {}", m_Importer.GetErrorString());
        return nullptr;
    }

    return scene;
}

void ModelImporter::ExtractTextures(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    ModelImportResult& result)
{
    std::unordered_set<std::string> uniquePaths;

    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* mat = scene->mMaterials[i];
        for (int type = aiTextureType_NONE + 1; type <= aiTextureType_UNKNOWN; ++type)
        {
            int count = mat->GetTextureCount((aiTextureType)type);
            for (int t = 0; t < count; ++t)
            {
                aiString texPath;
                if (mat->GetTexture((aiTextureType)type, t, &texPath) == AI_SUCCESS)
                {
                    auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath);
                    uniquePaths.insert(resolved.string());
                }
            }
        }
    }

    for (const auto& pathStr : uniquePaths)
    {
        std::filesystem::path texPath = pathStr;
        if (!std::filesystem::exists(texPath))
        {
            LOG_WARN("Texture not found: {}", pathStr);
            continue;
        }

        AssetMeta meta = registry.GetOrCreateMeta(texPath);
        auto cacheRoot = registry.GetAssetDir() / ".cache";
        auto cachePath = m_TextureImporter.GetCachePath(texPath, meta, cacheRoot);
        std::filesystem::create_directories(cachePath.parent_path());

        uint64_t currentHash = IAssetImporter::HashFile(texPath);
        if (meta.sourceHash != currentHash || !std::filesystem::exists(cachePath))
        {
            if (m_TextureImporter.Import(texPath, cachePath, meta.importerSettingsJson))
            {
                meta.importerID = m_TextureImporter.GetImporterID();
                meta.sourceHash = currentHash;
                registry.SaveMeta(texPath, meta);
            }
            else
            {
                LOG_ERROR("Texture import failed: {}", texPath.string());
                continue;
            }
        }

        registry.RegisterSubAsset(meta.uuid, cachePath, "TextureImporter");
        result.textureUUIDs[pathStr] = meta.uuid;
    }
}

void ModelImporter::ExtractMaterials(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    ModelImportResult& result)
{
    AssetMeta parentMeta = registry.GetOrCreateMeta(modelPath);

    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* aiMat = scene->mMaterials[i];

        std::string matFileName = modelPath.stem().string()
            + "_Mat" + std::to_string(i) + ".rmat";
        auto matFilePath = modelPath.parent_path() / matFileName;

        AssetMeta meta = registry.GetOrCreateMeta(matFilePath);
        if (meta.importerID.empty())
        {
            meta.importerID = "MaterialLoader";
            registry.SaveMeta(matFilePath, meta);
        }

        auto asset = CreateRef<MaterialAsset>(meta.uuid);

        aiString  texPath;
        aiColor4D aiColor;
        ai_real   value;

        // Albedo
        if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
        {
            auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath).string();
            if (result.textureUUIDs.contains(resolved))
            {
                asset->albedoTextureUUID = result.textureUUIDs.at(resolved);
                asset->useAlbedoMap = true;
            }
        }
        else
        {
            if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &aiColor) == AI_SUCCESS)
                asset->albedoColor = { aiColor.r, aiColor.g, aiColor.b };
            else if (aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &aiColor) == AI_SUCCESS)
                asset->albedoColor = { aiColor.r, aiColor.g, aiColor.b };
        }

        // Normal
        if (aiMat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_HEIGHT, 0, &texPath) == AI_SUCCESS)
        {
            auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath).string();
            if (result.textureUUIDs.contains(resolved))
            {
                asset->normalTextureUUID = result.textureUUIDs.at(resolved);
                asset->useNormalMap = true;
            }
        }

        // Metallic
        if (aiMat->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS)
        {
            auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath).string();
            if (result.textureUUIDs.contains(resolved))
            {
                asset->metallicTextureUUID = result.textureUUIDs.at(resolved);
                asset->useMetallicMap = true;
            }
        }
        else if (aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &value) == AI_SUCCESS)
            asset->metallic = static_cast<float>(value);

        // Roughness
        if (aiMat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_SHININESS, 0, &texPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_UNKNOWN, 0, &texPath) == AI_SUCCESS)
        {
            auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath).string();
            if (result.textureUUIDs.contains(resolved))
            {
                asset->roughnessTextureUUID = result.textureUUIDs.at(resolved);
                asset->useRoughnessMap = true;
            }
        }
        else if (aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &value) == AI_SUCCESS)
            asset->roughness = static_cast<float>(value);

        // AO
        if (aiMat->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texPath) == AI_SUCCESS)
        {
            auto resolved = ResolveRelativePath(texPath.C_Str(), modelPath).string();
            if (result.textureUUIDs.contains(resolved))
            {
                asset->aoTextureUUID = result.textureUUIDs.at(resolved);
                asset->useAOMap = true;
            }
        }

        // UV Transform
        aiUVTransform uvTransform;
        if (aiMat->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, 0), uvTransform) == AI_SUCCESS)
        {
            asset->UVScale = { uvTransform.mScaling.x,     uvTransform.mScaling.y };
            asset->UVOffset = { uvTransform.mTranslation.x, uvTransform.mTranslation.y };
        }

        if (!MaterialSerializer::Save(asset, matFilePath))
        {
            LOG_ERROR("Failed to write material: {}", matFilePath.string());
            continue;
        }

        result.materialUUIDs[i] = meta.uuid;

        bool exists = false;
        for (auto& sub : parentMeta.subAssets)
            if (sub.type == "Material" && sub.index == i) { exists = true; break; }

        if (!exists)
        {
            SubAssetEntry entry;
            entry.uuid = meta.uuid;
            entry.name = modelPath.stem().string() + "_Mat" + std::to_string(i);
            entry.type = "Material";
            entry.index = i;
            parentMeta.subAssets.push_back(entry);
        }
    }

    registry.SaveMeta(modelPath, parentMeta);
}

void ModelImporter::ExtractMeshes(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    ModelImportResult& result)
{
    m_MeshImporter.ImportFromScene(scene, modelPath, registry, result.meshUUIDs);

    AssetMeta parentMeta = registry.GetOrCreateMeta(modelPath);

    for (auto& [meshIndex, meshUUID] : result.meshUUIDs)
    {
        SubAssetEntry* existing = nullptr;
        for (auto& sub : parentMeta.subAssets)
        {
            if (sub.type == "Mesh" && sub.index == meshIndex)
            {
                existing = &sub;
                break;
            }
        }

        uint64_t hash = IAssetImporter::HashFile(modelPath)
            ^ (uint64_t(meshIndex) * 2654435761ULL);

        if (existing)
        {
            existing->uuid = meshUUID;
            existing->sourceHash = hash;
        }
        else
        {
            SubAssetEntry entry;
            entry.uuid = meshUUID;
            entry.name = scene->mMeshes[meshIndex]->mName.C_Str();
            if (entry.name.empty())
                entry.name = modelPath.stem().string() + "_Mesh" + std::to_string(meshIndex);
            entry.type = "Mesh";
            entry.index = meshIndex;
            entry.sourceHash = hash;
            parentMeta.subAssets.push_back(entry);
        }
    }

    registry.SaveMeta(modelPath, parentMeta);
}

AssetUUID ModelImporter::ConstructPrefab(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    const ModelImportResult& result)
{
    Scene prefabScene;
    entt::entity rootEntity = ProcessNode(
        scene->mRootNode, scene, entt::null, prefabScene, result);
    prefabScene.GetTransformSystem().Update();

    auto prefabPath = modelPath;
    prefabPath.replace_extension(".rprefab");

    AssetMeta meta = registry.GetOrCreateMeta(prefabPath);

    Ref<PrefabAsset> prefab = PrefabImporter::CreatePrefabAsset(
        prefabPath, meta.uuid, prefabScene, rootEntity);

    if (!prefab) return AssetUUID::Invalid();

    meta.importerID = "PrefabImporter";
    meta.dependencies.clear();
    for (auto& [idx, uuid] : result.meshUUIDs)
        meta.dependencies.push_back(uuid);
    for (auto& [idx, uuid] : result.materialUUIDs)
        meta.dependencies.push_back(uuid);

    registry.SaveMeta(prefabPath, meta);
    return meta.uuid;
}

entt::entity ModelImporter::ProcessNode(
    aiNode* node,
    const aiScene* modelScene,
    entt::entity parent,
    Scene& scene,
    const ModelImportResult& result)
{
    std::string nodeName = node->mName.C_Str();
    entt::entity e = scene.CreateEntity(nodeName).GetHandle();

    SetTransformForEntity(node, scene, e);

    if (parent != entt::null)
        scene.SetParent(e, parent);

    if (node->mNumMeshes == 1)
    {
        AttachMeshToEntity(0, node, modelScene, scene, e, result);
    }
    else if (node->mNumMeshes > 1)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            entt::entity child = scene.CreateEntity(
                nodeName + "_" + std::to_string(i)).GetHandle();
            SetTransformForEntity(node, scene, child);
            AttachMeshToEntity(i, node, modelScene, scene, child, result);
            scene.SetParent(child, e);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        ProcessNode(node->mChildren[i], modelScene, e, scene, result);

    return e;
}

void ModelImporter::AttachMeshToEntity(
    unsigned int meshIndex,
    aiNode* node,
    const aiScene* modelScene,
    Scene& scene,
    entt::entity e,
    const ModelImportResult& result)
{
    unsigned int globalMeshIndex = node->mMeshes[meshIndex];
    aiMesh* mesh = modelScene->mMeshes[globalMeshIndex];

    auto meshIt = result.meshUUIDs.find(globalMeshIndex);
    if (meshIt == result.meshUUIDs.end())
    {
        LOG_WARN("Mesh UUID not found for index: {}", globalMeshIndex);
        return;
    }

    auto matIt = result.materialUUIDs.find(mesh->mMaterialIndex);
    if (matIt == result.materialUUIDs.end())
    {
        LOG_WARN("Material UUID not found for index: {}", mesh->mMaterialIndex);
        return;
    }

    scene.AddComponent<MeshComponent>(e, meshIt->second);
    scene.AddComponent<MaterialComponent>(e, matIt->second);
}

void ModelImporter::SetTransformForEntity(
    aiNode* node,
    Scene& scene,
    entt::entity e)
{
    auto& tc = scene.GetComponent<TransformComponent>(e);
    glm::mat4 transform = math::ConvertToGlmMatrix(node->mTransformation);
    glm::vec3 pos, scale, rotDeg;
    math::DecomposeToEulerAngles(transform, scale, rotDeg, pos);
    tc.SetPosition(pos);
    tc.SetScale(scale);
    tc.SetEulerRotation(rotDeg);
    scene.GetTransformSystem().Update();
}