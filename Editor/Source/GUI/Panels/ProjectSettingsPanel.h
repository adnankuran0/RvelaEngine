#pragma once
#include <string>

namespace rv {

class Engine;

class ProjectSettingsPanel
{
public:
    ProjectSettingsPanel() = default;
    ~ProjectSettingsPanel() = default;

    void Draw(Engine* engine);

    void Open() { m_IsOpen = true; }
    void Close() { m_IsOpen = false; }
    bool IsOpen() const { return m_IsOpen; }

private:
    bool m_IsOpen = false;
    char m_NameBuf[128] = "";
    char m_StartSceneBuf[256] = "";
    char m_AutoloadNameBuf[128] = "";
    char m_AutoloadPathBuf[256] = "";
    char m_PhysicsLayerBuf[32][64] = {};
    std::string m_AutoloadError;
    bool m_Initialized = false;
};

}
