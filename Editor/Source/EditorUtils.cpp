#include "EditorUtils.h"
#include "GUI/Dialogs.h"
#include <fstream>
#include "Core/Engine.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetRegistry.h"

using namespace rv;

void EditorUtils::CreateScene(Engine& engine)
{
    auto& sm = engine.GetSceneManager();
    sm.SetActiveScene(sm.CreateScene("NewScene"));
}

void EditorUtils::OpenScene(Engine& engine)
{
    std::string file = Dialogs::OpenSceneDialog();
    if (!file.empty())
    {
        engine.GetSceneManager().LoadScene(file);
    }
}

bool EditorUtils::SaveScene(Engine& engine)
{
    Scene& activeScene = engine.GetActiveScene();
    if (activeScene.GetState() != SceneState::EDIT)
    {
        LOG_WARN("Scene can only be saved in EDIT mode.");
        return false;
    }
    if (!activeScene.GetPath().empty())
    {
        engine.GetSceneManager().SaveScene(activeScene.GetPath());
        return true;
    }
    else
    {
        return SaveSceneAs(engine);
    }
}

bool EditorUtils::SaveSceneAs(Engine& engine)
{
    Scene& activeScene = engine.GetActiveScene();
    if (activeScene.GetState() != SceneState::EDIT)
    {
        LOG_WARN("Scene can only be saved in EDIT mode.");
        return false;
    }
    std::string file = Dialogs::SaveSceneDialog();
    if (!file.empty())
    {
        std::ofstream ofs(file);
        if (ofs.is_open())
        {
            ofs.close();
            engine.GetSceneManager().SaveScene(file);
            return true;
        }
    }
    return false;
}

AssetUUID EditorUtils::ReadUUIDFromMeta(const std::string& assetPath)
{
    auto metaPath = assetPath + ".rmeta";
    if (!std::filesystem::exists(metaPath))
    {
        LOG_WARN("No .rmeta found for: {}", assetPath);
        return AssetUUID::Invalid();
    }

    AssetMeta meta;
    if (!meta.LoadFromFile(metaPath))
        return AssetUUID::Invalid();

    return meta.uuid;
}

static tsl::robin_map<AssetUUID, std::string> s_FileNameCache;

void EditorUtils::ClearAssetFileNameCache()
{
    s_FileNameCache.clear();
}

std::string EditorUtils::GetAssetFileName(const AssetUUID& uuid)
{
    if (!uuid.IsValid())
        return "";

    auto it = s_FileNameCache.find(uuid);
    if (it != s_FileNameCache.end())
        return it->second;

    auto path = AssetManager::Get().GetRegistry().GetPath(uuid);
    if (!path.empty())
    {
        std::string stem = path.stem().string();
        if (path.string().find(".cache") == std::string::npos && !AssetUUID::FromString(stem).IsValid())
        {
            std::string filename = path.filename().string();
            s_FileNameCache[uuid] = filename;
            return filename;
        }
    }

    // Fallback if not found in memory, scan assetDir once and cache all found UUIDs so we never scan disk again
    const auto& assetDir = AssetManager::Get().GetRegistry().GetAssetDir();
    if (!assetDir.empty() && std::filesystem::exists(assetDir))
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(assetDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".rmeta")
            {
                if (entry.path().string().find(".cache") != std::string::npos)
                    continue;

                AssetMeta meta;
                if (meta.LoadFromFile(entry.path()))
                {
                    auto srcPath = entry.path();
                    srcPath.replace_extension("");
                    std::string fn = srcPath.filename().string();

                    s_FileNameCache[meta.uuid] = fn;
                    for (const auto& sub : meta.subAssets)
                    {
                        if (sub.uuid.IsValid())
                            s_FileNameCache[sub.uuid] = fn;
                    }
                }
            }
        }
    }

    auto cachedIt = s_FileNameCache.find(uuid);
    if (cachedIt != s_FileNameCache.end())
        return cachedIt->second;

    // Cache empty string to avoid rescanning filesystem for nonexistent assets
    s_FileNameCache[uuid] = "";
    return "";
}
