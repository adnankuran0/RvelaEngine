#include "rvelapch.h"
#include "ModelImporter.h"
#include "PrefabImporter.h"
#include "Asset/AssetRegistry.h"
#include "Asset/Types/MeshAsset.h"
#include "Asset/Types/MaterialAsset.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "Scene/Components/SkeletonComponent.h"
#include "Scene/Components/SkeletalMeshComponent.h"
#include "Scene/Components/SkeletalMeshRendererComponent.h"
#include "Core/Log.h"
#include "Math/RvelaMath.h"
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>
#include <nlohmann/json.hpp>
#include "MaterialSerializer.h"
#include <fstream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

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
    auto prefabPath = sourcePath;
    prefabPath.replace_extension(".rprefab");
    return prefabPath;
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

    ExtractSkeleton(scene, sourcePath, registry, result);

    if (scene->mNumAnimations > 0 && result.skeletonUUID.IsValid())
    {
        result.animLibUUID = m_AnimationImporter.ImportFromScene(
            scene, sourcePath, registry, result.boneNameToIndex);
    }

    ExtractMeshes(scene, sourcePath, registry, result);
    MergeAndSaveSubAssets(scene, sourcePath, registry, result);

    if (settings.generatePrefab)
        result.prefabUUID = ConstructPrefab(scene, sourcePath, registry, result);

    return result;
}

void ModelImporter::ExtractSkeleton(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    ModelImportResult& result)
{
    result.skeletonUUID = m_SkeletonImporter.ImportFromScene(
        scene, modelPath, registry, &result.boneNameToIndex);

    if (!result.skeletonUUID.IsValid())
        return;

    AssetMeta parentMeta = registry.GetOrCreateMeta(modelPath);

    bool exists = false;
    for (auto& sub : parentMeta.subAssets)
        if (sub.type == "Skeleton") { exists = true; break; }

    if (!exists)
    {
        SubAssetEntry entry;
        entry.uuid = result.skeletonUUID;
        entry.name = modelPath.stem().string() + "_Skeleton";
        entry.type = "Skeleton";
        entry.index = 0;
        entry.hasCache = false;
        parentMeta.subAssets.push_back(entry);
    }

    registry.SaveMeta(modelPath, parentMeta);
}

