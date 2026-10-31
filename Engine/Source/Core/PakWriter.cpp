#include "rvelapch.h"
#include "PakWriter.h"
#include "Core/Log.h"
#include "Utils/FileUtils.h"
#include "Asset/AssetRegistry.h"
#include "Asset/AssetMeta.h"
#include <miniz/miniz.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <unordered_set>

using json = nlohmann::json;

namespace rv {

static std::string SafePathToString(const std::filesystem::path& p)
{
    try
    {
        auto u8 = p.u8string();
        return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
    }
    catch (...)
    {
        try
        {
            return p.generic_string();
        }
        catch (...)
        {
            return "";
        }
    }
}

bool PakWriter::ShouldStoreUncompressed(const std::string& extension)
{
    static const std::unordered_set<std::string> s_StoreExtensions = {
        ".rtex", ".rmesh", ".rskmesh", ".rskeleton", ".rskel", ".ranim", ".ranimlib",
        ".mp3", ".wav", ".ogg", ".flac",
        ".png", ".jpg", ".jpeg", ".dds", ".tga", ".hdr", ".bmp",
        ".fbx", ".obj", ".gltf", ".glb", ".blend",
        ".zip", ".rpak", ".dll", ".exe", ".ttf", ".otf"
    };

    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return s_StoreExtensions.find(ext) != s_StoreExtensions.end();
}

PakBuildResult PakWriter::BuildPak(const PakBuildOptions& options)
{
    PakBuildResult result;

    std::error_code ec;
    if (options.projectPath.empty() || !std::filesystem::exists(options.projectPath, ec))
    {
        result.errorMessage = "Project directory does not exist: " + SafePathToString(options.projectPath);
        LOG_ERROR("[PakWriter] {}", result.errorMessage);
        return result;
    }

    if (options.outputPakPath.empty())
    {
        result.errorMessage = "Output PAK path is empty.";
        LOG_ERROR("[PakWriter] {}", result.errorMessage);
        return result;
    }

    std::filesystem::create_directories(options.outputPakPath.parent_path(), ec);

    if (std::filesystem::exists(options.outputPakPath, ec))
    {
        std::filesystem::remove(options.outputPakPath, ec);
    }

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    std::string outPakStr = SafePathToString(options.outputPakPath);
    if (!mz_zip_writer_init_file(&zip, outPakStr.c_str(), 0))
    {
        result.errorMessage = "Failed to initialize zip writer at: " + outPakStr;
        LOG_ERROR("[PakWriter] {}", result.errorMessage);
        return result;
    }

    struct FileEntry
    {
        std::string archivePath;
        std::filesystem::path diskPath;
        uint32_t compressionLevel = MZ_BEST_SPEED;
    };
    std::vector<FileEntry> filesToPack;

    std::filesystem::path rprojPath;
    for (const auto& entry : std::filesystem::directory_iterator(options.projectPath, std::filesystem::directory_options::skip_permission_denied, ec))
    {
        if (entry.path().extension() == ".rproj")
        {
            rprojPath = entry.path();
            break;
        }
    }

    if (!rprojPath.empty())
    {
        filesToPack.push_back({ "project.rproj", rprojPath, MZ_BEST_SPEED });
        filesToPack.push_back({ SafePathToString(rprojPath.filename()), rprojPath, MZ_BEST_SPEED });
    }
    else
    {
        LOG_WARN("[PakWriter] No .rproj file found in project root: {}", SafePathToString(options.projectPath));
    }

    const std::vector<std::string> rootIconCandidates = {
        "icon.png", "icon.ico", "icon.jpg", "icon.jpeg", "Game.png", "Game.ico"
    };
    for (const auto& candidate : rootIconCandidates)
    {
        std::filesystem::path p = options.projectPath / candidate;
        if (std::filesystem::exists(p, ec))
        {
            filesToPack.push_back({ candidate, p, MZ_NO_COMPRESSION });
        }
    }

    static const std::unordered_set<std::string> s_RawModelExtensions = {
        ".fbx", ".obj", ".gltf", ".glb", ".blend", ".dae", ".3ds", ".stl", ".ply"
    };

    static const std::unordered_set<std::string> s_RawTextureExtensions = {
        ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".psd", ".hdr"
    };

    std::filesystem::path assetsDir = options.projectPath / "Assets";
    if (std::filesystem::exists(assetsDir, ec))
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsDir, std::filesystem::directory_options::skip_permission_denied, ec))
        {
            if (!entry.is_regular_file(ec)) continue;

            std::string extLower = SafePathToString(entry.path().extension());
            std::transform(extLower.begin(), extLower.end(), extLower.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            if (s_RawModelExtensions.contains(extLower))
            {
                continue;
            }

            if (s_RawTextureExtensions.contains(extLower))
            {
                std::filesystem::path metaPath = entry.path();
                metaPath += ".rmeta";
                if (std::filesystem::exists(metaPath, ec))
                {
                    continue;
                }
            }

            auto relPath = std::filesystem::relative(entry.path(), options.projectPath, ec);
            std::string archivePath = SafePathToString(relPath);
            std::replace(archivePath.begin(), archivePath.end(), '\\', '/');

            uint32_t compLevel = ShouldStoreUncompressed(extLower)
                ? MZ_NO_COMPRESSION
                : MZ_BEST_SPEED;

            filesToPack.push_back({ archivePath, entry.path(), compLevel });
        }
    }

    if (options.includeEngineResources)
    {
        std::filesystem::path engineRes = Path::GetEngineResourcesPath();
        if (std::filesystem::exists(engineRes, ec))
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(engineRes, std::filesystem::directory_options::skip_permission_denied, ec))
            {
                if (!entry.is_regular_file(ec)) continue;

                auto relPath = std::filesystem::relative(entry.path(), engineRes, ec);
                std::string archivePath = "Resources/Engine/" + SafePathToString(relPath);
                std::replace(archivePath.begin(), archivePath.end(), '\\', '/');

                std::string extLower = SafePathToString(entry.path().extension());
                std::transform(extLower.begin(), extLower.end(), extLower.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });

                uint32_t compLevel = ShouldStoreUncompressed(extLower)
                    ? MZ_NO_COMPRESSION
                    : MZ_BEST_SPEED;

                filesToPack.push_back({ archivePath, entry.path(), compLevel });
            }
        }
    }

    json manifest = json::array();
    for (const auto& file : filesToPack)
    {
        if (file.diskPath.extension() == ".rmeta")
        {
            std::ifstream metaFile(file.diskPath);
            if (metaFile.is_open())
            {
                try
                {
                    json metaJson = json::parse(metaFile);
                    if (metaJson.contains("uuid"))
                    {
                        std::string targetAssetPath = file.archivePath.substr(0, file.archivePath.length() - 6); // remove .rmeta
                        json item = {
                            { "uuid", metaJson["uuid"] },
                            { "path", targetAssetPath },
                            { "meta", metaJson }
                        };
                        manifest.push_back(item);
                    }
                }
                catch (...) {}
            }
        }
    }

    std::string manifestStr = manifest.dump();
    if (!mz_zip_writer_add_mem(&zip, "manifest.json", manifestStr.data(), manifestStr.size(), MZ_BEST_SPEED))
    {
        LOG_WARN("[PakWriter] Failed to add manifest.json to archive.");
    }
    else
    {
        result.totalFiles++;
        result.uncompressedBytes += manifestStr.size();
    }

    size_t fileCount = filesToPack.size();
    for (size_t i = 0; i < fileCount; ++i)
    {
        const auto& file = filesToPack[i];
        if (options.progressCallback)
        {
            float prog = static_cast<float>(i) / static_cast<float>(fileCount);
            options.progressCallback(file.archivePath, prog);
        }

        uintmax_t fileSize = std::filesystem::file_size(file.diskPath, ec);
        if (ec) fileSize = 0;

        std::string diskPathStr = SafePathToString(file.diskPath);
        if (!mz_zip_writer_add_file(&zip, file.archivePath.c_str(), diskPathStr.c_str(), NULL, 0, file.compressionLevel))
        {
            LOG_WARN("[PakWriter] Failed to add file to archive: {}", file.archivePath);
            continue;
        }

        result.totalFiles++;
        result.uncompressedBytes += fileSize;
    }

    if (options.progressCallback)
    {
        options.progressCallback("Finalizing archive...", 0.98f);
    }

    if (!mz_zip_writer_finalize_archive(&zip))
    {
        result.errorMessage = "Failed to finalize zip archive.";
        LOG_ERROR("[PakWriter] {}", result.errorMessage);
        mz_zip_writer_end(&zip);
        return result;
    }

    mz_zip_writer_end(&zip);

    if (options.progressCallback)
    {
        options.progressCallback("Done!", 1.0f);
    }

    std::error_code sizeEc;
    result.compressedBytes = std::filesystem::file_size(options.outputPakPath, sizeEc);
    result.success = true;

    return result;
}

}
