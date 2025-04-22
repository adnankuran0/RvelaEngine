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
#include "Component.h"
#include "../nlohmann/json.hpp"
#include "Core/Utils/MaterialManager.h"

using json = nlohmann::json;
using UUID = uint64_t;

class WorldTransformComponent : public Component {
public:
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

    std::string Serialize() const override
    {
        json j;
        j["position"] = { position.x,position.y,position.z };
        j["rotation"] = { rotation.x,rotation.y,rotation.z };
        j["scale"] = { scale.x,scale.y,scale.z };

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto positionData = j["position"];
        position = glm::vec3(positionData[0], positionData[1], positionData[2]);
        auto rotationData = j["rotation"];
        rotation = glm::vec3(rotationData[0], rotationData[1], rotationData[2]);
        auto scaleData = j["scale"];
        scale = glm::vec3(scaleData[0], scaleData[1], scaleData[2]);
        
    }
};

class TransformComponent : public Component {
public:
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

    std::string Serialize() const override
    {
        json j;
        j["position"] = { position.x,position.y,position.z };
        j["rotation"] = { rotation.x,rotation.y,rotation.z };
        j["scale"] = { scale.x,scale.y,scale.z };

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto positionData = j["position"];
        position = glm::vec3(positionData[0], positionData[1], positionData[2]);
        auto rotationData = j["rotation"];
        rotation = glm::vec3(rotationData[0], rotationData[1], rotationData[2]);
        auto scaleData = j["scale"];
        scale = glm::vec3(scaleData[0], scaleData[1], scaleData[2]);

    }
};


class TagComponent : public Component {
public:
    std::string tag;
    TagComponent() = default;
    TagComponent(const std::string& tag) : tag(tag) {}

    std::string Serialize() const override
    {
        json j;
        j["tag"] = tag;

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto tagData = j["tag"];
        tag = tagData;
    }
    
};

class MeshComponent{
public:
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



class MaterialComponent : public Component {
public:

    MaterialComponent() = delete;

    MaterialComponent(const std::string& materialPath)
        : materialPath(materialPath)
    {
        material = MaterialManager::LoadOrGetMaterial(materialPath);
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

    std::string& GetMaterialPath()
    {
        return materialPath;
    }

    void SetMaterialPath(const std::string& materialPath)
    {
        if (this->materialPath == materialPath) return;
        MaterialManager::UnloadMaterial(materialPath);
        MaterialManager::ClearExpiredMaterials();
        this->materialPath = materialPath;
        material = MaterialManager::LoadOrGetMaterial(materialPath);
    }

    

    std::string Serialize() const override
    {
        json j;
        j["materialPath"] = materialPath;

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto materialPathData = j["materialPath"];
        materialPath = materialPathData;

    }

    std::shared_ptr<Material> material;
    

private:
    std::string materialPath;
};

class SceneTreeComponent : public Component {
public:
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;

    UUID parentUUID = 0;
    std::vector<UUID> childrenUUIDs;

    std::string Serialize() const override
    {
        json j;
        j["parentUUID"] = parentUUID;
        j["childrenUUIDs"] = childrenUUIDs;

        return j.dump(4);
    }

    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        parentUUID = j["parentUUID"];
        childrenUUIDs.clear();
        for (auto& id : j["childrenUUIDs"])
        {
            childrenUUIDs.push_back(id.get<UUID>());
        }
    }

};

class UUIDComponent : public Component
{
public:
    UUID uuid;

    UUIDComponent() = default;
    UUIDComponent(UUID uuid):uuid(uuid){}

    std::string Serialize() const override
    {
        json j;
        j["UUID"] = uuid;

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto UUIDData = j["UUID"];
        uuid = UUIDData;

    }

};
