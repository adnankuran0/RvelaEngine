#include "rvelapch.h"
#include "AssetManager.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

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
    
    // MaterialID  MaterialIndex
    std::unordered_map<unsigned int, unsigned int> materials;

    if (scene->HasMaterials())
    {
        for (unsigned int i = 0; i < scene->mNumMaterials; i++)
        {
            auto& material = scene->mMaterials[i];
        }
    }

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


        meshDatas.push_back(data);
    }

    return meshDatas;
}
