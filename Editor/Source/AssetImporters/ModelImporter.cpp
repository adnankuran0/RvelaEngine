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
#include "Scene/Components/PrefabComponent.h"
#include "Core/Engine.h"
#include "Core/Log.h"
#include "Math/RvelaMath.h"
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>
#include <Assimp/GltfMaterial.h>
#include <nlohmann/json.hpp>
#include "MaterialSerializer.h"
#include <fstream>
#include <functional>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include "stb/stb_image.h"

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

static bool SubtreeHasMesh(const aiNode* node)
{
    if (!node)
        return false;
    if (node->mNumMeshes > 0)
        return true;
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        if (SubtreeHasMesh(node->mChildren[i]))
            return true;
    }
    return false;
}

static std::vector<std::string> GenerateCanonicalMeshNames(const aiScene* scene, const std::filesystem::path& modelPath)
{
    std::vector<std::string> names(scene->mNumMeshes);
    std::unordered_map<std::string, uint32_t> nameCounts;

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        std::string rawName = scene->mMeshes[i]->mName.C_Str();
        if (rawName.empty())
        {
            std::function<void(const aiNode*)> findNode = [&](const aiNode* node) {
                if (!rawName.empty()) return;
                for (unsigned int m = 0; m < node->mNumMeshes; ++m)
                {
                    if (node->mMeshes[m] == i && node->mName.length > 0)
                    {
                        rawName = node->mName.C_Str();
                        return;
                    }
                }
                for (unsigned int c = 0; c < node->mNumChildren; ++c)
                    findNode(node->mChildren[c]);
            };
            if (scene->mRootNode)
                findNode(scene->mRootNode);
        }

        if (rawName.empty())
        {
            rawName = "Mesh";
        }

        std::string clean = ModelImporter::SanitizeFilename(rawName);
        if (clean.empty()) clean = "Mesh";

        uint32_t count = nameCounts[clean]++;
        if (count == 0)
            names[i] = clean;
        else
            names[i] = clean + "_" + std::to_string(count);
    }

    return names;
}

