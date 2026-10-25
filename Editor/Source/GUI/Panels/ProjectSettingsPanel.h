#pragma once
#include "EditorPanel.h"
#include <string>

namespace rv {

class Engine;

class ProjectSettingsPanel : public EditorPanel
{
public:
    ProjectSettingsPanel() : EditorPanel("Project Settings", false) {}
    ~ProjectSettingsPanel() override = default;

    void Draw(Engine* engine);

private:
    char m_NameBuf[128] = "";
    char m_StartSceneBuf[256] = "";
    char m_AutoloadNameBuf[128] = "";
    char m_AutoloadPathBuf[256] = "";
    char m_PhysicsLayerBuf[32][64] = {};
    std::string m_AutoloadError;
    bool m_Initialized = false;
};

}