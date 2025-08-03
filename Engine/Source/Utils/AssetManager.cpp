#include "rvelapch.h"
#include "AssetManager.h"
#include "FileUtils.h"
#include "Core/Log.h"

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

fs::path AssetManager::FindTexturePath(const Path modelPath, const aiString& texPath)
{
    std::vector<std::string> possibleExtensions = { ".png", ".jpg", ".jpeg", ".tga", ".bmp" };

    fs::path modelFsPath(modelPath.GetAbsoluteStr());
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

    LOG_WARN("Texture not found: {}", textureFullPath.string());
    return "";
}

void AssetManager::LoadMaterials(const aiScene* scene, std::unordered_map<unsigned int, std::string>& materials, const Path modelPath)
{
    if (!scene->HasMaterials()) return;

#if 0
    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* aiMat = scene->mMaterials[i];
        MaterialData materialData;

        aiString name;
        if (AI_FAILURE == aiMat->Get(AI_MATKEY_NAME, name))
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1000, 9999);
            int randomNum = dis(gen);

            std::string fallbackName = "UnnamedMaterial_" + std::to_string(randomNum);
            name = aiString(fallbackName.c_str());

            LOG_WARN("Can't load material! Assigned random name: {}", name.C_Str());;
        }

        
        

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


        materialData.albedoMapPath = VRT_PATH(albedoPath);
        materialData.normalMapPath = VRT_PATH(normalPath);
        materialData.roughnessMapPath = VRT_PATH(roughnessPath);
        materialData.metallicMapPath = VRT_PATH(metallicPath);
        materialData.aoMapPath = VRT_PATH(aoPath);
        materialData.heightMapPath = VRT_PATH(heightPath);

        Path matSavePath = VRT_PATH("Assets/Materials/" +
            std::string(name.C_Str()) + ".rmaterial").GetAbsolute();
        materials[i] = matSavePath.GetAbsoluteStr();
        
        //MaterialManager::CreateMaterial(matSavePath, materialData);
        
    }
#endif
}

std::vector<MeshData> AssetManager::LoadModel(const Path path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.GetAbsoluteStr(),
        aiProcess_Triangulate
        | aiProcess_FlipUVs
        | aiProcess_GenSmoothNormals
    );


    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_WARN("Model could not be loaded!");
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

        glm::vec3 minBounds(FLT_MAX);
        glm::vec3 maxBounds(-FLT_MAX);

        for (unsigned int j = 0; j < mesh->mNumVertices; j++)
        {
            const aiVector3D& pos = mesh->mVertices[j];
            vertices.insert(vertices.end(), { pos.x, pos.y, pos.z });

            //Calculate bounds for AABB
            minBounds.x = std::min(minBounds.x, pos.x);
            minBounds.y = std::min(minBounds.y, pos.y);
            minBounds.z = std::min(minBounds.z, pos.z);

            maxBounds.x = std::max(maxBounds.x, pos.x);
            maxBounds.y = std::max(maxBounds.y, pos.y);
            maxBounds.z = std::max(maxBounds.z, pos.z);

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

        BoundingBox localAABB(minBounds, maxBounds);

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
        data.indexCount = (unsigned int) indices.size();
        data.name = meshName; 
        data.materialPath = VRT_PATH(materials[mesh->mMaterialIndex]);
        data.meshIndex = i;
        data.localAABB = localAABB;

        meshDatas.push_back(data);
    }

    return meshDatas;
}

MeshData AssetManager::LoadMesh(const Path modelPath, uint32_t meshIndex)
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
    auto& cacheEntry = modelCache[modelPath.GetAbsoluteStr()];

    if (!cacheEntry.scene) {
        auto importer = std::make_unique<Assimp::Importer>();
        const aiScene* scene = importer->ReadFile(modelPath.GetAbsoluteStr(),
            aiProcess_Triangulate
            | aiProcess_FlipUVs
            | aiProcess_GenSmoothNormals
        );

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            modelCache.erase(modelPath.GetAbsoluteStr());
            LOG_ERROR("Failed to load model: {}" ,modelPath.GetAbsoluteStr());
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
        LOG_ERROR("Invalid mesh index: {} for model: {}", meshIndex, modelPath.GetAbsoluteStr());
        return {};
    }

    aiMesh* mesh = scene->mMeshes[meshIndex];

    const bool hasNormals = mesh->HasNormals();
    const bool hasTexCoords = mesh->mTextureCoords[0] != nullptr;
    const size_t vertexSize = 3 + (hasNormals ? 3 : 3) + (hasTexCoords ? 2 : 2);

    std::vector<float> vertices;
    vertices.reserve(mesh->mNumVertices * vertexSize);

    glm::vec3 minBounds(FLT_MAX);
    glm::vec3 maxBounds(-FLT_MAX);

    for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
        const aiVector3D& pos = mesh->mVertices[j];
        vertices.insert(vertices.end(), { pos.x, pos.y, pos.z });

        //Calculate bounds for AABB
        minBounds.x = std::min(minBounds.x, pos.x);
        minBounds.y = std::min(minBounds.y, pos.y);
        minBounds.z = std::min(minBounds.z, pos.z);

        maxBounds.x = std::max(maxBounds.x, pos.x);
        maxBounds.y = std::max(maxBounds.y, pos.y);
        maxBounds.z = std::max(maxBounds.z, pos.z);

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

    BoundingBox localAABB(minBounds, maxBounds);

    std::vector<unsigned int> indices;
    indices.reserve(mesh->mNumFaces * 3);

    for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
        const aiFace& face = mesh->mFaces[j];
        indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }

    MeshData meshData;
    meshData.vertices = std::move(vertices);
    meshData.indices = std::move(indices);
    meshData.indexCount = (unsigned int) meshData.indices.size();
    meshData.name = cacheEntry.meshNames[meshIndex];
    meshData.materialPath = VRT_PATH(cacheEntry.materials[mesh->mMaterialIndex]);
    meshData.meshIndex = meshIndex;
    meshData.localAABB = localAABB;

    return meshData;
}
