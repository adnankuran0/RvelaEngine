#pragma once
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "../Renderer/VertexArray.h"
#include "../Renderer/VertexBuffer.h"
#include "../Renderer/ElementBuffer.h"
#include "../Renderer/BufferLayout.h"
#include "../Renderer/Texture.h"
#include "../Renderer/Shader.h"
#include "entt/entt.h"

struct WorldTransformComponent {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    WorldTransformComponent() = default;
    WorldTransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
    {
        position = pos;
        rotation = rot;
        scale = scl;
    }
    glm::mat4 GetMatrix()
    {
        glm::mat4 mat = glm::mat4(1.0f);

        float roll_rad = glm::radians(rotation.x);
        float pitch_rad = glm::radians(rotation.y);
        float yaw_rad = glm::radians(rotation.z);

        glm::quat qx = glm::angleAxis(roll_rad, glm::vec3(1, 0, 0));
        glm::quat qy = glm::angleAxis(pitch_rad, glm::vec3(0, 1, 0));
        glm::quat qz = glm::angleAxis(yaw_rad, glm::vec3(0, 0, 1));

        glm::quat quat = qy * qx * qz;

        mat = glm::translate(mat, position);
        mat *= glm::mat4_cast(quat);
        mat = glm::scale(mat, scale);
        return mat;
    }
};


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
    glm::mat4 GetMatrix()
    {
        glm::mat4 mat = glm::mat4(1.0f);

        float roll_rad = glm::radians(rotation.x);
        float pitch_rad = glm::radians(rotation.y);
        float yaw_rad = glm::radians(rotation.z);

        glm::quat qx = glm::angleAxis(roll_rad, glm::vec3(1, 0, 0));
        glm::quat qy = glm::angleAxis(pitch_rad, glm::vec3(0, 1, 0));
        glm::quat qz = glm::angleAxis(yaw_rad, glm::vec3(0, 0, 1));

        glm::quat quat = qy * qx * qz;

        mat = glm::translate(mat, position);
        mat *= glm::mat4_cast(quat);
        mat = glm::scale(mat, scale);
        return mat;
    }
};

struct TagComponent {
    std::string tag;
};

struct MeshComponent {
    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;
    BufferLayout Layout;
    MeshComponent() = default;
    MeshComponent(const MeshComponent&) = delete;
    MeshComponent& operator=(const MeshComponent&) = delete;
    MeshComponent(MeshComponent&&) = default;
    MeshComponent& operator=(MeshComponent&&) = default;
    MeshComponent(void* vertices,size_t sizeOfVertices,void* indices,size_t sizeOfIndices) 
    { 
        VAO.Bind();
        VBO.Init(vertices, sizeOfVertices);
        VBO.Bind();
        Layout.BindVertexBuffer(VBO.getID());
        Layout.Push<float>(3);
        Layout.Push<float>(3);
        Layout.Push<float>(2);
        VAO.SetBufferLayout(Layout);
        EBO.Init(indices, sizeOfIndices);
        EBO.Bind();
    }

    void Destroy()
    {
        VAO.Destroy();
        VBO.Destroy();
        EBO.Destroy();
    }
};

struct MaterialComponent {
    Shader  shader;
    
    Texture Albedo;
    Texture Normal;
    Texture Metallic;
    Texture Roughness;
    Texture Ao;
    Texture Height;
    

    MaterialComponent() = default;
    MaterialComponent(const MaterialComponent&) = delete;
    MaterialComponent& operator=(const MaterialComponent&) = delete;
    MaterialComponent(MaterialComponent&&) = default;
    MaterialComponent& operator=(MaterialComponent&&) = default;
    MaterialComponent(const std::string& vertexShaderPath, const std::string& fragmentShaderPath, const std::string& texturesPath)
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

    void Destroy()
    {   
        
        Albedo.Destroy();
        Normal.Destroy();
        Metallic.Destroy();
        Roughness.Destroy();
        Height.Destroy();
        Ao.Destroy();
        
        shader.Destroy();
    }

    void Debug()
    {
        std::cout << "Shader ID: " << shader.ID << "\n";
    }
};

struct SceneTreeComponent {
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

struct PointLightComponent
{
    float intensity;
    glm::vec3 color;
    PointLightComponent(float intensinty, glm::vec3 color) { intensity = intensinty; color = color; }
};