#pragma once

namespace rv {

class EditorPanel
{
public:
    virtual ~EditorPanel() = default;

    virtual void Open() { m_IsOpen = true; }
    virtual void Close() { m_IsOpen = false; }
    bool IsOpen() const { return m_IsOpen; }
    void SetOpen(bool open) { if (open) Open(); else Close(); }
    void Toggle() { SetOpen(!m_IsOpen); }

    const char* GetName() const { return m_Name; }

protected:
    EditorPanel() = default;
    explicit EditorPanel(const char* name, bool defaultOpen = true)
        : m_Name(name), m_IsOpen(defaultOpen) {}

    const char* m_Name = "";
    bool m_IsOpen = true;
};

}
