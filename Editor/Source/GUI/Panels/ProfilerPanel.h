#pragma once

#include "EditorPanel.h"
#include <string>

namespace rv {

class ProfilerPanel : public EditorPanel
{
public:
    ProfilerPanel() : EditorPanel("Profiler", true) {}
    ~ProfilerPanel() override = default;

    void Draw();

private:
    void DrawControls();
    void DrawSummaryCards();
    void DrawPerformanceGraphs();
    void DrawGPUTable();
    void DrawCPUTable();
    void DrawHardwareAndMemory();

    char m_Filter[64] = "";
    int m_TargetFPSIndex = 1;
    bool m_SortByShare = false;
};

}
