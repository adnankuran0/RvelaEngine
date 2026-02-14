#include "MaterialImporter.h"
#include "Assets/MaterialAsset.h"

Ref<MaterialAsset> MaterialImporter::CreateMaterialAsset(const std::string& path)
{
    std::unique_ptr<MaterialMeta> meta = std::make_unique<MaterialMeta>();
    Ref<MaterialAsset> asset = CreateRef<MaterialAsset>(path, std::move(meta));
    //path = s_UUIDToPath[meta->uuid];
    asset->Serialize();
    return asset;
}