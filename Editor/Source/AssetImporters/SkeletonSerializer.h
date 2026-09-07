#pragma once
#include "Asset/Types/SkeletonAsset.h"
#include "Core/Ref.h"
#include <filesystem>
#include <fstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace rv {

class SkeletonSerializer
{
public:
    static bool Save(const Ref<SkeletonAsset>& asset, const std::filesystem::path& path);
    static bool Load(const Ref<SkeletonAsset>& asset, const std::filesystem::path& path);
};

}