std::string ModelImporter::GetDefaultSettings() const
{
    ModelImportSettings s;

    json j;
    j["scale"] = s.scale;
    j["importNormals"] = s.importNormals;
    j["importTangents"] = s.importTangents;
    j["importUVs"] = s.importUVs;
    j["importTextures"] = s.importTextures;
    j["importMaterials"] = s.importMaterials;
    j["generatePrefab"] = s.generatePrefab;

    return j.dump();
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
                sub.hasCache = entry.hasCache;
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
        entry.hasCache = true;
        ensureSubAsset(entry);
    }

    for (auto& [matIndex, matUUID] : result.materialUUIDs)
    {
        SubAssetEntry entry;
        entry.uuid = matUUID;
        entry.name = modelPath.stem().string() + "_Mat" + std::to_string(matIndex);
        entry.type = "Material";
        entry.index = matIndex;
        entry.hasCache = false;
        ensureSubAsset(entry);
    }

    if (result.animLibUUID.IsValid())
    {
        SubAssetEntry entry;
        entry.uuid = result.animLibUUID;
        entry.name = modelPath.stem().string() + "_AnimLib";
        entry.type = "AnimationLib";
        entry.index = 0;
        entry.hasCache = false;
        ensureSubAsset(entry);
    }
    else
    {
        std::erase_if(parentMeta.subAssets, [](const SubAssetEntry& sub) {
            return sub.type == "AnimationLib";
        });
    }

    if (!result.skeletonUUID.IsValid())
    {
        std::erase_if(parentMeta.subAssets, [](const SubAssetEntry& sub) {
            return sub.type == "Skeleton";
        });
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
        aiProcess_GenUVCoords |
        aiProcess_TransformUVCoords |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_GenBoundingBoxes;

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
    auto ImportTextureFile = [&](const std::filesystem::path& texPath) -> AssetUUID {
        AssetMeta meta = registry.GetOrCreateMeta(texPath);
        auto cacheRoot = registry.GetAssetDir() / ".cache";
        auto cachePath = m_TextureImporter.GetCachePath(texPath, meta, cacheRoot);
        std::filesystem::create_directories(cachePath.parent_path());

        std::error_code ec;
        auto lastWrite = std::filesystem::last_write_time(texPath, ec);
        uint64_t currentTimeStamp = 0;
        if (!ec)
        {
            currentTimeStamp = std::chrono::duration_cast<std::chrono::seconds>(
                lastWrite.time_since_epoch()).count();
        }

        bool needsImport = !std::filesystem::exists(cachePath) ||
            meta.lastWriteTime != currentTimeStamp;

        if (needsImport)
        {
            LOG_INFO("Importing texture: {}", texPath.filename().string());

            if (m_TextureImporter.Import(texPath, cachePath, meta.importerSettingsJson))
            {
                meta.importerID = m_TextureImporter.GetImporterID();
                meta.lastWriteTime = currentTimeStamp;
                registry.SaveMeta(texPath, meta);
            }
            else
            {
                LOG_ERROR("Texture import failed: {}", texPath.string());
                return AssetUUID::Invalid();
            }
        }

        registry.RegisterSubAsset(meta.uuid, cachePath, "TextureImporter");
        return meta.uuid;
    };

    // 1. Extract all embedded textures (GLB, embedded FBX, etc.)
    if (scene->mNumTextures > 0)
    {
        auto texturesDir = modelPath.parent_path() / (modelPath.stem().string() + "_Textures");
        std::filesystem::create_directories(texturesDir);

        for (uint32_t t = 0; t < scene->mNumTextures; ++t)
        {
            const aiTexture* embedded = scene->mTextures[t];
            if (!embedded) continue;

            std::string ext = "png";
            if (embedded->mHeight == 0)
            {
                std::string hint = embedded->achFormatHint;
                std::transform(hint.begin(), hint.end(), hint.begin(), ::tolower);
                if (hint.find("jpg") != std::string::npos || hint.find("jpeg") != std::string::npos)
                    ext = "jpg";
                else if (hint.find("png") != std::string::npos)
                    ext = "png";
                else if (hint.find("tga") != std::string::npos)
                    ext = "tga";
                else if (hint.find("bmp") != std::string::npos)
                    ext = "bmp";
                else if (embedded->mWidth >= 4)
                {
                    const uint8_t* b = reinterpret_cast<const uint8_t*>(embedded->pcData);
                    if (b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G')
                        ext = "png";
                    else if (b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF)
                        ext = "jpg";
                }
            }

            std::string stemName;
            std::string rawName = embedded->mFilename.C_Str();
            if (!rawName.empty() && rawName[0] != '*')
            {
                std::string clean = SanitizeFilename(std::filesystem::path(rawName).stem().string());
                if (!clean.empty())
                    stemName = modelPath.stem().string() + "_" + clean;
            }
            if (stemName.empty())
            {
                stemName = modelPath.stem().string() + "_embed_" + std::to_string(t);
            }

            auto outFilePath = texturesDir / (stemName + "." + ext);

            bool writeOk = true;
            if (!std::filesystem::exists(outFilePath))
            {
                if (embedded->mHeight == 0)
                {
                    std::ofstream out(outFilePath, std::ios::binary);
                    if (out.is_open())
                    {
                        out.write(reinterpret_cast<const char*>(embedded->pcData), embedded->mWidth);
                        out.close();
                    }
                    else writeOk = false;
                }
                else
                {
                    uint32_t w = embedded->mWidth;
                    uint32_t h = embedded->mHeight;
                    std::vector<uint8_t> rgba(w * h * 4);
                    const aiTexel* texels = embedded->pcData;
                    for (uint32_t i = 0; i < w * h; ++i)
                    {
                        rgba[i * 4 + 0] = texels[i].r;
                        rgba[i * 4 + 1] = texels[i].g;
                        rgba[i * 4 + 2] = texels[i].b;
                        rgba[i * 4 + 3] = texels[i].a;
                    }
                    int ret = stbi_write_png(outFilePath.string().c_str(), w, h, 4, rgba.data(), w * 4);
                    if (!ret) writeOk = false;
                }
            }

            if (writeOk && std::filesystem::exists(outFilePath))
            {
                AssetUUID uuid = ImportTextureFile(outFilePath);
                if (uuid.IsValid())
                {
                    std::string starKey = "*" + std::to_string(t);
                    result.textureUUIDs[starKey] = uuid;
                    result.textureUUIDs[outFilePath.string()] = uuid;
                    result.textureUUIDs[outFilePath.lexically_normal().string()] = uuid;
                    if (!rawName.empty())
                    {
                        result.textureUUIDs[rawName] = uuid;
                        result.textureUUIDs[ResolveRelativePath(rawName, modelPath).string()] = uuid;
                        result.textureUUIDs[ResolveRelativePath(rawName, modelPath).lexically_normal().string()] = uuid;
                    }
                }
            }
        }
    }

    // 2. Collect external textures from materials
    std::unordered_map<std::string, std::filesystem::path> externalTextures;

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
                    std::string rawPath = texPath.C_Str();
                    if (rawPath.empty()) continue;
                    if (result.textureUUIDs.contains(rawPath)) continue;

                    if (const aiTexture* emb = scene->GetEmbeddedTexture(rawPath.c_str()))
                    {
                        continue;
                    }

                    auto resolved = ResolveRelativePath(rawPath, modelPath);
                    if (!std::filesystem::exists(resolved))
                    {
                        auto fb1 = modelPath.parent_path() / resolved.filename();
                        if (std::filesystem::exists(fb1))
                            resolved = fb1;
                        else
                        {
                            auto fb2 = modelPath.parent_path() / "textures" / resolved.filename();
                            if (std::filesystem::exists(fb2))
                                resolved = fb2;
                            else
                            {
                                auto fb3 = modelPath.parent_path() / "Textures" / resolved.filename();
                                if (std::filesystem::exists(fb3))
                                    resolved = fb3;
                            }
                        }
                    }

                    if (std::filesystem::exists(resolved))
                    {
                        externalTextures[rawPath] = resolved.lexically_normal();
                    }
                    else
                    {
                        LOG_WARN("Texture not found: {}", rawPath);
                    }
                }
            }
        }
    }

    // Import external textures
    for (const auto& [rawPath, texPath] : externalTextures)
    {
        std::string resolvedStr = texPath.string();
        if (result.textureUUIDs.contains(resolvedStr))
        {
            result.textureUUIDs[rawPath] = result.textureUUIDs[resolvedStr];
            continue;
        }

        AssetUUID uuid = ImportTextureFile(texPath);
        if (uuid.IsValid())
        {
            result.textureUUIDs[resolvedStr] = uuid;
            result.textureUUIDs[rawPath] = uuid;
        }
    }
}

