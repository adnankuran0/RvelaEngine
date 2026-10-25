#pragma once
#include "EditorPanel.h"
#include <string>
#include <vector>

namespace rv {

class Engine;

class ProjectSelectorPanel : public EditorPanel
{
public:
    ProjectSelectorPanel() : EditorPanel("Project Selector", true) {}
    ~ProjectSelectorPanel() override = default;

    bool Draw(Engine* engine);

    void Open() override;
    void RefreshRecents();

private:
    bool m_RecentsLoaded = false;
    std::vector<std::string> m_CachedRecents;
    char m_NewProjectName[128] = "MyProject";
    char m_NewProjectPath[512] = "";
    std::string m_ErrorMessage = "";
};

}