#include "rvelapch.h"
#include "AssetMeta.h"
#include <nlohmann/json.hpp>

using namespace rv;

using json = nlohmann::json;

bool AssetMeta::SaveToFile(const std::filesystem::path& metaPath) const
{
    json j;
    j["uuid"] = uuid.ToString();
    j["importerID"] = importerID;
    j["lastWriteTime"] = lastWriteTime;
    j["importerSettings"] = importerSettingsJson.empty() ? json::object() : json::parse(importerSettingsJson);

    json deps = json::array();
    for (auto& dep : dependencies)
        deps.push_back(dep.ToString());
    j["dependencies"] = deps;

    json jSubAssets = json::array();
    for (auto& sub : subAssets)
    {
        jSubAssets.push_back({
            {"uuid",  sub.uuid.ToString()},
            {"name",  sub.name},
            {"type",  sub.type},
            {"index", sub.index},
            {"hasCache", sub.hasCache}
            });
    }
    j["subAssets"] = jSubAssets;

    std::ofstream file(metaPath);
    if (!file)
    {
        LOG_ERROR("Cannot write: {}", metaPath.string());
        return false;
    }
    file << j.dump(4);
    return true;
}

bool AssetMeta::LoadFromFile(const std::filesystem::path& metaPath)
{
    std::ifstream file(metaPath);
    if (!file)
    {
        LOG_ERROR("Cannot read: {}", metaPath.string());
        return false;
    }

    json j;
    try
    {
        j = json::parse(file);
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("JSON parse error: {}", e.what());
        return false;
    }

    uuid = AssetUUID::FromString(j.value("uuid", ""));
    importerID = j.value("importerID", "");

    if (j.contains("lastWriteTime"))
        lastWriteTime = j.value<uint64_t>("lastWriteTime", 0);
    else if (j.contains("sourceHash"))
        lastWriteTime = j.value<uint64_t>("sourceHash", 0);

    importerSettingsJson = j.contains("importerSettings") ? j["importerSettings"].dump() : "{}";

    dependencies.clear();
    if (j.contains("dependencies"))
        for (auto& dependency : j["dependencies"])
            dependencies.push_back(AssetUUID::FromString(dependency.get<std::string>()));

    subAssets.clear();
    if (j.contains("subAssets"))
    {
        for (auto& jSub : j["subAssets"])
        {
            SubAssetEntry entry;
            entry.uuid = AssetUUID::FromString(jSub.value("uuid", ""));
            entry.name = jSub.value("name", "");
            entry.type = jSub.value("type", "");
            entry.index = jSub.value("index", 0u);
            entry.hasCache = jSub.value("hasCache", false);
            subAssets.push_back(entry);
        }
    }

    return true;
}

bool AssetMeta::IsModified(const std::filesystem::path& path) const
{
    std::error_code ec;
    auto currentTime = std::filesystem::last_write_time(path, ec);
    if (ec) return true;

    auto currentTimeStamp = std::chrono::duration_cast<std::chrono::seconds>(
        currentTime.time_since_epoch()).count();

    return currentTimeStamp != lastWriteTime;
}