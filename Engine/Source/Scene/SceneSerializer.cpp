#include "rvelapch.h"
#include "SceneSerializer.h"
#include "Entity.h"
#include "Audio/AudioManager.h"

using namespace rv;

void SceneSerializer::SerializeHierarchyRecursively(Scene& scene, entt::entity current, json& outEntitiesArray, const std::unordered_set<entt::entity>& prefabChildren)
{
    if (current == entt::null || !scene.GetRegistry().valid(current)) return;

    if (current != scene.GetRootEntity())
    {
        if (prefabChildren.count(current) == 0)
        {
            outEntitiesArray.push_back(SceneSerializer::SerializeEntity(scene, current));
        }
    }

    if (scene.HasComponent<SceneTreeComponent>(current))
    {
        const auto& children = scene.GetComponent<SceneTreeComponent>(current).children;
        for (entt::entity child : children)
        {
            SerializeHierarchyRecursively(scene, child, outEntitiesArray, prefabChildren);
        }
    }
}

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

    entt::entity root = scene.GetRootEntity();
    SerializeHierarchyRecursively(scene, root, sceneJson["Entities"], prefabChildren);

    auto allEntitiesView = scene.GetRegistry().view<UUIDComponent, SceneTreeComponent>();
    for (auto e : allEntitiesView)
    {
        if (e == root || prefabChildren.count(e)) continue;

        EntityUUID uuid = scene.GetComponent<UUIDComponent>(e).uuid;
        bool alreadySaved = false;
        for (const auto& saved : sceneJson["Entities"])
        {
            if (saved.contains("UUID") && saved["UUID"] == uuid)
            {
                alreadySaved = true;
                break;
            }
        }

        if (!alreadySaved)
        {
            sceneJson["Entities"].push_back(SerializeEntity(scene, e));
        }
    }

    std::ofstream ofs(path);
    ofs << sceneJson.dump(4);
}

void SceneSerializer::LoadScene(Scene& scene, const std::string& path)
{
    scene.SetPath(path);

    json j;
    std::ifstream stream(path);
    if (!stream.is_open()) {
        LOG_ERROR("Scene file could not be opened: {}", path);
        return;
    }

    try {
        stream >> j;
    }
    catch (const std::exception& e) {
        LOG_ERROR("JSON parse error in {}: {}", path, e.what());
        return;
    }

    if (j.contains("Environment"))
        scene.GetEnvironment().Deserialize(j["Environment"]);

    if (!j.contains("Entities") || !j["Entities"].is_array())
        return;

    std::unordered_map<EntityUUID, entt::entity> uuidToEntity;
    std::vector<entt::entity> loadedEntities;
    std::unordered_set<entt::entity> prefabInstances;

    entt::entity rootHandle = scene.GetRootEntity();
    EntityUUID rootUUID = scene.GetComponent<UUIDComponent>(rootHandle).uuid;
    uuidToEntity[rootUUID] = rootHandle;

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
                scene.GetUUIDEntityMap()[savedUUID] = instance.GetHandle();
            }

            scene.GetComponent<SceneTreeComponent>(instance).parentUUID =
                entityJson.contains("ParentUUID") ? entityJson["ParentUUID"].get<EntityUUID>() : 0;

            prefabInstances.insert(instance.GetHandle());
            loadedEntities.push_back(instance.GetHandle());
            continue;
        }

        Entity e = scene.CreateEntityRaw();
        entt::entity handle = e.GetHandle();

        if (entityJson.contains("UUID"))
        {
            auto& uuidComp = scene.GetComponent<UUIDComponent>(handle);
            uuidComp.Deserialize(entityJson["UUID"]);
            uuidToEntity[uuidComp.uuid] = handle;
            scene.GetUUIDEntityMap()[uuidComp.uuid] = handle;
        }

        DeserializeEntityComponents(scene, handle, entityJson);

        loadedEntities.push_back(handle);
    }

    for (entt::entity entity : loadedEntities)
    {
        if (entity == rootHandle) continue;

        auto& tree = scene.GetComponent<SceneTreeComponent>(entity);
        if (tree.parentUUID != 0 && uuidToEntity.find(tree.parentUUID) != uuidToEntity.end())
        {
            scene.SetParentKeepLocal(entity, uuidToEntity[tree.parentUUID]);
        }
        else
        {
            scene.SetParentKeepLocal(entity, rootHandle);
        }
    }
}

