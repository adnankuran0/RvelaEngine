#include "rvelapch.h"
#include "AssetManager.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "Core/Utils/MaterialManager.h"
#include <filesystem>
#include <mutex>

//TODO: Move this function to its own place
bool doesFileExist(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

void FindNodeForMesh(aiNode* node, unsigned int meshIndex, std::string& meshName)
{
    if (!node) return;

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        if (node->mMeshes[i] == meshIndex) {
            meshName = node->mName.C_Str();
            return;
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        FindNodeForMesh(node->mChildren[i], meshIndex, meshName);
    }
}

fs::path AssetManager::FindTexturePath(const std::string& modelPath, const aiString& texPath)
{
    std::vector<std::string> possibleExtensions = { ".png", ".jpg", ".jpeg", ".tga", ".bmp" };

    fs::path modelFsPath(modelPath);
    fs::path modelDir = modelFsPath.parent_path();

    fs::path textureFullPath = modelDir / texPath.C_Str();
    textureFullPath = fs::weakly_canonical(textureFullPath);

    if (fs::exists(textureFullPath))
        return textureFullPath;

    fs::path textureDir = textureFullPath.parent_path();
    std::string textureNameStem = textureFullPath.stem().string();

    for (const auto& ext : possibleExtensions)
    {
        fs::path altPath = textureDir / (textureNameStem + ext);
        if (fs::exists(altPath))
            return altPath;
    }

    std::cout << "Texture not found: " << textureFullPath << std::endl;
    return "";
}

void AssetManager::LoadMaterials(const aiScene* scene, std::unordered_map<unsigned int, std::string>& materials, const std::string& modelPath)
{
    if (!scene->HasMaterials()) return;

    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* aiMat = scene->mMaterials[i];
        MaterialData materialData;



        aiString name;
        if (AI_FAILURE == aiMat->Get(AI_MATKEY_NAME, name))
            std::cout << "Can't load material!" << std::endl;

        
        

        aiColor4D color;
        if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color))
            materialData.albedoColor = glm::vec4(color.r, color.g, color.b, color.a);
        else
            materialData.albedoColor = glm::vec4(1.0f);

        struct TextureTarget {
            aiTextureType type;
            std::string& destination;
        };

        aiString texPath;

        std::string albedoPath, normalPath, roughnessPath, metallicPath ,aoPath, heightPath;

        std::vector<TextureTarget> targets = {
            { aiTextureType_DIFFUSE, albedoPath },
            { aiTextureType_NORMALS, normalPath },
            { aiTextureType_SHININESS, roughnessPath },
            { aiTextureType_METALNESS, metallicPath },
            { aiTextureType_LIGHTMAP, aoPath },
            { aiTextureType_HEIGHT, heightPath }
        };

        //FIX: Assimp cant find ao textures


        for (auto& target : targets)
        {
            if (AI_SUCCESS == aiMat->GetTexture(target.type, 0, &texPath))
            {
                fs::path foundPath = FindTexturePath(modelPath, texPath);
                if (!foundPath.empty())
                {
                    std::string nicePath = foundPath.string();
                    std::replace(nicePath.begin(), nicePath.end(), '\\', '/');
                    target.destination = nicePath;
                }
            }
        }

        materialData.albedoMapPath = albedoPath;
        materialData.normalMapPath = normalPath;
        materialData.roughnessMapPath = roughnessPath;
        materialData.metallicMapPath = metallicPath;
        materialData.aoMapPath = aoPath;
        materialData.heightMapPath = heightPath;

        std::string matSavePath = "D:/GitHub/RvelaEngine/Resources/Engine/Materials/" + std::string(name.C_Str()) + ".rmaterial";
        materials[i] = matSavePath;
        
        MaterialManager::CreateMaterial(matSavePath, materialData);
        
    }

}