void ModelImporter::ExtractMaterials(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    ModelImportResult& result)
{
    AssetMeta parentMeta = registry.GetOrCreateMeta(modelPath);

    auto GetTextureInfo = [&](aiMaterial* mat, aiTextureType type, int index = 0) -> std::pair<std::string, AssetUUID> {
        aiString texPath;
        if (mat->GetTexture(type, index, &texPath) == AI_SUCCESS)
        {
            std::string raw = texPath.C_Str();
            if (raw.empty()) return { "", AssetUUID::Invalid() };

            if (result.textureUUIDs.contains(raw))
                return { raw, result.textureUUIDs.at(raw) };

            auto resolved = ResolveRelativePath(raw, modelPath).lexically_normal().string();
            if (result.textureUUIDs.contains(resolved))
                return { raw, result.textureUUIDs.at(resolved) };

            std::string fn = std::filesystem::path(raw).filename().string();
            for (const auto& [key, uuid] : result.textureUUIDs)
            {
                if (std::filesystem::path(key).filename().string() == fn)
                    return { raw, uuid };
            }
        }
        return { "", AssetUUID::Invalid() };
    };

    auto isARMPathName = [](const std::string& path) -> bool {
        std::string lower = path;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower.find("arm") != std::string::npos ||
               lower.find("orm") != std::string::npos ||
               lower.find("metallicroughness") != std::string::npos ||
               lower.find("occlusionroughnessmetallic") != std::string::npos ||
               lower.find("roughnessmetallic") != std::string::npos;
    };

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
        ai_real value;

        // 1. Albedo / Base Color
        auto [albedoPath, albedoUUID] = GetTextureInfo(aiMat, aiTextureType_DIFFUSE);
        if (!albedoUUID.IsValid())
            std::tie(albedoPath, albedoUUID) = GetTextureInfo(aiMat, aiTextureType_BASE_COLOR);

        if (albedoUUID.IsValid())
        {
            asset->albedoTextureUUID = albedoUUID;
            asset->useAlbedoMap = true;
            asset->albedoColor = glm::vec4(1.0f);
        }
        else
        {
            aiColor4D aiColor(1.0f, 1.0f, 1.0f, 1.0f);
            if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &aiColor) == AI_SUCCESS ||
                aiGetMaterialColor(aiMat, AI_MATKEY_BASE_COLOR, &aiColor) == AI_SUCCESS)
            {
                asset->albedoColor = glm::vec4(aiColor.r, aiColor.g, aiColor.b, aiColor.a);
            }
            else
            {
                asset->albedoColor = glm::vec4(1.0f);
            }
        }

        // 2. Normal
        auto [normalPath, normalUUID] = GetTextureInfo(aiMat, aiTextureType_NORMALS);
        if (!normalUUID.IsValid())
            std::tie(normalPath, normalUUID) = GetTextureInfo(aiMat, aiTextureType_HEIGHT);

        if (normalUUID.IsValid())
        {
            asset->normalTextureUUID = normalUUID;
            asset->useNormalMap = true;
        }

        // 3. Metallic, Roughness, AO & ARM Detection
        auto [metalPath, metalUUID] = GetTextureInfo(aiMat, aiTextureType_METALNESS);

        auto [roughPath, roughUUID] = GetTextureInfo(aiMat, aiTextureType_DIFFUSE_ROUGHNESS);
        if (!roughUUID.IsValid())
            std::tie(roughPath, roughUUID) = GetTextureInfo(aiMat, aiTextureType_SHININESS);
        if (!roughUUID.IsValid())
            std::tie(roughPath, roughUUID) = GetTextureInfo(aiMat, aiTextureType_UNKNOWN);

        auto [aoPath, aoUUID] = GetTextureInfo(aiMat, aiTextureType_AMBIENT_OCCLUSION);
        if (!aoUUID.IsValid())
            std::tie(aoPath, aoUUID) = GetTextureInfo(aiMat, aiTextureType_LIGHTMAP);

        bool isARM = false;
        AssetUUID armUUID = AssetUUID::Invalid();

        if (metalUUID.IsValid() && roughUUID.IsValid() && metalUUID == roughUUID)
        {
            isARM = true;
            armUUID = metalUUID;
        }
        else if (metalUUID.IsValid() && isARMPathName(metalPath))
        {
            isARM = true;
            armUUID = metalUUID;
        }
        else if (roughUUID.IsValid() && isARMPathName(roughPath))
        {
            isARM = true;
            armUUID = roughUUID;
        }
        else if (aoUUID.IsValid() && isARMPathName(aoPath))
        {
            isARM = true;
            armUUID = aoUUID;
        }

        if (isARM && armUUID.IsValid())
        {
            asset->useARMMap = true;
            asset->armTextureUUID = armUUID;
            asset->useMetallicMap = false;
            asset->useRoughnessMap = false;

            if (!aoUUID.IsValid() || aoUUID == armUUID)
            {
                asset->useAOMap = false;
            }
            else
            {
                asset->useAOMap = true;
                asset->aoTextureUUID = aoUUID;
            }
        }
        else
        {
            asset->useARMMap = false;

            if (metalUUID.IsValid())
            {
                asset->metallicTextureUUID = metalUUID;
                asset->useMetallicMap = true;
            }

            if (roughUUID.IsValid())
            {
                asset->roughnessTextureUUID = roughUUID;
                asset->useRoughnessMap = true;
            }

            if (aoUUID.IsValid())
            {
                asset->aoTextureUUID = aoUUID;
                asset->useAOMap = true;
            }
        }

        // Values
        if (aiGetMaterialFloat(aiMat, AI_MATKEY_METALLIC_FACTOR, &value) == AI_SUCCESS)
            asset->metallic = static_cast<float>(value);

        if (aiGetMaterialFloat(aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &value) == AI_SUCCESS)
            asset->roughness = static_cast<float>(value);

        // Emissive
        aiColor4D emissiveColor(0.0f, 0.0f, 0.0f, 1.0f);
        if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor) == AI_SUCCESS)
        {
            asset->emissiveColor = glm::vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
            if (asset->emissiveColor.r > 0.0f || asset->emissiveColor.g > 0.0f || asset->emissiveColor.b > 0.0f)
                asset->emissiveIntensity = 1.0f;
        }
        if (aiGetMaterialFloat(aiMat, AI_MATKEY_EMISSIVE_INTENSITY, &value) == AI_SUCCESS)
            asset->emissiveIntensity = static_cast<float>(value);

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
            entry.hasCache = false;
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

    if (!result.boneNameToIndex.empty())
    {
        m_SkeletalMeshImporter.ImportFromScene(
            scene, modelPath, registry, result.boneNameToIndex, result.skeletalMeshUUIDs);
    }

    AssetMeta parentMeta = registry.GetOrCreateMeta(modelPath);

    auto ensureMeshSub = [&](const std::string& type, unsigned int meshIndex, AssetUUID uuid, bool hasCache)
        {
            SubAssetEntry* existing = nullptr;
            for (auto& sub : parentMeta.subAssets)
                if (sub.type == type && sub.index == meshIndex) { existing = &sub; break; }

            if (existing)
            {
                existing->uuid = uuid;
            }
            else
            {
                SubAssetEntry entry;
                entry.uuid = uuid;
                entry.name = scene->mMeshes[meshIndex]->mName.C_Str();
                if (entry.name.empty())
                    entry.name = modelPath.stem().string() + "_" + type + std::to_string(meshIndex);
                entry.type = type;
                entry.index = meshIndex;
                entry.hasCache = hasCache;
                parentMeta.subAssets.push_back(entry);
            }
        };

    for (auto& [meshIndex, meshUUID] : result.meshUUIDs)
        ensureMeshSub("Mesh", meshIndex, meshUUID, true);

    for (auto& [meshIndex, meshUUID] : result.skeletalMeshUUIDs)
        ensureMeshSub("SkeletalMesh", meshIndex, meshUUID, true);

    registry.SaveMeta(modelPath, parentMeta);
}

