#include "rvelapch.h"
#include "SceneSerializer.h"
#include "Entity.h"

using namespace rv;


void SceneSerializer::SaveScene(Scene& scene, const std::string& path)
{
    json sceneJson;
    sceneJson["Environment"] = scene.GetEnvironment().Serialize();
    sceneJson["Entities"] = json::array();

    auto view = scene.GetRegistry().view<UUIDComponent, SceneTreeComponent>();
    entt::entity root = scene.GetRootEntity();

    for (auto e : view)
    {
        if (e == root) continue;

        sceneJson["Entities"].push_back(
            SerializeEntity(scene, e)
        );
    }

    std::ofstream ofs(path);
    ofs << sceneJson.dump(4);
}

void SceneSerializer::LoadScene(Scene& scene, const std::string& path)
{
    scene.SetPath(path);

    json j;
    std::ifstream(path) >> j;

    std::unordered_map<EntityUUID, entt::entity> uuidToEntity;
    if (j.contains("Environment"))
        scene.GetEnvironment().Deserialize(j["Environment"]);
    for (auto& entityJson : j["Entities"])
    {
        bool isPrefab = entityJson.contains("Prefab");

        if (isPrefab)
        {
            // Instantiate
            AssetUUID prefabUUID = AssetUUID::FromString(entityJson["Prefab"]);
            Entity instance = scene.Instantiate(prefabUUID);

            if (entityJson.contains("Transform"))
            {
                scene.GetComponent<TransformComponent>(instance).Deserialize(entityJson["Transform"]);
            }

            if (entityJson.contains("UUID"))
            {
                EntityUUID savedUUID = entityJson["UUID"];
                uuidToEntity[savedUUID] = instance.GetHandle();
            }

            continue;
        }

        Entity e = scene.CreateEntity("");
        entt::entity handle = e.GetHandle();

        auto& uuidComp = scene.GetComponent<UUIDComponent>(handle);
        uuidComp.Deserialize(entityJson["UUID"]);
        uuidToEntity[uuidComp.uuid] = handle;

        if (entityJson.contains("Tag"))
            scene.GetComponent<TagComponent>(handle).Deserialize(entityJson["Tag"]);

        if (entityJson.contains("Transform"))
            scene.GetComponent<TransformComponent>(handle).Deserialize(entityJson["Transform"]);

        if (entityJson.contains("Material"))
            scene.AddComponent<MaterialComponent>(handle).Deserialize(entityJson["Material"]);

        if (entityJson.contains("Mesh"))
        {
            auto& comp = scene.AddComponent<MeshComponent>(handle);
            comp.Deserialize(entityJson["Mesh"]);
            scene.AddComponent<MeshRendererComponent>(handle, comp.GetMesh());
        }

        if (entityJson.contains("PointLight"))
            scene.AddComponent<PointLightComponent>(handle).Deserialize(entityJson["PointLight"]);

        if (entityJson.contains("DirectionalLight"))
            scene.AddComponent<DirectionalLightComponent>(handle).Deserialize(entityJson["DirectionalLight"]);

        if (entityJson.contains("CameraComponent"))
        {
            scene.AddComponent<CameraComponent>(handle).Deserialize(entityJson["CameraComponent"]);
        }

        if (entityJson.contains("ScriptComponent"))
        {
            scene.AddComponent<ScriptComponent>(handle).Deserialize(entityJson["ScriptComponent"]);

        }

        if (entityJson.contains("ParentUUID"))
        {
            auto& tree = scene.GetComponent<SceneTreeComponent>(handle);
            tree.parentUUID = entityJson["ParentUUID"];
        }
    }

    for (auto& [uuid, entity] : uuidToEntity)
    {
        auto& tree = scene.GetComponent<SceneTreeComponent>(entity);
        if (tree.parentUUID != 0 && uuidToEntity.contains(tree.parentUUID))
        {
            scene.SetParentKeepLocal(entity, uuidToEntity[tree.parentUUID]);
        }
        else
        {
            scene.SetParentKeepLocal(entity, scene.GetRootEntity());
        }
    }

    scene.UpdateHierarchy();
}

json SceneSerializer::SerializeEntity(Scene& scene, entt::entity e)
{
    json j;

    bool isPrefab = scene.HasComponent<PrefabComponent>(e);
    if (isPrefab)
    {
        auto& comp = scene.GetComponent<PrefabComponent>(e);
        j["Prefab"] = comp.Serialize();

        // per instance transform
        if (scene.HasComponent<TransformComponent>(e))
            j["Transform"] = scene.GetComponent<TransformComponent>(e).Serialize();

        if (scene.HasComponent<UUIDComponent>(e))
            j["UUID"] = scene.GetComponent<UUIDComponent>(e).Serialize();

        return j;
    }

    if (scene.HasComponent<UUIDComponent>(e))
        j["UUID"] = scene.GetComponent<UUIDComponent>(e).Serialize();

    if (scene.HasComponent<TagComponent>(e))
        j["Tag"] = scene.GetComponent<TagComponent>(e).Serialize();

    if (scene.HasComponent<TransformComponent>(e))
        j["Transform"] = scene.GetComponent<TransformComponent>(e).Serialize();

    if (scene.HasComponent<MaterialComponent>(e))
        j["Material"] = scene.GetComponent<MaterialComponent>(e).Serialize();

    if (scene.HasComponent<MeshComponent>(e))
        j["Mesh"] = scene.GetComponent<MeshComponent>(e).Serialize();

    if (scene.HasComponent<PointLightComponent>(e))
        j["PointLight"] = scene.GetComponent<PointLightComponent>(e).Serialize();

    if (scene.HasComponent<DirectionalLightComponent>(e))
        j["DirectionalLight"] = scene.GetComponent<DirectionalLightComponent>(e).Serialize();

    if (scene.HasComponent<CameraComponent>(e))
        j["CameraComponent"] = scene.GetComponent<CameraComponent>(e).Serialize();

    if (scene.HasComponent<ScriptComponent>(e))
        j["ScriptComponent"] = scene.GetComponent<ScriptComponent>(e).Serialize();

    if (scene.HasComponent<SceneTreeComponent>(e))
    {
        auto& tree = scene.GetComponent<SceneTreeComponent>(e);
        entt::entity parent = tree.parent;

        if (parent != entt::null && parent != scene.GetRootEntity())
        {
            auto& parentUUID = scene.GetComponent<UUIDComponent>(parent);
            j["ParentUUID"] = parentUUID.uuid;
        }
        else
        {
            j["ParentUUID"] = 0;
        }
    }

    return j;
}
