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
    j["sourceHash"] = sourceHash;
    j["importerSettings"] = importerSettingsJson.empty() ? json::object() : json::parse(importerSettingsJson);

    json deps = json::array();
    for (auto& dep : dependencies)
        deps.push_back(dep.ToString());
    j["dependencies"] = deps;

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
    sourceHash = j.value<uint64_t>("sourceHash",0);
    importerSettingsJson = j.contains("importerSettings") ? j["importerSettings"].dump() : "{}";

    dependencies.clear();
    if (j.contains("dependencies"))
        for (auto& dependency : j["dependencies"])
            dependencies.push_back(AssetUUID::FromString(dependency.get<std::string>()));

    return true;
}