static std::vector<std::string> GenerateCanonicalMaterialNames(const aiScene* scene, const std::filesystem::path& modelPath)
{
    std::vector<std::string> names(scene->mNumMaterials);
    std::unordered_map<std::string, uint32_t> nameCounts;

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* aiMat = scene->mMaterials[i];
        aiString aiName;
        std::string rawName;
        if (aiMat->Get(AI_MATKEY_NAME, aiName) == AI_SUCCESS && aiName.length > 0)
        {
            rawName = ModelImporter::SanitizeFilename(aiName.C_Str());
        }
        if (rawName.empty() || rawName == "DefaultMaterial")
        {
            rawName = "Mat" + std::to_string(i);
        }

        uint32_t count = nameCounts[rawName]++;
        if (count == 0)
            names[i] = rawName;
        else
            names[i] = rawName + "_" + std::to_string(count);
    }
    return names;
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
        result.prefabUUID = ConstructPrefab(scene, sourcePath, registry, result, settings);

    if (result.prefabUUID.IsValid())
        AssetManager::Get().Unload(result.prefabUUID);
    if (result.skeletonUUID.IsValid())
        AssetManager::Get().Unload(result.skeletonUUID);
    if (result.animLibUUID.IsValid())
        AssetManager::Get().Unload(result.animLibUUID);

    if (Engine::Get())
    {
        Scene& activeScene = Engine::Get()->GetActiveScene();

        if (result.prefabUUID.IsValid())
        {
            std::vector<entt::entity> prefabInstances;
            auto pView = activeScene.GetRegistry().view<PrefabComponent>();
            for (auto e : pView)
            {
                if (pView.get<PrefabComponent>(e).GetPrefabID() == result.prefabUUID)
                {
                    prefabInstances.push_back(e);
                }
            }
            for (auto inst : prefabInstances)
            {
                PrefabImporter::RevertPrefab(activeScene, inst, true);
            }
        }

        auto meshView = activeScene.GetRegistry().view<MeshComponent>();
        for (auto e : meshView)
        {
            auto& mc = meshView.get<MeshComponent>(e);
            for (const auto& [idx, uuid] : result.meshUUIDs)
            {
                if (mc.GetMeshID() == uuid)
                {
                    mc.SetMesh(uuid);
                    break;
                }
            }
        }

        auto skelMeshView = activeScene.GetRegistry().view<SkeletalMeshComponent>();
        for (auto e : skelMeshView)
        {
            auto& sc = skelMeshView.get<SkeletalMeshComponent>(e);
            for (const auto& [idx, uuid] : result.skeletalMeshUUIDs)
            {
                if (sc.GetMeshID() == uuid)
                {
                    sc.SetMesh(uuid);
                    break;
                }
            }
        }
    }

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
    parentMeta.subAssets.clear();

    std::vector<std::string> meshNames = GenerateCanonicalMeshNames(scene, modelPath);
    std::vector<std::string> matNames = GenerateCanonicalMaterialNames(scene, modelPath);

    for (auto& [meshIndex, meshUUID] : result.meshUUIDs)
    {
        SubAssetEntry entry;
        entry.uuid = meshUUID;
        entry.name = (meshIndex < meshNames.size()) ? meshNames[meshIndex] : (modelPath.stem().string() + "_Mesh" + std::to_string(meshIndex));
        entry.type = "Mesh";
        entry.index = meshIndex;
        entry.hasCache = true;
        parentMeta.subAssets.push_back(entry);
    }

    for (auto& [meshIndex, meshUUID] : result.skeletalMeshUUIDs)
    {
        SubAssetEntry entry;
        entry.uuid = meshUUID;
        entry.name = (meshIndex < meshNames.size()) ? meshNames[meshIndex] : (modelPath.stem().string() + "_SkeletalMesh" + std::to_string(meshIndex));
        entry.type = "SkeletalMesh";
        entry.index = meshIndex;
        entry.hasCache = true;
        parentMeta.subAssets.push_back(entry);
    }

    for (auto& [matIndex, matUUID] : result.materialUUIDs)
    {
        SubAssetEntry entry;
        entry.uuid = matUUID;
        entry.name = (matIndex < matNames.size()) ? matNames[matIndex] : (modelPath.stem().string() + "_Mat" + std::to_string(matIndex));
        entry.type = "Material";
        entry.index = matIndex;
        entry.hasCache = false;
        parentMeta.subAssets.push_back(entry);
    }

    if (result.skeletonUUID.IsValid())
    {
        SubAssetEntry entry;
        entry.uuid = result.skeletonUUID;
        entry.name = modelPath.stem().string() + "_Skeleton";
        entry.type = "Skeleton";
        entry.index = 0;
        entry.hasCache = false;
        parentMeta.subAssets.push_back(entry);
    }

    if (result.animLibUUID.IsValid())
    {
        SubAssetEntry entry;
        entry.uuid = result.animLibUUID;
        entry.name = modelPath.stem().string() + "_AnimLib";
        entry.type = "AnimationLib";
        entry.index = 0;
        entry.hasCache = false;
        parentMeta.subAssets.push_back(entry);
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

    if (settings.scale != 1.0f)
    {
        flags |= aiProcess_GlobalScale;
    }

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
            //LOG_INFO("Importing texture: {}", texPath.filename().string());

            if (m_TextureImporter.Import(texPath, cachePath, meta.importerSettingsJson))
            {
                meta.importerID = m_TextureImporter.GetImporterID();
                meta.lastWriteTime = currentTimeStamp;
                registry.SaveMeta(texPath, meta);
                AssetManager::Get().Unload(meta.uuid);
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

    // Extract all embedded textures
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
            bool shouldWrite = !std::filesystem::exists(outFilePath);
            if (!shouldWrite && embedded->mHeight == 0)
            {
                std::error_code ec;
                uintmax_t existingSize = std::filesystem::file_size(outFilePath, ec);
                if (ec || existingSize != embedded->mWidth)
                {
                    shouldWrite = true;
                }
                else
                {
                    std::ifstream in(outFilePath, std::ios::binary);
                    if (in)
                    {
                        std::vector<char> buf(embedded->mWidth);
                        in.read(buf.data(), embedded->mWidth);
                        if (memcmp(buf.data(), embedded->pcData, embedded->mWidth) != 0)
                            shouldWrite = true;
                    }
                }
            }

            if (shouldWrite)
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

    // Collect external textures from materials
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

    std::vector<std::string> matNames = GenerateCanonicalMaterialNames(scene, modelPath);
    std::unordered_set<AssetUUID> claimedMatUUIDs;
    std::vector<AssetUUID> assignedMatUUIDs(scene->mNumMaterials);

    // match by subtype and canonical subname
    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        for (const auto& sub : parentMeta.subAssets)
        {
            if (sub.type == "Material" && sub.name == matNames[i] && !claimedMatUUIDs.contains(sub.uuid))
            {
                assignedMatUUIDs[i] = sub.uuid;
                claimedMatUUIDs.insert(sub.uuid);
                break;
            }
        }
    }

    // fallback
    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        if (assignedMatUUIDs[i].IsValid()) continue;
        for (const auto& sub : parentMeta.subAssets)
        {
            if (sub.type == "Material" && sub.index == i && !claimedMatUUIDs.contains(sub.uuid))
            {
                assignedMatUUIDs[i] = sub.uuid;
                claimedMatUUIDs.insert(sub.uuid);
                break;
            }
        }
    }

    // new UUIDs
    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        if (!assignedMatUUIDs[i].IsValid())
        {
            assignedMatUUIDs[i] = AssetUUID{};
        }
    }

    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* aiMat = scene->mMaterials[i];

        std::string matFileName = modelPath.stem().string() + "_" + matNames[i] + ".rmat";
        auto matFilePath = modelPath.parent_path() / matFileName;
        auto legacyPath = modelPath.parent_path() / (modelPath.stem().string() + "_Mat" + std::to_string(i) + ".rmat");
        if (!std::filesystem::exists(matFilePath) && std::filesystem::exists(legacyPath))
            matFilePath = legacyPath;

        AssetUUID matUUID = assignedMatUUIDs[i];

        AssetMeta meta = registry.GetOrCreateMeta(matFilePath);
        meta.uuid = matUUID;
        meta.importerID = "MaterialLoader";
        registry.SaveMeta(matFilePath, meta);

        auto asset = CreateRef<MaterialAsset>(meta.uuid);
        ai_real value;

        // Albedo
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

        // Normal
        auto [normalPath, normalUUID] = GetTextureInfo(aiMat, aiTextureType_NORMALS);
        if (!normalUUID.IsValid())
            std::tie(normalPath, normalUUID) = GetTextureInfo(aiMat, aiTextureType_HEIGHT);

        if (normalUUID.IsValid())
        {
            asset->normalTextureUUID = normalUUID;
            asset->useNormalMap = true;
        }

        // Metallic, Roughness, AO & ARM
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

        // Transparency
        bool hasAlpha = false;

        // glTF alphaMode
        aiString alphaMode;
        if (aiMat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
        {
            std::string modeStr = alphaMode.C_Str();
            if (modeStr == "BLEND")
            {
                asset->transparencyMode = TransparencyMode::Alpha;
                hasAlpha = true;
            }
            else if (modeStr == "MASK")
            {
                asset->transparencyMode = TransparencyMode::AlphaScissor;
                hasAlpha = true;
                ai_real cutoff = 0.5f;
                if (aiGetMaterialFloat(aiMat, AI_MATKEY_GLTF_ALPHACUTOFF, &cutoff) == AI_SUCCESS)
                {
                    asset->alphaCutoff = static_cast<float>(cutoff);
                }
            }
        }

        // Opacity
        if (!hasAlpha)
        {
            ai_real opacity = 1.0f;
            if (aiGetMaterialFloat(aiMat, AI_MATKEY_OPACITY, &opacity) == AI_SUCCESS && opacity < 0.999f)
            {
                asset->transparencyMode = TransparencyMode::Alpha;
                asset->albedoColor.a = static_cast<float>(opacity);
                hasAlpha = true;
            }
        }

        if (!hasAlpha)
        {
            ai_real transFactor = 0.0f;
            if (aiGetMaterialFloat(aiMat, AI_MATKEY_TRANSPARENCYFACTOR, &transFactor) == AI_SUCCESS && transFactor > 0.001f)
            {
                asset->transparencyMode = TransparencyMode::Alpha;
                asset->albedoColor.a = 1.0f - static_cast<float>(transFactor);
                hasAlpha = true;
            }
        }

        if (!hasAlpha)
        {
            ai_real transmission = 0.0f;
            if (aiGetMaterialFloat(aiMat, AI_MATKEY_TRANSMISSION_FACTOR, &transmission) == AI_SUCCESS && transmission > 0.001f)
            {
                asset->transparencyMode = TransparencyMode::Alpha;
                hasAlpha = true;
            }
        }

        // Opacity map texture
        if (!hasAlpha && aiMat->GetTextureCount(aiTextureType_OPACITY) > 0)
        {
            asset->transparencyMode = TransparencyMode::Alpha;
            hasAlpha = true;
        }

        // Texture Flags
        if (!hasAlpha)
        {
            int texFlags = 0;
            if (aiMat->Get(AI_MATKEY_TEXFLAGS(aiTextureType_DIFFUSE, 0), texFlags) == AI_SUCCESS ||
                aiMat->Get(AI_MATKEY_TEXFLAGS(aiTextureType_BASE_COLOR, 0), texFlags) == AI_SUCCESS)
            {
                if (texFlags & aiTextureFlags_UseAlpha)
                {
                    asset->transparencyMode = TransparencyMode::Alpha;
                    hasAlpha = true;
                }
            }
        }

        // Diffuse
        if (!hasAlpha && asset->albedoColor.a < 0.999f)
        {
            asset->transparencyMode = TransparencyMode::Alpha;
            hasAlpha = true;
        }

        // Check if the albedo texture file contains actual alpha pixels
        if (!hasAlpha && asset->useAlbedoMap && albedoUUID.IsValid())
        {
            std::filesystem::path texFilePath = registry.GetPath(albedoUUID);
            if (texFilePath.empty() || !std::filesystem::exists(texFilePath))
            {
                for (const auto& [key, uuid] : result.textureUUIDs)
                {
                    if (uuid == albedoUUID && std::filesystem::exists(key))
                    {
                        texFilePath = key;
                        break;
                    }
                }
            }

            if (!texFilePath.empty() && std::filesystem::exists(texFilePath))
            {
                int tw = 0, th = 0, tc = 0;
                if (stbi_info(texFilePath.string().c_str(), &tw, &th, &tc) && tc == 4)
                {
                    uint8_t* pix = stbi_load(texFilePath.string().c_str(), &tw, &th, &tc, 4);
                    if (pix)
                    {
                        for (int p = 0; p < tw * th; ++p)
                        {
                            if (pix[p * 4 + 3] < 250)
                            {
                                asset->transparencyMode = TransparencyMode::Alpha;
                                hasAlpha = true;
                                break;
                            }
                        }
                        stbi_image_free(pix);
                    }
                }
            }
        }

        // Blend Mode & Two-Sided
        int blendFunc = 0;
        if (aiGetMaterialInteger(aiMat, AI_MATKEY_BLEND_FUNC, &blendFunc) == AI_SUCCESS)
        {
            if (blendFunc == aiBlendMode_Additive)
                asset->blendMode = BlendMode::Add;
        }

        int twoSided = 0;
        if (aiGetMaterialInteger(aiMat, AI_MATKEY_TWOSIDED, &twoSided) == AI_SUCCESS && twoSided != 0)
        {
            asset->cullMode = CullMode::Disabled;
        }

        if (!MaterialSerializer::Save(asset, matFilePath))
        {
            LOG_ERROR("Failed to write material: {}", matFilePath.string());
            continue;
        }

        AssetManager::Get().Unload(meta.uuid);
        result.materialUUIDs[i] = meta.uuid;
    }

    // Unload any deleted materials
    for (const auto& sub : parentMeta.subAssets)
    {
        if (sub.type == "Material" && !claimedMatUUIDs.contains(sub.uuid))
        {
            AssetManager::Get().Unload(sub.uuid);
        }
    }
}

