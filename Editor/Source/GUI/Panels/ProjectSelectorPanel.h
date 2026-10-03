#pragma once
#include <string>

namespace rv {

class Engine;

class ProjectSelectorPanel
{
public:
    ProjectSelectorPanel() = default;
    ~ProjectSelectorPanel() = default;

    bool Draw(Engine* engine);

    void Open() { m_IsOpen = true; }
    void Close() { m_IsOpen = false; }
    bool IsOpen() const { return m_IsOpen; }

private:
    bool m_IsOpen = true;
    char m_NewProjectName[128] = "MyProject";
    char m_NewProjectPath[512] = "";
    std::string m_ErrorMessage = "";
};

}
