
#include "PrefabImporter.h"

#include "Assets/PrefabAsset.h"
#include "Scene/Scene.h"
#include "Utils/Serializer.h"

Ref<PrefabAsset> PrefabImporter::CreatePrefabAsset(const std::string& path, Scene& scene, entt::entity& rootEntity)
{
    std::unique_ptr<PrefabMeta> meta = std::make_unique<PrefabMeta>();
    meta->entityCount = CountEntitiesRecursively(scene, rootEntity);

    Ref<PrefabAsset> asset = CreateRef<PrefabAsset>(path, std::move(meta));

    auto entites = scene.GetRegistry().view<UUIDComponent>();

    std::vector<std::byte> buffer;
    json j;

    SerializeEntityRecursively(rootEntity, rootEntity,scene, buffer,j);

    asset->SetData(std::move(buffer));

    asset->Serialize();

    return asset;
}

void PrefabImporter::SerializeEntityRecursively(entt::entity& e, entt::entity& rootEntity,Scene& scene, std::vector<std::byte>& buffer, json& j)
{
    // serialize component count
    unsigned int componentCount = scene.GetComponentCount(e);
    const std::byte* data = reinterpret_cast<const std::byte*>(&componentCount);
    buffer.insert(buffer.end(), data, data + sizeof(unsigned int));


    // serialize all components
    auto& uuidComp = scene.GetComponent<UUIDComponent>(e);
    SerializeBin_UUIDComp(uuidComp, buffer);
    
    if (scene.HasComponent<TagComponent>(e))
    {
        auto& comp = scene.GetComponent<TagComponent>(e);
        SerializeBin_TagComp(comp, buffer);
    }

    if (scene.HasComponent<TransformComponent>(e))
    {
        auto comp = scene.GetComponent<TransformComponent>(e); 

        if (e == rootEntity)
        {
            comp.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
            comp.SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        }

        SerializeBin_TransformComp(comp, buffer);
    }

    if (scene.HasComponent<MaterialComponent>(e))
    {
        auto& comp = scene.GetComponent<MaterialComponent>(e);
        comp.GetMaterial()->Serialize();
        SerializeBin_MaterialComp(comp, buffer);
    }

    if (scene.HasComponent<SceneTreeComponent>(e))
    {
        auto comp = scene.GetComponent<SceneTreeComponent>(e);

        if (e == rootEntity)
            comp.parentUUID = 0;

        SerializeBin_SceneTreeComp(comp, buffer);
    }

    if (scene.HasComponent<MeshComponent>(e))
    {
        auto& comp = scene.GetComponent<MeshComponent>(e);
        SerializeBin_MeshComp(comp, buffer);
    }

    if (scene.HasComponent<MeshRendererComponent>(e))
    {
        auto& comp = scene.GetComponent<MeshRendererComponent>(e);
        
        SerializeBin_MeshRendererComp(comp, buffer);
    }

    if (scene.HasComponent<PointLightComponent>(e))
    {
        auto& comp = scene.GetComponent<PointLightComponent>(e);
        SerializeBin_PointLightComp(comp, buffer);
    }

    if (scene.HasComponent<DirectionalLightComponent>(e))
    {
        auto& comp = scene.GetComponent<DirectionalLightComponent>(e);
        SerializeBin_DirectionalLightComp(comp, buffer);
    }

    if (scene.HasComponent<CameraComponent>(e))
    {
        auto& comp = scene.GetComponent<CameraComponent>(e);
        SerializeBin_CameraComp(comp, buffer);
    }


    if (scene.HasComponent<SceneTreeComponent>(e))
    {
        auto& children = scene.GetComponent<SceneTreeComponent>(e).children;
        if (children.empty()) return;

        for (auto& child : children)
        {
            SerializeEntityRecursively(child, rootEntity, scene, buffer,j);
        }
    }

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
