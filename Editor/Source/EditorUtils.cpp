#include "EditorUtils.h"
#include "GUI/Dialogs.h"
#include <fstream>
#include "Core/Engine.h"

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