void ModelImporter::ExtractMeshes(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    ModelImportResult& result)
{
    if (!scene || scene->mNumMeshes == 0)
        return;

    std::vector<std::string> meshNames = GenerateCanonicalMeshNames(scene, modelPath);
    AssetMeta parentMeta = registry.GetOrCreateMeta(modelPath);

    std::unordered_set<AssetUUID> claimedUUIDs;
    std::vector<AssetUUID> assignedUUIDs(scene->mNumMeshes);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];
        std::string expectedType = (mesh->mNumBones > 0 && !result.boneNameToIndex.empty()) ? "SkeletalMesh" : "Mesh";
        for (const auto& sub : parentMeta.subAssets)
        {
            if (sub.type == expectedType && sub.name == meshNames[i] && !claimedUUIDs.contains(sub.uuid))
            {
                assignedUUIDs[i] = sub.uuid;
                claimedUUIDs.insert(sub.uuid);
                break;
            }
        }
    }

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        if (assignedUUIDs[i].IsValid()) continue;
        aiMesh* mesh = scene->mMeshes[i];
        std::string expectedType = (mesh->mNumBones > 0 && !result.boneNameToIndex.empty()) ? "SkeletalMesh" : "Mesh";
        for (const auto& sub : parentMeta.subAssets)
        {
            if (sub.type == expectedType && sub.index == i && !claimedUUIDs.contains(sub.uuid))
            {
                assignedUUIDs[i] = sub.uuid;
                claimedUUIDs.insert(sub.uuid);
                break;
            }
        }
    }

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        if (!assignedUUIDs[i].IsValid())
        {
            assignedUUIDs[i] = AssetUUID{};
        }
        aiMesh* mesh = scene->mMeshes[i];
        if (mesh->mNumBones > 0 && !result.boneNameToIndex.empty())
            result.skeletalMeshUUIDs[i] = assignedUUIDs[i];
        else
            result.meshUUIDs[i] = assignedUUIDs[i];
    }

    for (const auto& sub : parentMeta.subAssets)
    {
        if ((sub.type == "Mesh" || sub.type == "SkeletalMesh") && !claimedUUIDs.contains(sub.uuid))
        {
            auto cachePath = registry.GetPath(sub.uuid);
            if (!cachePath.empty() && std::filesystem::exists(cachePath))
            {
                std::error_code ec;
                std::filesystem::remove(cachePath, ec);
            }
            AssetManager::Get().Unload(sub.uuid);
        }
    }

    m_MeshImporter.ImportFromScene(scene, modelPath, registry, result.meshUUIDs, meshNames);

    if (!result.boneNameToIndex.empty())
    {
        m_SkeletalMeshImporter.ImportFromScene(
            scene, modelPath, registry, result.boneNameToIndex, result.skeletalMeshUUIDs, meshNames);
    }
}

