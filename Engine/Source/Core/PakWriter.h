#pragma once

#include <filesystem>
#include <string>
#include <cstdint>
#include <functional>

namespace rv {

struct PakBuildOptions
{
    std::filesystem::path projectPath;
    std::filesystem::path outputPakPath;
    bool includeEngineResources = true;
    std::function<void(const std::string& currentFile, float progress)> progressCallback = nullptr;
};

struct PakBuildResult
{
    bool success = false;
    std::string errorMessage;
    uint32_t totalFiles = 0;
    uint64_t uncompressedBytes = 0;
    uint64_t compressedBytes = 0;
};

class PakWriter
{
public:
    static PakBuildResult BuildPak(const PakBuildOptions& options);

private:
    static bool ShouldStoreUncompressed(const std::string& extension);
};

}
