#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Assets/Asset.h"
#include "Assets/AssetMeta.h"

class PrefabMeta : public AssetMeta
{
public:
    std::unique_ptr<AssetMeta> Clone() const override;
    void Serialize(std::vector<char>& outBuffer, size_t& offset) const override;
    void Deserialize(const std::vector<char>& inBuffer, size_t& offset) override;

public:
    uint32_t entityCount = 0;
};

class PrefabAsset : public Asset
{
public:
    PrefabAsset(const std::string& path, std::unique_ptr<PrefabMeta> prefabMeta);
    ~PrefabAsset();

    bool Load();
    void Serialize();
    void Unload();

    const std::vector<std::byte>& GetData() const;
    void SetData(std::vector<std::byte>&& buffer);

    const std::string& GetPath() const;
    bool IsLoaded() const;

private:
    static std::vector<uint8_t> ReadPrefabData(const std::string& filePath);

private:
    std::string m_Path;
    std::vector<std::byte> m_Data;
    bool m_Loaded = false;
};