AssetUUID ModelImporter::ConstructPrefab(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    const ModelImportResult& result)
{
    Scene prefabScene;

    std::unordered_set<std::string> boneNames;
    for (auto& [boneName, _] : result.boneNameToIndex)
    {
        boneNames.insert(boneName);
    }

    std::string rootName = modelPath.stem().string();
    entt::entity rootEntity = prefabScene.CreateEntity(rootName).GetHandle();

    if (result.skeletonUUID.IsValid())
    {
        prefabScene.AddComponent<SkeletonComponent>(rootEntity, result.skeletonUUID);
    }

    if (result.animLibUUID.IsValid())
    {
        prefabScene.AddComponent<AnimatorComponent>(rootEntity, result.animLibUUID);
    }

    if (scene->mRootNode->mNumMeshes == 1)
    {
        AttachMeshToEntity(0, scene->mRootNode, scene, prefabScene, rootEntity, result);
    }
    else if (scene->mRootNode->mNumMeshes > 1)
    {
        for (unsigned int i = 0; i < scene->mRootNode->mNumMeshes; ++i)
        {
            entt::entity child = prefabScene.CreateEntity(rootName + "_" + std::to_string(i)).GetHandle();
            AttachMeshToEntity(i, scene->mRootNode, scene, prefabScene, child, result);
            prefabScene.SetParent(child, rootEntity);
        }
    }

    for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; ++i)
    {
        ProcessNode(scene->mRootNode->mChildren[i], scene, rootEntity, prefabScene, result, boneNames);
    }

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
    for (auto& [idx, uuid] : result.skeletalMeshUUIDs)
        meta.dependencies.push_back(uuid);
    for (auto& [idx, uuid] : result.materialUUIDs)
        meta.dependencies.push_back(uuid);
    if (result.skeletonUUID.IsValid())
        meta.dependencies.push_back(result.skeletonUUID);
    if (result.animLibUUID.IsValid())
        meta.dependencies.push_back(result.animLibUUID);

    registry.SaveMeta(prefabPath, meta);
    return meta.uuid;
}

