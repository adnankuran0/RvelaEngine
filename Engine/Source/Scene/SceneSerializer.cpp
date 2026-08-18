#include "rvelapch.h"
#include "SceneSerializer.h"
#include "Entity.h"
#include "Audio/AudioManager.h"

using namespace rv;

void SceneSerializer::SaveScene(Scene& scene, const std::string& path)
{
    scene.SetPath(path);

    json sceneJson;
    sceneJson["Environment"] = scene.GetEnvironment().Serialize();
    sceneJson["Entities"] = json::array();

    std::unordered_set<entt::entity> prefabChildren;
    auto prefabView = scene.GetRegistry().view<PrefabComponent>();
    for (auto e : prefabView)
    {
        if (scene.HasComponent<SceneTreeComponent>(e))
        {
            for (auto child : scene.GetComponent<SceneTreeComponent>(e).children)
                CollectChildrenRecursively(scene, child, prefabChildren);
        }
    }

    auto view = scene.GetRegistry().view<UUIDComponent, SceneTreeComponent>();
    entt::entity root = scene.GetRootEntity();

    for (auto e : view)
    {
        if (e == root) continue;
        if (prefabChildren.count(e)) continue;
        sceneJson["Entities"].push_back(SerializeEntity(scene, e));
    }

    std::ofstream ofs(path);
    ofs << sceneJson.dump(4);
}

void SceneSerializer::LoadScene(Scene& scene, const std::string& path)
{
    scene.SetPath(path);

    json j;
    std::ifstream(path) >> j;

    if (j.contains("Environment"))
        scene.GetEnvironment().Deserialize(j["Environment"]);

    std::unordered_map<EntityUUID, entt::entity> uuidToEntity;
    std::unordered_set<entt::entity> prefabInstances;

    for (auto& entityJson : j["Entities"])
    {
        if (entityJson.contains("Prefab"))
        {
            AssetUUID prefabUUID = AssetUUID::FromString(entityJson["Prefab"]);
            Entity instance = scene.Instantiate(prefabUUID);

            if (entityJson.contains("Transform"))
                scene.GetComponent<TransformComponent>(instance).Deserialize(entityJson["Transform"]);

            if (entityJson.contains("UUID"))
            {
                EntityUUID savedUUID = entityJson["UUID"];
                uuidToEntity[savedUUID] = instance.GetHandle();
            }

            prefabInstances.insert(instance.GetHandle());
            continue;
        }

        DeserializeEntity(scene, entityJson, uuidToEntity);
    }

    for (auto& [uuid, entity] : uuidToEntity)
    {
        if (prefabInstances.count(entity)) continue;

        auto& tree = scene.GetComponent<SceneTreeComponent>(entity);
        if (tree.parentUUID != 0 && uuidToEntity.contains(tree.parentUUID))
            scene.SetParentKeepLocal(entity, uuidToEntity[tree.parentUUID]);
        else
            scene.SetParentKeepLocal(entity, scene.GetRootEntity());
    }
}

