#pragma once
#include "Assets/MaterialAsset.h"
#include "Assets/TextureAsset.h"

namespace rv { 

template<typename T>
struct AssetMagic {};

template<>
struct AssetMagic<MaterialMeta> {
    static constexpr uint32_t magic = MAGIC_MATERIAL;
};

template<>
struct AssetMagic<TextureMeta> {
    static constexpr uint32_t magic = MAGIC_TEXTURE;
};

}