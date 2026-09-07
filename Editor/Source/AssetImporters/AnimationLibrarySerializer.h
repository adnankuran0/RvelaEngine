#pragma once
#include "Asset/Types/AnimationLibraryAsset.h"
#include "Asset/AssetRegistry.h"
#include "Animation/IPropertyTrack.h"
#include "Core/Ref.h"
#include "json.hpp"
#include <fstream>

namespace rv {

class AnimationLibrarySerializer
{
public:
    static Ref<AnimationLibraryAsset> CreateNew(const std::filesystem::path& path, AssetRegistry& registry);
    static bool Save(const Ref<AnimationLibraryAsset>& asset, const std::filesystem::path& path);
};

}