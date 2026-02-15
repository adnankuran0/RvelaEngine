#pragma once

#include "Assets/AssetMeta.h"
#include "Assets/Asset.h"
#include <vector>
#include <memory>
#include <string>
#include <filesystem>

namespace rv {

class SceneMeta : public AssetMeta
{
public:
    std::unique_ptr<AssetMeta> Clone() const override;

    void Serialize(std::vector<char>& outBuffer, size_t& offset) const override;
    void Deserialize(const std::vector<char>& inBuffer, size_t& offset) override;

public:
    uint32_t entityCount = 0;
};

class SceneAsset : public Asset
{
public:
    SceneAsset(const std::string& path, std::unique_ptr<SceneMeta> sceneMeta);
    ~SceneAsset();

    bool Load();
    void Serialize();
    void Unload();

    const std::vector<std::byte>& GetData() const;
    void SetData(std::vector<std::byte>&& buffer);

    const std::string& GetPath() const;
    bool IsLoaded() const;

private:
    static std::vector<uint8_t> ReadSceneData(const std::filesystem::path& filePath);

private:
    std::string m_Path;
    std::vector<std::byte> m_Data;
    bool m_Loaded = false;
};

}