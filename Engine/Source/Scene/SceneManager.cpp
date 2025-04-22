#include "rvelapch.h"
#include "SceneManager.h"
#include <fstream>
#include "../nlohmann/json.hpp"
#include "Core/Utils/AssetManager.h"
#include "UUIDGenerator.h"  

//TODO: UUID olayı sıkıntı sahneyi yükledikten sonra silerken hata veriyor
//TODO: MeshComponent sadece yol gibi bir şey tutmalı şuanki ise MeshRendererComponenet gibi bir şey olmalı

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

    json sceneJson;
    file >> sceneJson;
    file.close();

    auto& registry = scene.GetRegistry();
    for (auto entity : registry.view<UUIDComponent>())
        scene.DestroyEntity(entity);
    

    std::unordered_map<UUID, entt::entity> uuidToEntity;

    for (auto& entityJson : sceneJson["Entities"])
    {
        UUID uuid = entityJson["UUID"];
        UUIDGenerator::RegisterExternalUUID(uuid);
        Entity e = scene.CreateEntityWithUUID("am", uuid);
       
        entt::entity entity = e.GetHandle();

        uuidToEntity[uuid] = entity;

        if (entityJson.contains("TagComponent"))
        {
            scene.GetComponent<TagComponent>(entity).Deserialize(entityJson["TagComponent"]);
        }

        if (entityJson.contains("TransformComponent"))
        {
            scene.GetComponent<TransformComponent>(entity).Deserialize(entityJson["TransformComponent"]);
        }

        if (entityJson.contains("WorldTransformComponent"))
        {
            scene.GetComponent<WorldTransformComponent>(entity).Deserialize(entityJson["WorldTransformComponent"]);
        }

        if (entityJson.contains("MaterialComponent"))
        {
            std::string serializedMat = entityJson["MaterialComponent"];
            json j = json::parse(serializedMat);
            std::string newMaterialPath = j["materialPath"];

            scene.AddComponent<MaterialComponent>(entity, newMaterialPath);
            auto& mat = scene.GetComponent<MaterialComponent>(entity);
            mat.Deserialize(serializedMat); 
        }
        
        std::vector<MeshData> meshDatas = AssetManager::LoadModel("D:/GitHub/RvelaEngine/Resources/Engine/Models/cube.fbx");
        scene.AddComponent<MeshComponent>(entity,meshDatas.back().vertices.data(), meshDatas.back().vertices.size() * sizeof(float),
                meshDatas.back().indices.data(), meshDatas.back().indices.size() * sizeof(unsigned int), meshDatas.back().indices.size());

        if (entityJson.contains("SceneTreeComponent"))
        {
            scene.GetComponent<SceneTreeComponent>(entity).Deserialize(entityJson["SceneTreeComponent"]);
        }
        
    }

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