#include "rvelapch.h"
#include "AssetManager.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

void AssetManager::LoadAsset(const std::string& path, ModelComponent& model)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate
        | aiProcess_FlipUVs
        | aiProcess_GenSmoothNormals
        );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "Model could not loaded!\n";
        return;
    }

    std::cout << "model has " << scene->mNumMeshes << " meshes\n";

    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        
        std::vector<float> vertices;
        //vertices.reserve(mesh->mNumVertices * 8); 

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

        std::cout << "Asset loaded with " << mesh->mNumVertices << " unique vertices and "
            << indices.size() << " indices.\n";

        model.meshes.emplace_back(vertices.data(), vertices.size() * sizeof(float),
            indices.data(), indices.size() * sizeof(unsigned int));
    
	}

}

