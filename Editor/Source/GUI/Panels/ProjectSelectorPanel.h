#pragma once
#include <string>
#include <vector>

namespace rv {

class Engine;

class ProjectSelectorPanel
{
public:
    ProjectSelectorPanel() = default;
    ~ProjectSelectorPanel() = default;

    bool Draw(Engine* engine);

    void Open();
    void Close() { m_IsOpen = false; }
    bool IsOpen() const { return m_IsOpen; }
    void RefreshRecents();

private:
    bool m_IsOpen = true;
    bool m_RecentsLoaded = false;
    std::vector<std::string> m_CachedRecents;
    char m_NewProjectName[128] = "MyProject";
    char m_NewProjectPath[512] = "";
    std::string m_ErrorMessage = "";
};

}
