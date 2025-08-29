
#include "PrefabImporter.h"
#include "json.hpp"

Ref<PrefabAsset> PrefabImporter::CreatePrefabAsset(const std::string& path, Scene& scene, entt::entity& rootEntity)
{
    std::unique_ptr<PrefabMeta> meta = std::make_unique<PrefabMeta>();
    meta->entityCount = CountEntitiesRecursively(scene, rootEntity);

    Ref<PrefabAsset> asset = CreateRef<PrefabAsset>(path, std::move(meta));

    auto entites = scene.GetRegistry().view<UUIDComponent>();

    std::vector<std::byte> buffer;
    json j;

    SerializeEntityRecursively(rootEntity, scene, buffer,j);

    asset->SetData(std::move(buffer));

    asset->Serialize();
    //std::ofstream file(path + ".json");
    //file << j.dump(4);
    //file.close();

    return asset;
}

void PrefabImporter::SerializeEntityRecursively(entt::entity& e, Scene& scene, std::vector<std::byte>& buffer, json& j)
{
    // serialize component count
    unsigned int componentCount = GetComponentCount(scene, e);
    const std::byte* data = reinterpret_cast<const std::byte*>(&componentCount);
    buffer.insert(buffer.end(), data, data + sizeof(unsigned int));

    // serialize all components
    auto& uuidComp = scene.GetComponent<UUIDComponent>(e);
    SerializeBin_UUIDComp(uuidComp, buffer);
    //SerializeJson_UUIDComp(uuidComp, j);
    //j[uuidComp.uuid] = "";

    if (scene.HasComponent<TagComponent>(e))
    {
        auto& comp = scene.GetComponent<TagComponent>(e);
        SerializeBin_TagComp(comp, buffer);
        //SerializeJson_TagComp(comp, j[uuidComp.uuid]);
    }

    if (scene.HasComponent<TransformComponent>(e))
    {
        auto& comp = scene.GetComponent<TransformComponent>(e);
        SerializeBin_TransformComp(comp, buffer);
        //SerializeJson_TransformComp(comp, j[uuidComp.uuid]);
    }

    if (scene.HasComponent<MaterialComponent>(e))
    {
        auto& comp = scene.GetComponent<MaterialComponent>(e);
        SerializeBin_MaterialComp(comp, buffer);
        //SerializeJson_MaterialComp(comp, j[uuidComp.uuid]);
    }

    if (scene.HasComponent<SceneTreeComponent>(e))
    {
        auto& comp = scene.GetComponent<SceneTreeComponent>(e);
        SerializeBin_SceneTreeComp(comp, buffer);
        //SerializeJson_SceneTreeComp(comp, j[uuidComp.uuid]);
    }

    if (scene.HasComponent<MeshComponent>(e))
    {
        auto& comp = scene.GetComponent<MeshComponent>(e);
        SerializeBin_MeshComp(comp, buffer);
        //SerializeJson_MeshComp(comp, j[uuidComp.uuid]);
    }

    if (scene.HasComponent<MeshRendererComponent>(e))
    {
        auto& comp = scene.GetComponent<MeshRendererComponent>(e);
        
        SerializeBin_MeshRendererComp(comp, buffer);
        //SerializeJson_MeshComp(comp, j[uuidComp.uuid]);
    }

    if (scene.HasComponent<PointLightComponent>(e))
    {
        auto& comp = scene.GetComponent<PointLightComponent>(e);
        SerializeBin_PointLightComp(comp, buffer);
        //SerializeJson_PointLightComp(comp, j[uuidComp.uuid]);
    }

    if (scene.HasComponent<DirectionalLightComponent>(e))
    {
        auto& comp = scene.GetComponent<DirectionalLightComponent>(e);
        SerializeBin_DirectionalLightComp(comp, buffer);
        //SerializeJson_DirectionalLightComp(comp, j[uuidComp.uuid]);
    }

    if (scene.HasComponent<SceneTreeComponent>(e))
    {
        auto& children = scene.GetComponent<SceneTreeComponent>(e).children;
        if (children.empty()) return;

        for (auto& child : children)
        {
            SerializeEntityRecursively(child, scene, buffer,j);
        }

    }

}

unsigned int PrefabImporter::GetComponentCount(Scene& scene, entt::entity& entity)
{
    auto& registry = scene.GetRegistry();
    unsigned int count = 0;

    if (registry.any_of<TransformComponent>(entity)) count++;
    if (registry.any_of<MeshComponent>(entity)) count++;
    if (registry.any_of<DirectionalLightComponent>(entity)) count++;
    if (registry.any_of<MaterialComponent>(entity)) count++;
    if (registry.any_of<MeshRendererComponent>(entity)) count++;
    if (registry.any_of<PointLightComponent>(entity)) count++;
    if (registry.any_of<SceneTreeComponent>(entity)) count++;
    if (registry.any_of<SpotLightComponent>(entity)) count++;
    if (registry.any_of<TagComponent>(entity)) count++;
    if (registry.any_of<UUIDComponent>(entity)) count++;

    return count;
}

unsigned int PrefabImporter::CountEntitiesRecursively(Scene& scene, entt::entity e)
{
    unsigned int count = 1; // include self
    if (scene.HasComponent<SceneTreeComponent>(e))
    {
        auto& children = scene.GetComponent<SceneTreeComponent>(e).children;
        for (auto child : children)
            count += CountEntitiesRecursively(scene, child);
    }
    return count;
}