std::vector<MeshData> AssetManager::LoadModel(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate
        | aiProcess_FlipUVs
        | aiProcess_GenSmoothNormals
    );


    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "Model could not be loaded!\n";
    }
    
    // MaterialIndex  Path
    std::unordered_map<unsigned int, std::string> materials;

    LoadMaterials(scene, materials, path);

    std::vector<MeshData> meshDatas;
    meshDatas.reserve(scene->mNumMeshes);

    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];

        std::vector<float> vertices;
        vertices.reserve(mesh->mNumVertices * 8);

        for (unsigned int j = 0; j < mesh->mNumVertices; j++)
        {
            vertices.push_back(mesh->mVertices[j].x);
            vertices.push_back(mesh->mVertices[j].y);
            vertices.push_back(mesh->mVertices[j].z);

            if (mesh->HasNormals())
            {
                vertices.push_back(mesh->mNormals[j].x);
                vertices.push_back(mesh->mNormals[j].y);
                vertices.push_back(mesh->mNormals[j].z);
            }
            else
            {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }

            if (mesh->mTextureCoords[0])
            {
                vertices.push_back(mesh->mTextureCoords[0][j].x);
                vertices.push_back(mesh->mTextureCoords[0][j].y);
            }
            else
            {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
        }

        std::vector<unsigned int> indices;
        indices.reserve(mesh->mNumFaces * 3);

        for (unsigned int j = 0; j < mesh->mNumFaces; j++)
        {
            aiFace face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++)
            {
                indices.push_back(face.mIndices[k]);
            }
        }

        

        std::string meshName = "Unnamed"; 
        FindNodeForMesh(scene->mRootNode, i, meshName); 

        MeshData data;
        data.vertices = vertices;
        data.indices = indices;
        data.indexCount = indices.size();
        data.name = meshName; 
        data.materialPath = materials[mesh->mMaterialIndex];
        data.meshIndex = i;

        meshDatas.push_back(data);
    }

    return meshDatas;
}

MeshData AssetManager::LoadMesh(const std::string& modelPath, uint32_t meshIndex)
{
    struct ModelCacheEntry {
        std::unique_ptr<Assimp::Importer> importer;
        const aiScene* scene = nullptr;
        std::unordered_map<unsigned int, std::string> materials;
        std::vector<std::string> meshNames;
    };

    static std::unordered_map<std::string, ModelCacheEntry> modelCache;
    static std::mutex cacheMutex;

    std::unique_lock<std::mutex> lock(cacheMutex);
    auto& cacheEntry = modelCache[modelPath];

    if (!cacheEntry.scene) {
        auto importer = std::make_unique<Assimp::Importer>();
        const aiScene* scene = importer->ReadFile(modelPath,
            aiProcess_Triangulate
            | aiProcess_FlipUVs
            | aiProcess_GenSmoothNormals
        );

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            modelCache.erase(modelPath);
            std::cerr << "Failed to load model: " << modelPath << "\n";
            return {};
        }

        cacheEntry.importer = std::move(importer);
        cacheEntry.scene = scene;

        LoadMaterials(scene, cacheEntry.materials, modelPath);

        cacheEntry.meshNames.resize(scene->mNumMeshes);
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            FindNodeForMesh(scene->mRootNode, i, cacheEntry.meshNames[i]);
        }
    }

    const aiScene* scene = cacheEntry.scene;
    if (meshIndex >= scene->mNumMeshes) {
        std::cerr << "Invalid mesh index: " << meshIndex << " for model: " << modelPath << "\n";
        return {};
    }

    aiMesh* mesh = scene->mMeshes[meshIndex];

    const bool hasNormals = mesh->HasNormals();
    const bool hasTexCoords = mesh->mTextureCoords[0] != nullptr;
    const size_t vertexSize = 3 + (hasNormals ? 3 : 3) + (hasTexCoords ? 2 : 2);

    std::vector<float> vertices;
    vertices.reserve(mesh->mNumVertices * vertexSize);

    for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
        const aiVector3D& pos = mesh->mVertices[j];
        vertices.insert(vertices.end(), { pos.x, pos.y, pos.z });

        if (hasNormals) {
            const aiVector3D& normal = mesh->mNormals[j];
            vertices.insert(vertices.end(), { normal.x, normal.y, normal.z });
        }
        else {
            vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
        }

        if (hasTexCoords) {
            const aiVector3D& texCoord = mesh->mTextureCoords[0][j];
            vertices.insert(vertices.end(), { texCoord.x, texCoord.y });
        }
        else {
            vertices.insert(vertices.end(), { 0.0f, 0.0f });
        }
    }

    std::vector<unsigned int> indices;
    indices.reserve(mesh->mNumFaces * 3);

    for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
        const aiFace& face = mesh->mFaces[j];
        indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }

    MeshData meshData;
    meshData.vertices = std::move(vertices);
    meshData.indices = std::move(indices);
    meshData.indexCount = meshData.indices.size();
    meshData.name = cacheEntry.meshNames[meshIndex];
    meshData.materialPath = cacheEntry.materials[mesh->mMaterialIndex];
    meshData.meshIndex = meshIndex;

    return meshData;
}
