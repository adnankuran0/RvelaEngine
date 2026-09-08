#pragma once
#include "Asset/IAssetImporter.h"
#include "MeshImporter.h"
#include "TextureImporter.h"
#include <Assimp/Importer.hpp>
#include "SkeletonImporter.h"
#include "SkeletalMeshImporter.h"
#include <entt/entt.h>
#include <unordered_map>
#include <unordered_set>

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;

namespace rv {

class Scene;
class AssetRegistry;

struct ModelImportResult
{
    AssetUUID prefabUUID;
    AssetUUID skeletonUUID;
    std::unordered_map<unsigned int, AssetUUID> meshUUIDs; // meshIndex - uuid
    std::unordered_map<unsigned int, AssetUUID> materialUUIDs; // matIndex - uuid
    std::unordered_map<std::string, AssetUUID>  textureUUIDs; // path - uuid
    std::unordered_map<unsigned int, AssetUUID> skeletalMeshUUIDs;
    std::unordered_map<std::string, int32_t>    oneNameToIndex;

    bool IsValid() const { return prefabUUID.IsValid(); }
};

struct ModelImportSettings
{
    float scale = 1.0f;
    bool importNormals = true;
    bool importTangents = true;
    bool importUVs = true;
    bool importTextures = true;
    bool importMaterials = true;
    bool generatePrefab = true;
};

class ModelImporter : public IAssetImporter
{
public:
    std::string GetImporterID() const override { return "ModelImporter"; }

    std::vector<std::string> GetSupportedExtensions() const override
    {
        return { ".fbx", ".obj", ".gltf", ".glb" };
    }

    std::filesystem::path GetCachePath(
        const std::filesystem::path& sourcePath,
        const AssetMeta& meta,
        const std::filesystem::path& cacheRoot) const override;

    bool Import(
        const std::filesystem::path& sourcePath,
        const std::filesystem::path& outCachePath,
        const std::string& settingsJson) override;

    ModelImportResult ImportModel(
        const std::filesystem::path& sourcePath,
        AssetRegistry& registry,
        const ModelImportSettings& settings = {});

    std::string GetDefaultSettings() const override;
    static ModelImportSettings ParseSettings(const std::string& settingsJson);

private:
    const aiScene* LoadScene(
        const std::filesystem::path& path,
        const ModelImportSettings& settings);

    void ExtractTextures(
        const aiScene* scene,
        const std::filesystem::path& modelPath,
        AssetRegistry& registry,
        ModelImportResult& result);

    void ExtractMaterials(
        const aiScene* scene,
        const std::filesystem::path& modelPath,
        AssetRegistry& registry,
        ModelImportResult& result);

    void ExtractMeshes(
        const aiScene* scene,
        const std::filesystem::path& modelPath,
        AssetRegistry& registry,
        ModelImportResult& result);

    void ExtractSkeleton(
        const aiScene* scene,
        const std::filesystem::path& modelPath,
        AssetRegistry& registry,
        ModelImportResult& result);

    AssetUUID ConstructPrefab(
        const aiScene* scene,
        const std::filesystem::path& modelPath,
        AssetRegistry& registry,
        const ModelImportResult& result);

    entt::entity ProcessNode(
        aiNode* node,
        const aiScene* modelScene,
        entt::entity parent,
        Scene& scene,
        const ModelImportResult& result);

    void AttachMeshToEntity(
        unsigned int meshIndex,
        aiNode* node,
        const aiScene* modelScene,
        Scene& scene,
        entt::entity e,
        const ModelImportResult& result);

    void SetTransformForEntity(
        aiNode* node,
        Scene& scene,
        entt::entity e);
    void MergeAndSaveSubAssets(
        const aiScene* scene,
        const std::filesystem::path& modelPath,
        AssetRegistry& registry,
        const ModelImportResult& result);
    std::filesystem::path ResolveRelativePath(
        const std::string& texPath,
        const std::filesystem::path& modelPath) const;

    static std::string SanitizeFilename(const std::string& name);

    MeshImporter m_MeshImporter;
    SkeletalMeshImporter m_SkeletalMeshImporter;
    TextureImporter m_TextureImporter;
    SkeletonImporter m_SkeletonImporter;

    Assimp::Importer m_Importer;
};

}