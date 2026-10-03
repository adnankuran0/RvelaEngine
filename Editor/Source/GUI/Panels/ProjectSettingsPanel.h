#pragma once

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
    bool m_Initialized = false;
};

}
