#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include "json.hpp"
#include "Scene/UUIDGenerator.h"

struct AssetMeta {
    UUID uuid;
    std::string type;

    static AssetMeta LoadFromFile(const std::filesystem::path& metaPath) {
        std::ifstream file(metaPath);
        if (!file.is_open())
            throw std::runtime_error("Failed to open meta file: " + metaPath.string());

        nlohmann::json j;
        file >> j;

        AssetMeta meta;
        meta.uuid = j["UUID"];
        meta.type = j["Type"];
        return meta;
    }

    void SaveToFile(const std::filesystem::path& metaPath) const {
        nlohmann::json j;
        j["UUID"] = uuid;
        j["Type"] = type;

        std::ofstream file(metaPath);
        if (!file.is_open())
            throw std::runtime_error("Failed to write meta file: " + metaPath.string());

        file << j.dump(4);
    }

    static std::filesystem::path GetMetaPath(const std::filesystem::path& assetPath) {
        return assetPath.string() + ".meta";
    }

    static bool Exists(const std::filesystem::path& assetPath) {
        return std::filesystem::exists(GetMetaPath(assetPath));
    }
};
