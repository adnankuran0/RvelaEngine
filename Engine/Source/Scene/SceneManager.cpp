#include "rvelapch.h"
#include "SceneManager.h"
#include <fstream>
#include "../nlohmann/json.hpp"
#include "Core/Utils/AssetManager.h"
#include "UUIDGenerator.h"  


using json = nlohmann::json;

void SceneManager::SaveScene(Scene& scene, const std::string& path)
{
    json sceneJson;
    auto& registry = scene.GetRegistry();

    registry.view<UUIDComponent>().each([&](auto entity, UUIDComponent& uuidComp)
        {
            json entityJson;
            entityJson["UUID"] = uuidComp.uuid;

            if (scene.HasComponent<TagComponent>(entity))
                entityJson["TagComponent"] = scene.GetComponent<TagComponent>(entity).Serialize();

            if (scene.HasComponent<TransformComponent>(entity))
                entityJson["TransformComponent"] = scene.GetComponent<TransformComponent>(entity).Serialize();

            if (scene.HasComponent<WorldTransformComponent>(entity))
                entityJson["WorldTransformComponent"] = scene.GetComponent<WorldTransformComponent>(entity).Serialize();

            if (scene.HasComponent<MaterialComponent>(entity))
                entityJson["MaterialComponent"] = scene.GetComponent<MaterialComponent>(entity).Serialize();

            if (scene.HasComponent<SceneTreeComponent>(entity))
                entityJson["SceneTreeComponent"] = scene.GetComponent<SceneTreeComponent>(entity).Serialize();

            if (scene.HasComponent<MeshComponent>(entity))
            {
                entityJson["MeshComponent"] = scene.GetComponent<MeshComponent>(entity).Serialize();
            }

            if (scene.HasComponent<PointLightComponent>(entity))
            {
                entityJson["PointLightComponent"] = scene.GetComponent<PointLightComponent>(entity).Serialize();
            }

            if (scene.HasComponent<DirectionalLightComponent>(entity))
            {
                entityJson["DirectionalLightComponent"] = scene.GetComponent<DirectionalLightComponent>(entity).Serialize();
            }

            sceneJson["Entities"].push_back(entityJson);
        });

    std::ofstream file(path);
    file << sceneJson.dump(16);
    file.close();
}

void SceneManager::LoadScene(Scene& scene, const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return;
    auto start = std::chrono::high_resolution_clock::now();
    json sceneJson;
    file >> sceneJson;
    file.close();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Json loading took " << duration.count() << "ms\n";

    auto& registry = scene.GetRegistry();
    for (auto entity : registry.view<UUIDComponent>())
        scene.DestroyEntity(entity);
    

    std::unordered_map<UUID, entt::entity> uuidToEntity;

    start = std::chrono::high_resolution_clock::now();
    auto meshLoadTotal = std::chrono::milliseconds(0);

    for (auto& entityJson : sceneJson["Entities"])
    {
        UUID uuid = entityJson["UUID"];
        UUIDGenerator::RegisterExternalUUID(uuid);
        Entity e = scene.CreateEntityWithUUID("am", uuid);
        entt::entity entity = e.GetHandle();
        uuidToEntity[uuid] = entity;

        if (entityJson.contains("TagComponent"))
            scene.GetComponent<TagComponent>(entity).Deserialize(entityJson["TagComponent"]);

        if (entityJson.contains("TransformComponent"))
            scene.GetComponent<TransformComponent>(entity).Deserialize(entityJson["TransformComponent"]);

        if (entityJson.contains("WorldTransformComponent"))
            scene.GetComponent<WorldTransformComponent>(entity).Deserialize(entityJson["WorldTransformComponent"]);

        if (entityJson.contains("MaterialComponent"))
        {
            std::string serializedMat = entityJson["MaterialComponent"];
            json j = json::parse(serializedMat);
            std::string newMaterialPath = j["materialPath"];

            scene.AddComponent<MaterialComponent>(entity, newMaterialPath);
            auto& mat = scene.GetComponent<MaterialComponent>(entity);
            mat.Deserialize(serializedMat);
        }

        if (entityJson.contains("MeshComponent"))
        {
            std::string serializedMesh = entityJson["MeshComponent"];
            json j = json::parse(serializedMesh);
            std::string newModelPath = j["modelPath"];
            uint16_t meshIndex = j["meshIndex"];

            auto meshStart = std::chrono::high_resolution_clock::now();
            MeshData meshData = AssetManager::LoadMesh(newModelPath, meshIndex);
            auto meshEnd = std::chrono::high_resolution_clock::now();
            meshLoadTotal += std::chrono::duration_cast<std::chrono::milliseconds>(meshEnd - meshStart);

            scene.AddComponent<MeshComponent>(entity, newModelPath, meshIndex);
            scene.AddComponent<MeshRendererComponent>(entity, meshData.vertices.data(),
                meshData.vertices.size() * sizeof(float),
                meshData.indices.data(),
                meshData.indices.size() * sizeof(unsigned int),
                meshData.indices.size());
        }

        if (entityJson.contains("PointLightComponent"))
        {
            scene.AddComponent<PointLightComponent>(entity);
            auto& pointLightComponent = scene.GetComponent<PointLightComponent>(entity);
            pointLightComponent.Deserialize(entityJson["PointLightComponent"]);
        }

        if (entityJson.contains("DirectionalLightComponent"))
        {
            scene.AddComponent<DirectionalLightComponent>(entity);
            auto& directionalLightComponent = scene.GetComponent<DirectionalLightComponent>(entity);
            directionalLightComponent.Deserialize(entityJson["DirectionalLightComponent"]);
        }

        if (entityJson.contains("SceneTreeComponent"))
        {
            auto& sceneTreeComp = scene.GetComponent<SceneTreeComponent>(entity);
            std::unordered_set<UUID> uniqueChildren;
            for (UUID childUUID : sceneTreeComp.childrenUUIDs)
                uniqueChildren.insert(childUUID);
            sceneTreeComp.childrenUUIDs.assign(uniqueChildren.begin(), uniqueChildren.end());
            sceneTreeComp.Deserialize(entityJson["SceneTreeComponent"]);
        }
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Scene loading took " << duration.count() << "ms\n";
    std::cout << "Total mesh loading took " << meshLoadTotal.count() << "ms\n";

    for (auto& [uuid, entity] : uuidToEntity)
    {
        if (!scene.HasComponent<SceneTreeComponent>(entity)) continue;

        auto& tree = scene.GetComponent<SceneTreeComponent>(entity);

        if (tree.parentUUID != 0 && uuidToEntity.contains(tree.parentUUID))
            scene.SetParent(entity, uuidToEntity[tree.parentUUID]);

        tree.children.clear();
        for (UUID childUUID : tree.childrenUUIDs)
        {
            if (uuidToEntity.contains(childUUID))
                tree.children.push_back(uuidToEntity[childUUID]);
        }
    }

    scene.UpdateHierarchy();
    
}