json SceneSerializer::SerializeEntity(Scene& scene, entt::entity e)
{
    json j;

    if(scene.HasComponent<PrefabComponent>(e))
    {
        j["Prefab"] = scene.GetComponent<PrefabComponent>(e).Serialize();
        j["Transform"] = scene.GetComponent<TransformComponent>(e).Serialize();
        j["UUID"] = scene.GetComponent<UUIDComponent>(e).Serialize();

        if (scene.HasComponent<SceneTreeComponent>(e))
        {
            entt::entity parent = scene.GetComponent<SceneTreeComponent>(e).parent;
            j["ParentUUID"] = (parent != entt::null && parent != scene.GetRootEntity())
                ? scene.GetComponent<UUIDComponent>(parent).uuid
                : 0;
        }

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

    if (scene.HasComponent<AnimatorComponent>(e))
        j["AnimatorComponent"] = scene.GetComponent<AnimatorComponent>(e).Serialize();

    if (scene.HasComponent<SkeletalMeshComponent>(e))
        j["SkeletalMeshComponent"] = scene.GetComponent<SkeletalMeshComponent>(e).Serialize();

    if (scene.HasComponent<SkeletonComponent>(e))
        j["SkeletonComponent"] = scene.GetComponent<SkeletonComponent>(e).Serialize();

    if (scene.HasComponent<UICanvasComponent>(e))
        j["UICanvasComponent"] = scene.GetComponent<UICanvasComponent>(e).Serialize();

    if (scene.HasComponent<RectTransformComponent>(e))
        j["RectTransformComponent"] = scene.GetComponent<RectTransformComponent>(e).Serialize();

    if (scene.HasComponent<UIImageComponent>(e))
        j["UIImageComponent"] = scene.GetComponent<UIImageComponent>(e).Serialize();

    if (scene.HasComponent<UITextComponent>(e))
        j["UITextComponent"] = scene.GetComponent<UITextComponent>(e).Serialize();

    if (scene.HasComponent<UIButtonComponent>(e))
        j["UIButtonComponent"] = scene.GetComponent<UIButtonComponent>(e).Serialize();

    if (scene.HasComponent<UISliderComponent>(e))
        j["UISliderComponent"] = scene.GetComponent<UISliderComponent>(e).Serialize();

    if (scene.HasComponent<UIProgressBarComponent>(e))
        j["UIProgressBarComponent"] = scene.GetComponent<UIProgressBarComponent>(e).Serialize();

    if (scene.HasComponent<UICheckboxComponent>(e))
        j["UICheckboxComponent"] = scene.GetComponent<UICheckboxComponent>(e).Serialize();

    return j;
}

void SceneSerializer::DeserializeEntity(
    Scene& scene,
    const json& entityJson,
    std::unordered_map<EntityUUID, entt::entity>& uuidToEntity)
{
}

void SceneSerializer::DeserializeEntityComponents(Scene& scene, entt::entity handle, const json& entityJson)
{
    if (entityJson.contains("Tag"))
        scene.AddComponent<TagComponent>(handle).Deserialize(entityJson["Tag"]);

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
    {
        auto& rb = scene.AddComponent<RigidbodyComponent>(handle);
        rb.Deserialize(entityJson["RigidbodyComponent"]);
        rb.RuntimeBodyID = JPH::BodyID();
        rb.SetShapeDirty();
    }

    if (entityJson.contains("CharacterBodyComponent"))
    {
        auto& cb = scene.AddComponent<CharacterBodyComponent>(handle);
        cb.Deserialize(entityJson["CharacterBodyComponent"]);
        cb.character = nullptr;
        cb.SetShapeDirty();
    }

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
    {
        auto& audio = scene.AddComponent<AudioEmitterComponent>(handle);
        audio.Deserialize(entityJson["AudioEmitterComponent"]);
        audio.instanceID = UINT32_MAX;
        audio.prevPosValid = false;
    }

    if (entityJson.contains("ParticleEmitterComponent"))
        scene.AddComponent<ParticleEmitterComponent>(handle).Deserialize(entityJson["ParticleEmitterComponent"]);

    if (entityJson.contains("AnimatorComponent"))
        scene.AddComponent<AnimatorComponent>(handle).Deserialize(entityJson["AnimatorComponent"]);

    if (entityJson.contains("SkeletalMeshComponent"))
    {
        auto& comp = scene.AddComponent<SkeletalMeshComponent>(handle);
        comp.Deserialize(entityJson["SkeletalMeshComponent"]);
        scene.AddComponent<SkeletalMeshRendererComponent>(handle, comp.GetMesh());
    }

    if (entityJson.contains("SkeletonComponent"))
        scene.AddComponent<SkeletonComponent>(handle).Deserialize(entityJson["SkeletonComponent"]);

    if (entityJson.contains("UICanvasComponent"))
        scene.AddComponent<UICanvasComponent>(handle).Deserialize(entityJson["UICanvasComponent"]);

    if (entityJson.contains("RectTransformComponent"))
        scene.AddComponent<RectTransformComponent>(handle).Deserialize(entityJson["RectTransformComponent"]);

    if (entityJson.contains("UIImageComponent"))
        scene.AddComponent<UIImageComponent>(handle).Deserialize(entityJson["UIImageComponent"]);

    if (entityJson.contains("UITextComponent"))
        scene.AddComponent<UITextComponent>(handle).Deserialize(entityJson["UITextComponent"]);

    if (entityJson.contains("UIButtonComponent"))
        scene.AddComponent<UIButtonComponent>(handle).Deserialize(entityJson["UIButtonComponent"]);

    if (entityJson.contains("UISliderComponent"))
        scene.AddComponent<UISliderComponent>(handle).Deserialize(entityJson["UISliderComponent"]);

    if (entityJson.contains("UIProgressBarComponent"))
        scene.AddComponent<UIProgressBarComponent>(handle).Deserialize(entityJson["UIProgressBarComponent"]);

    if (entityJson.contains("UICheckboxComponent"))
        scene.AddComponent<UICheckboxComponent>(handle).Deserialize(entityJson["UICheckboxComponent"]);

    if (entityJson.contains("Prefab"))
    {
        AssetUUID pUUID = AssetUUID::FromString(entityJson["Prefab"]);
        if (pUUID.IsValid())
        {
            scene.AddComponent<PrefabComponent>(handle, pUUID);
        }
    }
}

std::string SceneSerializer::GenerateUniqueName(Scene& scene, const std::string& originalName)
{
    std::string baseName = originalName;

    size_t lastSpace = baseName.find_last_of(' ');
    if (lastSpace != std::string::npos && lastSpace + 1 < baseName.size())
    {
        bool onlyDigits = true;
        for (size_t i = lastSpace + 1; i < baseName.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(baseName[i])))
            {
                onlyDigits = false;
                break;
            }
        }
        if (onlyDigits)
        {
            baseName = baseName.substr(0, lastSpace);
        }
    }

    std::unordered_set<std::string> existingNames;
    for (auto entity : scene.GetRegistry().view<TagComponent>())
    {
        existingNames.insert(scene.GetRegistry().get<TagComponent>(entity).tag);
    }

    int counter = 1;
    std::string candidateName = baseName + " " + std::to_string(counter);
    while (existingNames.count(candidateName) > 0)
    {
        counter++;
        candidateName = baseName + " " + std::to_string(counter);
    }

    return candidateName;
}

