#include "EditorUtils.h"
#include "GUI/Dialogs.h"
#include <fstream>
#include "Core/Engine.h"

using namespace rv;

void rv::EditorUtils::CreateScene(Engine& engine)
{
    auto& sm = engine.GetSceneManager();
    sm.SetActiveScene(sm.CreateScene("NewScene"));
}

void rv::EditorUtils::OpenScene(Engine& engine)
{
    std::string file = Dialogs::OpenSceneDialog();
    if (!file.empty())
    {
        engine.GetSceneManager().LoadScene(file);
    }
}

void rv::EditorUtils::SaveScene(Engine& engine)
{
    Scene& activeScene = engine.GetActiveScene();
    if (activeScene.GetState() != SceneState::EDIT)
    {
        LOG_WARN("Scene can only be saved in EDIT mode.");
        return;
    }
    if (!activeScene.GetPath().empty())
        engine.GetSceneManager().SaveScene(activeScene.GetPath());
    else
        SaveSceneAs(engine);
}

void rv::EditorUtils::SaveSceneAs(Engine& engine)
{
    Scene& activeScene = engine.GetActiveScene();
    if (activeScene.GetState() != SceneState::EDIT)
    {
        LOG_WARN("Scene can only be saved in EDIT mode.");
        return;
    }
    std::string file = Dialogs::OpenSceneDialog();
    if (!file.empty())
    {
        std::ofstream ofs(file);
        if (ofs.is_open())
        {
            ofs.close();
            engine.GetSceneManager().SaveScene(file);
        }
    }
}

