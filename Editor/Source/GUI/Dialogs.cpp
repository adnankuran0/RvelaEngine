#include "Dialogs.h"
#include <ImGui/tinyfiledialogs.h>
#include "Utils/ProjectManager.h"

using namespace rv;

std::string rv::Dialogs::OpenSceneDialog()
{
    constexpr const char* filterPatterns[] = { "*.rscene" };
    std::string defaultPath = ProjectManager::IsProjectLoaded()
        ? ProjectManager::GetAssetDirectory().string()
        : "";
    const char* filePath = tinyfd_openFileDialog("Select a scene", defaultPath.c_str(), 1, filterPatterns, nullptr, 0);
    return filePath ? filePath : "";
}

std::string rv::Dialogs::SaveSceneDialog()
{
    const char* filterPatterns[] = { "*.rscene" };
    std::string defaultPath = ProjectManager::IsProjectLoaded()
        ? (ProjectManager::GetAssetDirectory() / "Scenes" / "scene.rscene").string()
        : "scene.rscene";
    const char* filePath = tinyfd_saveFileDialog("Save Scene As", defaultPath.c_str(), 1, filterPatterns, NULL);

    return filePath ? filePath : "";
}

std::string rv::Dialogs::OpenProjectDialog()
{
    constexpr const char* filterPatterns[] = { "*.rproj" };
    std::string defaultPath = ProjectManager::IsProjectLoaded()
        ? ProjectManager::GetProjectPath().string()
        : "";
    const char* filePath = tinyfd_openFileDialog("Select a Project", defaultPath.c_str(), 1, filterPatterns, nullptr, 0);
    return filePath ? filePath : "";
}

std::string rv::Dialogs::SelectFolderDialog(const std::string& title)
{
    std::string defaultPath = ProjectManager::IsProjectLoaded()
        ? ProjectManager::GetProjectPath().parent_path().string()
        : "";
    const char* folderPath = tinyfd_selectFolderDialog(title.c_str(), defaultPath.c_str());
    return folderPath ? folderPath : "";
}