AssetUUID ModelImporter::ConstructPrefab(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    const ModelImportResult& result,
    const ModelImportSettings& settings)
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
        if (result.animLibUUID.IsValid())
            prefabScene.AddComponent<AnimatorComponent>(rootEntity, result.animLibUUID);
        else
            prefabScene.AddComponent<AnimatorComponent>(rootEntity);
    }
    else if (result.animLibUUID.IsValid())
    {
        prefabScene.AddComponent<AnimatorComponent>(rootEntity, result.animLibUUID);
    }

    if (scene->mRootNode)
    {
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
                prefabScene.SetParentKeepLocal(child, rootEntity);
            }
        }

        for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; ++i)
        {
            ProcessNode(scene->mRootNode->mChildren[i], scene, rootEntity, prefabScene, result, boneNames);
        }
    }

    std::string ext = modelPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".fbx")
    {
        auto& tc = prefabScene.GetComponent<TransformComponent>(rootEntity);
        tc.SetScale(glm::vec3(0.01f * settings.scale));
    }
    else if (settings.scale != 1.0f)
    {
        auto& tc = prefabScene.GetComponent<TransformComponent>(rootEntity);
        tc.SetScale(glm::vec3(settings.scale));
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
    if (!node)
        return entt::null;

    std::string nodeName = node->mName.C_Str();
    if (nodeName.empty())
        nodeName = "Node";

    if (boneNames.contains(nodeName) && node->mNumMeshes == 0)
    {
        if (!SubtreeHasMesh(node))
        {
            return entt::null;
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], modelScene, parent, scene, result, boneNames);
        }
        return entt::null;
    }

    entt::entity e = scene.CreateEntity(nodeName).GetHandle();
    SetTransformForEntity(node, scene, e);

    if (parent != entt::null)
        scene.SetParentKeepLocal(e, parent);

    if (node->mNumMeshes == 1)
    {
        AttachMeshToEntity(0, node, modelScene, scene, e, result);
    }
    else if (node->mNumMeshes > 1)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            entt::entity child = scene.CreateEntity(nodeName + "_" + std::to_string(i)).GetHandle();
            AttachMeshToEntity(i, node, modelScene, scene, child, result);
            scene.SetParentKeepLocal(child, e);
        }
    }

    entt::entity parentForChildren = (e != entt::null) ? e : parent;
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], modelScene, parentForChildren, scene, result, boneNames);
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
        auto& smc = scene.AddComponent<SkeletalMeshComponent>(e, skelMeshIt->second);
        scene.AddComponent<SkeletalMeshRendererComponent>(e, smc.GetMesh());
        if (matUUID.IsValid())
            scene.AddComponent<MaterialComponent>(e, matUUID);
        else
            scene.AddComponent<MaterialComponent>(e);
        return;
    }

    auto meshIt = result.meshUUIDs.find(globalMeshIndex);
    if (meshIt != result.meshUUIDs.end())
    {
        auto& mc = scene.AddComponent<MeshComponent>(e, meshIt->second);
        scene.AddComponent<MeshRendererComponent>(e, mc.GetMesh());
        if (matUUID.IsValid())
            scene.AddComponent<MaterialComponent>(e, matUUID);
        else
            scene.AddComponent<MaterialComponent>(e);
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
    glm::vec3 pos, scale;
    glm::quat rotation;
    math::DecomposeToQuat(transform, scale, rotation, pos);
    tc.SetPosition(pos);
    tc.SetScale(scale);
    tc.SetRotation(rotation);
}