Entity SceneSerializer::CloneEntity(Scene& scene, entt::entity sourceHandle)
{
    if (sourceHandle == entt::null || !scene.GetRegistry().valid(sourceHandle))
        return Entity{};

    if (sourceHandle == scene.GetRootEntity())
        return Entity{};

    auto CloneRecursive = [&scene](auto& self, entt::entity srcHandle, entt::entity parentHandle, bool isRoot) -> entt::entity
    {
        if (srcHandle == entt::null || !scene.GetRegistry().valid(srcHandle))
            return entt::null;

        json entityJson = SerializeEntity(scene, srcHandle);

        Entity newEntity = scene.CreateEntityRaw();
        entt::entity newHandle = newEntity.GetHandle();

        EntityUUID newUUID = EntityUUIDGenerator::GeneratePersistent();
        newEntity.GetComponent<UUIDComponent>().uuid = newUUID;
        scene.GetUUIDEntityMap()[newUUID] = newHandle;

        DeserializeEntityComponents(scene, newHandle, entityJson);

        if (isRoot)
        {
            std::string srcName = scene.HasComponent<TagComponent>(newHandle)
                ? scene.GetComponent<TagComponent>(newHandle).tag
                : "Entity";
            scene.GetComponent<TagComponent>(newHandle).tag = GenerateUniqueName(scene, srcName);
        }

        scene.SetParentKeepLocal(newHandle, parentHandle);

        if (scene.HasComponent<SceneTreeComponent>(srcHandle))
        {
            auto childrenCopy = scene.GetComponent<SceneTreeComponent>(srcHandle).children;
            for (entt::entity child : childrenCopy)
            {
                self(self, child, newHandle, false);
            }
        }

        return newHandle;
    };

    entt::entity parentHandle = scene.GetRootEntity();
    if (scene.HasComponent<SceneTreeComponent>(sourceHandle))
    {
        entt::entity p = scene.GetComponent<SceneTreeComponent>(sourceHandle).parent;
        if (p != entt::null && scene.GetRegistry().valid(p))
            parentHandle = p;
    }

    entt::entity cloned = CloneRecursive(CloneRecursive, sourceHandle, parentHandle, true);
    return Entity(cloned, &scene);
}

void SceneSerializer::CollectChildrenRecursively(Scene& scene, entt::entity e, std::unordered_set<entt::entity>& out)
{
    out.insert(e);
    if (scene.HasComponent<SceneTreeComponent>(e))
        for (auto child : scene.GetComponent<SceneTreeComponent>(e).children)
            CollectChildrenRecursively(scene, child, out);
}