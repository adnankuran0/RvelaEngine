#pragma once
#include "Asset/Types/MaterialAsset.h"
#include "Asset/AssetRegistry.h"
#include <filesystem>
#include <fstream>
#include "Core/Ref.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace rv {

class MaterialSerializer
{
public:
    static Ref<MaterialAsset> CreateNew(const std::filesystem::path& path, AssetRegistry& registry);
    static bool Save(const Ref<MaterialAsset>& asset, const std::filesystem::path& path);
    static bool Load(const Ref<MaterialAsset>& asset, const std::filesystem::path& path);
};

}