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
#include "Core/RvelaMath.h"
#include "entt/entt.h"
#include "../Renderer/Material.h"
#include <memory>

struct WorldTransformComponent {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::mat4 fixRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));

    WorldTransformComponent() = default;
    WorldTransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
        : position(pos), rotation(rot), scale(scl) {
    }

    glm::quat GetQuaternion() const {
        return EulerToQuat(rotation);
    }

    glm::mat4 GetMatrix() const {
        return (glm::translate(glm::mat4(1.0f), position) *
            glm::mat4_cast(GetQuaternion()) *
            glm::scale(glm::mat4(1.0f), scale));
    }
};

struct TransformComponent {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::mat4 fixRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));

    TransformComponent() = default;
    TransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
        : position(pos), rotation(rot), scale(scl) {
    }

    glm::quat GetQuaternion() const {
        return EulerToQuat(rotation);
    }

    glm::mat4 GetMatrix() const {
        return (glm::translate(glm::mat4(1.0f), position) *
            glm::mat4_cast(GetQuaternion()) *
            glm::scale(glm::mat4(1.0f), scale));
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
    unsigned int indexCount = 0;
    MeshComponent() = default;
    MeshComponent(const MeshComponent&) = delete;
    MeshComponent& operator=(const MeshComponent&) = delete;
    MeshComponent(MeshComponent&&) = default;
    MeshComponent& operator=(MeshComponent&&) = default;
    MeshComponent(void* vertices,size_t sizeOfVertices,void* indices,size_t sizeOfIndices,unsigned int indexCount) 
    { 
        this->indexCount = indexCount;

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

    MaterialComponent(const std::string& materialPath)
        : materialPath(materialPath)
    {
    }

    MaterialComponent(const MaterialComponent& other)
        : materialPath(other.materialPath)
    {
    }

    MaterialComponent(MaterialComponent&& other) noexcept
        : materialPath(std::move(other.materialPath))
    {
    }

    ~MaterialComponent()
    {
    }

    std::string materialPath;
    std::shared_ptr<Material> material;
};

struct SceneTreeComponent {
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

