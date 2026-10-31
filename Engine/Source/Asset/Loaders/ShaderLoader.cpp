#include "rvelapch.h"
#include "ShaderLoader.h"
#include "Asset/Types/ShaderAsset.h"
#include "Asset/AssetManager.h"
#include "Core/Log.h"
#include <fstream>
#include <sstream>
#include "Asset/AssetMeta.h"

namespace rv {

Ref<Asset> ShaderLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    std::ifstream file(assetPath);
    if (!file)
    {
        LOG_ERROR("Cannot open shader file: {}", assetPath.string());
        return nullptr;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    auto asset = CreateRef<ShaderAsset>(meta.uuid);
    asset->filePath = assetPath;
    std::error_code ec;
    asset->lastWriteTime = std::filesystem::last_write_time(assetPath, ec);
    asset->source = std::move(source);
    asset->Compile();
    return asset;
}

Ref<Asset> ShaderLoader::LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta)
{
    if (!data || size == 0)
    {
        LOG_ERROR("ShaderLoader::LoadFromMemory: empty buffer for shader {}", meta.uuid.ToString());
        return nullptr;
    }

    std::string source(reinterpret_cast<const char*>(data), size);
    auto asset = CreateRef<ShaderAsset>(meta.uuid);
    asset->filePath = AssetManager::Get().GetRegistry().GetPath(meta.uuid);
    asset->source = std::move(source);
    asset->Compile();
    return asset;
}

}
