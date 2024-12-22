#pragma once
#include <string>
#include "glm/glm.hpp"
#include "../Renderer/VertexArray.h"
#include "../Renderer/VertexBuffer.h"
#include "../Renderer/ElementBuffer.h"
#include "../Renderer/BufferLayout.h"
#include "../Renderer/Texture.h"
#include "../Renderer/Shader.h"



struct TransformComponent {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    TransformComponent() = default;
    TransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
    {
        position = pos;
        rotation = rot;
        scale = scl;
    }
};

struct NameComponent {
    std::string name;
};

struct MeshComponent {
    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;
    BufferLayout Layout;
    MeshComponent() = default;
    MeshComponent(void* vertices,size_t sizeOfVertices,void* indices,size_t sizeOfIndices) 
    { 
        VAO.Bind();
        VBO.Init(vertices, sizeOfVertices);
        VBO.Bind();
        Layout.BindVertexBuffer(VBO.getID());
        Layout.Push<float>(3);
        Layout.Push<float>(3);
        Layout.Push<float>(2);
        VAO.SetBufferLayout(&Layout);
        EBO.Init(indices, sizeOfIndices);
        EBO.Bind();
    }
};

struct MetarialComponent {
    Shader  shader;
    Texture Albedo;
    Texture Normal;
    Texture Metallic;
    Texture Roughness;
    Texture Ao;
    Texture Height;

    MetarialComponent() = default;
    MetarialComponent(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const std::string texturesPath)
    {
        shader.Init(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
        Albedo.Init();
        Albedo.GenerateFromImage(texturesPath+"/albedo.png");
        Normal.Init();
        Normal.GenerateFromImage(texturesPath + "/normal.png");
        Metallic.Init();
        Metallic.GenerateFromImage(texturesPath + "/metallic.png");
        Roughness.Init();
        Roughness.GenerateFromImage(texturesPath + "/roughness.png");
        Ao.Init();
        Ao.GenerateFromImage(texturesPath + "/ao.png");
        Height.Init();
        Height.GenerateFromImage(texturesPath + "/height.png");
    }
};