entt::entity ModelImporter::ProcessNode(
    aiNode* node,
    const aiScene* modelScene,
    entt::entity parent,
    Scene& scene,
    const ModelImportResult& result,
    const std::unordered_set<std::string>& boneNames)
{
    std::string nodeName = node->mName.C_Str();

    if (boneNames.contains(nodeName) && node->mNumMeshes == 0)
    {
        return entt::null;
    }

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
            entt::entity child = scene.CreateEntity(nodeName + "_" + std::to_string(i)).GetHandle();
            SetTransformForEntity(node, scene, child);
            AttachMeshToEntity(i, node, modelScene, scene, child, result);
            scene.SetParent(child, e);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], modelScene, e, scene, result, boneNames);
    }

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

    AssetUUID matUUID;
    auto matIt = result.materialUUIDs.find(mesh->mMaterialIndex);
    if (matIt != result.materialUUIDs.end())
        matUUID = matIt->second;

    auto skelMeshIt = result.skeletalMeshUUIDs.find(globalMeshIndex);
    if (skelMeshIt != result.skeletalMeshUUIDs.end())
    {
        scene.AddComponent<SkeletalMeshComponent>(e, skelMeshIt->second);
        if (matUUID.IsValid())
            scene.AddComponent<MaterialComponent>(e, matUUID);
        return;
    }

    auto meshIt = result.meshUUIDs.find(globalMeshIndex);
    if (meshIt != result.meshUUIDs.end())
    {
        scene.AddComponent<MeshComponent>(e, meshIt->second);
        if (matUUID.IsValid())
            scene.AddComponent<MaterialComponent>(e, matUUID);
        return;
    }
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