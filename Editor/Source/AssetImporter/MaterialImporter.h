#pragma once
#include "AssetImporter.h"
#include "Core/Ref.h"
#include "Assets/MaterialAsset.h"
#include <assimp/scene.h>

class MaterialImporter 
{
public:

    inline static Ref<MaterialAsset> CreateMaterialAsset(const std::string& path)
    {
        std::unique_ptr<MaterialMeta> meta = std::make_unique<MaterialMeta>();
        Ref<MaterialAsset> asset = CreateRef<MaterialAsset>(path, std::move(meta));
        //path = s_UUIDToPath[meta->uuid];
        asset->Serialize();
        return asset;
    }

};