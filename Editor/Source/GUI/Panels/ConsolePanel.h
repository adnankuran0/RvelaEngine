#pragma once
#include <string>

namespace rv {

class ConsolePanel
{
public:
    ConsolePanel() = default;
    ~ConsolePanel() = default;

    void Draw();

private:
    bool m_ShowTrace = true;
    bool m_ShowInfo = true;
    bool m_ShowWarn = true;
    bool m_ShowError = true;
    bool m_ShowLua = true;
    bool m_AutoScroll = true;
    char m_FilterBuffer[128] = "";
};

} // namespace rv