json SceneSerializer::SerializeEntity(Scene& scene, entt::entity e)
{
    json j;

    if (scene.HasComponent<PrefabComponent>(e))
    {
        j["Prefab"] = scene.GetComponent<PrefabComponent>(e).Serialize();
        j["Transform"] = scene.GetComponent<TransformComponent>(e).Serialize();
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

    if (scene.HasComponent<RigidbodyComponent>(e))
        j["RigidbodyComponent"] = scene.GetComponent<RigidbodyComponent>(e).Serialize();

    if (scene.HasComponent<CharacterBodyComponent>(e))
        j["CharacterBodyComponent"] = scene.GetComponent<CharacterBodyComponent>(e).Serialize();

    if (scene.HasComponent<BoxColliderComponent>(e))
        j["BoxColliderComponent"] = scene.GetComponent<BoxColliderComponent>(e).Serialize();

    if (scene.HasComponent<SphereColliderComponent>(e))
        j["SphereColliderComponent"] = scene.GetComponent<SphereColliderComponent>(e).Serialize();

    if (scene.HasComponent<CapsuleColliderComponent>(e))
        j["CapsuleColliderComponent"] = scene.GetComponent<CapsuleColliderComponent>(e).Serialize();

    if (scene.HasComponent<CylinderColliderComponent>(e))
        j["CylinderColliderComponent"] = scene.GetComponent<CylinderColliderComponent>(e).Serialize();

    if (scene.HasComponent<MeshColliderComponent>(e))
        j["MeshColliderComponent"] = scene.GetComponent<MeshColliderComponent>(e).Serialize();

    if (scene.HasComponent<ConvexHullColliderComponent>(e))
        j["ConvexHullColliderComponent"] = scene.GetComponent<ConvexHullColliderComponent>(e).Serialize();

    if (scene.HasComponent<SceneTreeComponent>(e))
    {
        auto& tree = scene.GetComponent<SceneTreeComponent>(e);
        entt::entity parent = tree.parent;

        j["ParentUUID"] = (parent != entt::null && parent != scene.GetRootEntity())
            ? scene.GetComponent<UUIDComponent>(parent).uuid
            : 0;
    }

    if (scene.HasComponent<AudioEmitterComponent>(e))
        j["AudioEmitterComponent"] = scene.GetComponent<AudioEmitterComponent>(e).Serialize();

    if (scene.HasComponent<ParticleEmitterComponent>(e))
        j["ParticleEmitterComponent"] = scene.GetComponent<ParticleEmitterComponent>(e).Serialize();

    return j;
}

void SceneSerializer::DeserializeEntity(
    Scene& scene,
    const json& entityJson,
    std::unordered_map<EntityUUID, entt::entity>& uuidToEntity)
{
    Entity e = scene.CreateEntity("");
    entt::entity handle = e.GetHandle();

    if (entityJson.contains("UUID"))
    {
        auto& uuidComp = scene.GetComponent<UUIDComponent>(handle);
        uuidComp.Deserialize(entityJson["UUID"]);
        uuidToEntity[uuidComp.uuid] = handle;
    }

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
        scene.AddComponent<CameraComponent>(handle).Deserialize(entityJson["CameraComponent"]);

    if (entityJson.contains("ScriptComponent"))
        scene.AddComponent<ScriptComponent>(handle).Deserialize(entityJson["ScriptComponent"]);

    if (entityJson.contains("RigidbodyComponent"))
        scene.AddComponent<RigidbodyComponent>(handle).Deserialize(entityJson["RigidbodyComponent"]);

    if (entityJson.contains("CharacterBodyComponent"))
        scene.AddComponent<CharacterBodyComponent>(handle).Deserialize(entityJson["CharacterBodyComponent"]);

    if (entityJson.contains("BoxColliderComponent"))
        scene.AddComponent<BoxColliderComponent>(handle).Deserialize(entityJson["BoxColliderComponent"]);

    if (entityJson.contains("SphereColliderComponent"))
        scene.AddComponent<SphereColliderComponent>(handle).Deserialize(entityJson["SphereColliderComponent"]);

    if (entityJson.contains("CapsuleColliderComponent"))
        scene.AddComponent<CapsuleColliderComponent>(handle).Deserialize(entityJson["CapsuleColliderComponent"]);

    if (entityJson.contains("CylinderColliderComponent"))
        scene.AddComponent<CylinderColliderComponent>(handle).Deserialize(entityJson["CylinderColliderComponent"]);

    if (entityJson.contains("MeshColliderComponent"))
        scene.AddComponent<MeshColliderComponent>(handle).Deserialize(entityJson["MeshColliderComponent"]);

    if (entityJson.contains("ConvexHullColliderComponent"))
        scene.AddComponent<ConvexHullColliderComponent>(handle).Deserialize(entityJson["ConvexHullColliderComponent"]);

    if (entityJson.contains("ParentUUID"))
        scene.GetComponent<SceneTreeComponent>(handle).parentUUID = entityJson["ParentUUID"];

    if (entityJson.contains("AudioEmitterComponent"))
        scene.AddComponent<AudioEmitterComponent>(handle).Deserialize(entityJson["AudioEmitterComponent"]);

    if (entityJson.contains("ParticleEmitterComponent"))
        scene.AddComponent<ParticleEmitterComponent>(handle).Deserialize(entityJson["ParticleEmitterComponent"]);
}

void SceneSerializer::CollectChildrenRecursively(Scene& scene, entt::entity e, std::unordered_set<entt::entity>& out)
{
    out.insert(e);
    if (scene.HasComponent<SceneTreeComponent>(e))
        for (auto child : scene.GetComponent<SceneTreeComponent>(e).children)
            CollectChildrenRecursively(scene, child, out);
}
