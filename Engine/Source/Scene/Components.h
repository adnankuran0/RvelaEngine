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
    glm::quat rotation;
    glm::vec3 scale;
    glm::vec3 rotation_degrees = glm::vec3(0.0f);

    WorldTransformComponent() = default;
    WorldTransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
        : position(pos), rotation(rot), scale(scl) {
    }

    glm::mat4 GetMatrix() const {
        return glm::translate(glm::mat4(1.0f), position) *
            glm::mat4_cast(rotation) *
            glm::scale(glm::mat4(1.0f), scale);
    }

    void SetEulerRotation(const glm::vec3& euler) {
        rotation_degrees = euler;
        rotation = EulerToQuat(rotation_degrees);
    }
    glm::vec3 GetEulerRotation() const {
        return rotation_degrees;
    }

    glm::vec3 GetForward() const {
        return glm::normalize(glm::mat3_cast(rotation) * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 GetUp() const {
        return glm::normalize(glm::mat3_cast(rotation) * glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 GetRight() const {
        return glm::normalize(glm::mat3_cast(rotation) * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    std::string Serialize() const override
    {
        json j;
        j["position"] = { position.x,position.y,position.z };
        j["rotation"] = { rotation_degrees.x, rotation_degrees.y, rotation_degrees.z };
        j["scale"] = { scale.x,scale.y,scale.z };

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto positionData = j["position"];
        position = glm::vec3(positionData[0], positionData[1], positionData[2]);
        auto rotationData = j["rotation"];
        glm::vec3 euler(rotationData[0], rotationData[1], rotationData[2]);
        SetEulerRotation(euler);
        auto scaleData = j["scale"];
        scale = glm::vec3(scaleData[0], scaleData[1], scaleData[2]);

    }
};

class TransformComponent : public Component {
public:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    glm::vec3 rotation_degrees = glm::vec3(0.0f);


    TransformComponent() = default;
    TransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
        : position(pos), rotation(rot), scale(scl) {
    }

    void SetEulerRotation(const glm::vec3& euler) {
        rotation_degrees = euler;
        rotation = EulerToQuat(rotation_degrees);
    }
    glm::vec3 GetEulerRotation() const {
        return rotation_degrees;
    }

    void SetRotation(const glm::quat& quat)
    {
        rotation = quat;
        rotation_degrees = QuatToEuler(quat);
    }

    glm::mat4 GetMatrix() const {
        return glm::translate(glm::mat4(1.0f), position) *
            glm::mat4_cast(rotation) *
            glm::scale(glm::mat4(1.0f), scale);
    }

    glm::vec3 GetForward() const {
        return glm::normalize(glm::mat3_cast(rotation) * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 GetUp() const {
        return glm::normalize(glm::mat3_cast(rotation) * glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 GetRight() const {
        return glm::normalize(glm::mat3_cast(rotation) * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    std::string Serialize() const override
    {
        json j;
        j["position"] = { position.x,position.y,position.z };
        j["rotation"] = { rotation_degrees.x, rotation_degrees.y, rotation_degrees.z };
        j["scale"] = { scale.x,scale.y,scale.z };

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto positionData = j["position"];
        position = glm::vec3(positionData[0], positionData[1], positionData[2]);
        auto rotationData = j["rotation"];
        glm::vec3 euler(rotationData[0], rotationData[1], rotationData[2]);
        SetEulerRotation(euler);
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

class MeshRendererComponent {
public:
    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;
    BufferLayout Layout;
    unsigned int indexCount = 0;
    MeshRendererComponent() = default;
    MeshRendererComponent(const MeshRendererComponent&) = delete;
    MeshRendererComponent& operator=(const MeshRendererComponent&) = delete;
    MeshRendererComponent(MeshRendererComponent&&) = default;
    MeshRendererComponent& operator=(MeshRendererComponent&&) = default;
    MeshRendererComponent(void* vertices, size_t sizeOfVertices, void* indices, size_t sizeOfIndices, unsigned int indexCount)
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

struct MeshComponent : public ISerializable {
    std::string modelPath;
    uint32_t meshIndex;

    MeshComponent() = delete;
    MeshComponent(const std::string& modelPath, uint16_t meshIndex) : modelPath(modelPath), meshIndex(meshIndex) {}

    std::string Serialize() const override {
        json j;
        j["modelPath"] = modelPath;
        j["meshIndex"] = meshIndex;
        return j.dump(4);
    }

    void Deserialize(const std::string& str) override {
        json j = json::parse(str);
        modelPath = j["modelPath"];
        meshIndex = j["meshIndex"];
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
    UUIDComponent(UUID uuid) :uuid(uuid) {}

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

class PointLightComponent : public Component
{
public:
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 10.0f;
    float radius = 10.0f;
    PointLightComponent() = default;
    PointLightComponent(const glm::vec3& color, float intensity, float radius) : color(color), intensity(intensity), radius(radius) {}

    std::string Serialize() const override
    {
        json j;
        j["color"] = { color.r,color.g,color.b };
        j["intensity"] = intensity;
        j["radius"] = radius;
        return j.dump(4);
    }

    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto colorData = j["color"];
        color = glm::vec3(colorData[0], colorData[1], colorData[2]);
        intensity = j["intensity"];
        radius = j["radius"];
    }
};

class DirectionalLightComponent : public Component
{
public:
    DirectionalLightComponent() = default;
    DirectionalLightComponent(const glm::vec3& color, float intensity) : color(color), intensity(intensity) {}

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 10.0f;
    bool castShadows = true;
    float shadowBias = 0.005f;

    std::string Serialize() const override
    {
        json j;
        j["color"] = { color.r,color.g,color.b };
        j["intensity"] = intensity;
        return j.dump(4);
    }

    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto colorData = j["color"];
        color = glm::vec3(colorData[0], colorData[1], colorData[2]);
        intensity = j["intensity"];
    }
};

class SpotLightComponent : public Component
{
public:
    SpotLightComponent() = default;
    SpotLightComponent(const glm::vec3& color, float intensity, float radius, float innerCutoff, float outerCutoff)
        : color(color), intensity(intensity), radius(radius), innerCutoff(glm::cos(glm::radians(innerCutoff))), outerCutoff(glm::cos(glm::radians(outerCutoff))) {
    }

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float radius = 5.0f;
    float innerCutoff = glm::cos(glm::radians(15.0f));
    float outerCutoff = glm::cos(glm::radians(30.0f));

    std::string Serialize() const override
    {
        json j;
        j["color"] = { color.r,color.g,color.b };
        j["intensity"] = intensity;
        j["radius"] = radius;
        j["innerCutoff"] = innerCutoff;
        j["outerCutoff"] = outerCutoff;
        return j.dump(4);
    }

    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto colorData = j["color"];
        color = glm::vec3(colorData[0], colorData[1], colorData[2]);
        intensity = j["intensity"];
        radius = j["radius"];
        innerCutoff = j["innerCutoff"];
        outerCutoff = j["outerCutoff"];
    }

};