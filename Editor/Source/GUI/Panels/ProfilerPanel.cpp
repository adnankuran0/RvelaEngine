#include "ProfilerPanel.h"
#include "Core/Profiler.h"
#include "GUI/Style.h"
#include <imgui.h>
#include <glad/gl.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>

namespace rv {

static const float s_TargetFrameTimes[] = { 33.333f, 16.666f, 8.333f, 6.944f };
static const char* const s_TargetFPSLabels[] = { "30 FPS (33.3ms)", "60 FPS (16.6ms)", "120 FPS (8.3ms)", "144 FPS (6.9ms)" };

static const ImVec4 s_PassColors[] = {
    ImVec4(0.26f, 0.59f, 0.98f, 1.0f),
    ImVec4(0.98f, 0.59f, 0.26f, 1.0f),
    ImVec4(0.35f, 0.85f, 0.45f, 1.0f),
    ImVec4(0.85f, 0.35f, 0.65f, 1.0f),
    ImVec4(0.95f, 0.85f, 0.25f, 1.0f),
    ImVec4(0.45f, 0.85f, 0.85f, 1.0f),
    ImVec4(0.75f, 0.45f, 0.95f, 1.0f),
    ImVec4(0.95f, 0.45f, 0.45f, 1.0f)
};

void ProfilerPanel::Draw()
{
    if (!m_IsOpen) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
    if (ImGui::Begin(m_Name, &m_IsOpen))
    {
        DrawControls();
        ImGui::Spacing();
        DrawSummaryCards();
        ImGui::Spacing();
        DrawPerformanceGraphs();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("ProfilerTabs"))
        {
            if (ImGui::BeginTabItem("GPU Passes"))
            {
                DrawGPUTable();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("CPU Breakdown"))
            {
                DrawCPUTable();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Hardware"))
            {
                DrawHardwareAndMemory();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void ProfilerPanel::DrawControls()
{
    auto& profiler = Profiler::Get();

    bool isPaused = profiler.IsPaused();
    if (isPaused)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, GetWarningColor());
        if (ImGui::Button("Resume Capture", ImVec2(130.0f, 26.0f)))
            profiler.SetPaused(false);
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, GetAccentColor());
        if (ImGui::Button("Pause Capture", ImVec2(130.0f, 26.0f)))
            profiler.SetPaused(true);
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset Stats", ImVec2(100.0f, 26.0f)))
        profiler.ResetStats();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::Combo("Target", &m_TargetFPSIndex, s_TargetFPSLabels, IM_ARRAYSIZE(s_TargetFPSLabels));

    ImGui::SameLine();
    ImGui::Checkbox("Sort by Share %", &m_SortByShare);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::InputTextWithHint("##ScopeFilter", "Filter scopes...", m_Filter, sizeof(m_Filter));

    if (isPaused)
    {
        ImGui::SameLine();
        ImGui::TextColored(GetWarningColor(), "[PAUSED]");
    }
}

void ProfilerPanel::DrawSummaryCards()
{
    auto& profiler = Profiler::Get();

    float fps = profiler.GetFPS();
    float cpuMs = profiler.GetCPUFrameTime();
    float gpuMs = profiler.GetGPUFrameTime();
    const auto& stats = profiler.GetRenderStats();

    float availWidth = ImGui::GetContentRegionAvail().x;
    float cardWidth = std::max(120.0f, (availWidth - 30.0f) / 4.0f);
    float cardHeight = 75.0f;

    auto drawCard = [&](const char* title, const char* value, const char* subtitle, ImVec4 valColor)
    {
        ImGui::BeginChild(title, ImVec2(cardWidth, cardHeight), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::TextDisabled("%s", title);
        ImGui::TextColored(valColor, "%s", value);
        ImGui::TextDisabled("%s", subtitle);
        ImGui::EndChild();
    };

    ImVec4 fpsColor = (fps >= 55.0f) ? GetSuccessColor() : (fps >= 28.0f ? GetWarningColor() : GetErrorColor());
    char fpsBuf[32];
    snprintf(fpsBuf, sizeof(fpsBuf), "%.1f FPS", fps);
    char fpsSubBuf[32];
    snprintf(fpsSubBuf, sizeof(fpsSubBuf), "Frame: %.2f ms", (fps > 0.0f) ? (1000.0f / fps) : 0.0f);
    drawCard("Framerate", fpsBuf, fpsSubBuf, fpsColor);

    ImGui::SameLine();

    char cpuBuf[32];
    snprintf(cpuBuf, sizeof(cpuBuf), "%.2f ms", cpuMs);
    float targetMs = s_TargetFrameTimes[m_TargetFPSIndex];
    ImVec4 cpuColor = (cpuMs <= targetMs) ? GetSuccessColor() : GetWarningColor();
    drawCard("CPU Frame", cpuBuf, "Total CPU Loop", cpuColor);

    ImGui::SameLine();

    char gpuBuf[32];
    snprintf(gpuBuf, sizeof(gpuBuf), "%.2f ms", gpuMs);
    ImVec4 gpuColor = (gpuMs <= targetMs) ? GetSuccessColor() : GetWarningColor();
    drawCard("GPU Frame", gpuBuf, "Render Passes", gpuColor);

    ImGui::SameLine();

    char geomBuf[32];
    snprintf(geomBuf, sizeof(geomBuf), "%u Draws", stats.drawCalls);
    char geomSubBuf[48];
    if (stats.triangles >= 1000)
        snprintf(geomSubBuf, sizeof(geomSubBuf), "%.1fK Tris | %.1fK Verts", stats.triangles / 1000.0f, stats.vertices / 1000.0f);
    else
        snprintf(geomSubBuf, sizeof(geomSubBuf), "%u Tris | %u Verts", stats.triangles, stats.vertices);

    drawCard("Geometry", geomBuf, geomSubBuf, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void ProfilerPanel::DrawPerformanceGraphs()
{
    auto& profiler = Profiler::Get();

    const auto& cpuHist = profiler.GetCPUHistory();
    const auto& gpuHist = profiler.GetGPUHistory();
    size_t offset = profiler.GetHistoryOffset();

    float targetMs = s_TargetFrameTimes[m_TargetFPSIndex];
    float maxScale = std::max(targetMs * 1.5f, 20.0f);

    float availWidth = ImGui::GetContentRegionAvail().x;
    float graphWidth = (availWidth - 10.0f) * 0.5f;

    char cpuOverlay[64];
    snprintf(cpuOverlay, sizeof(cpuOverlay), "CPU: %.2f ms (Target: %.1fms)", profiler.GetCPUFrameTime(), targetMs);
    ImGui::PlotLines("##CPUPlot", cpuHist.data(), static_cast<int>(cpuHist.size()), static_cast<int>(offset), cpuOverlay, 0.0f, maxScale, ImVec2(graphWidth, 75.0f));

    ImGui::SameLine();

    char gpuOverlay[64];
    snprintf(gpuOverlay, sizeof(gpuOverlay), "GPU: %.2f ms (Target: %.1fms)", profiler.GetGPUFrameTime(), targetMs);
    ImGui::PlotLines("##GPUPlot", gpuHist.data(), static_cast<int>(gpuHist.size()), static_cast<int>(offset), gpuOverlay, 0.0f, maxScale, ImVec2(graphWidth, 75.0f));
}

void ProfilerPanel::DrawGPUTable()
{
    auto& profiler = Profiler::Get();
    const auto& samples = profiler.GetGPUSamples();
    float totalGPU = profiler.GetGPUFrameTime();

    if (totalGPU > 0.001f && !samples.empty())
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        float availX = ImGui::GetContentRegionAvail().x;
        float barHeight = 18.0f;
        ImVec2 p1 = ImVec2(p0.x + availX, p0.y + barHeight);

        drawList->AddRectFilled(p0, p1, IM_COL32(35, 38, 45, 255), 4.0f);

        float currentX = p0.x;
        for (size_t i = 0; i < samples.size(); ++i)
        {
            float segWidth = (samples[i].durationMs / totalGPU) * availX;
            if (segWidth < 1.0f) continue;

            const ImVec4& c = s_PassColors[i % IM_ARRAYSIZE(s_PassColors)];
            ImU32 col = ImColor(c);

            drawList->AddRectFilled(ImVec2(currentX, p0.y), ImVec2(currentX + segWidth, p1.y), col, 2.0f);
            currentX += segWidth;
        }

        ImGui::Dummy(ImVec2(availX, barHeight + 6.0f));
    }

    std::vector<size_t> indices(samples.size());
    for (size_t i = 0; i < samples.size(); ++i)
        indices[i] = i;

    if (m_SortByShare)
    {
        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            return samples[a].durationMs > samples[b].durationMs;
        });
    }

    static ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("GPUTable", 6, tableFlags))
    {
        ImGui::TableSetupColumn("Pass", ImGuiTableColumnFlags_WidthStretch, 0.35f);
        ImGui::TableSetupColumn("Time (ms)", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Share", ImGuiTableColumnFlags_WidthStretch, 0.25f);
        ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Min (ms)", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableHeadersRow();

        for (size_t idx : indices)
        {
            const auto& s = samples[idx];
            if (m_Filter[0] != '\0' && s.name.find(m_Filter) == std::string::npos)
                continue;

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            const ImVec4& color = s_PassColors[idx % IM_ARRAYSIZE(s_PassColors)];
            ImGui::ColorButton("##passColor", color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(12.0f, 12.0f));
            ImGui::SameLine();
            ImGui::Text("%s", s.name.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", s.durationMs);

            ImGui::TableNextColumn();
            float ratio = (totalGPU > 0.001f) ? (s.durationMs / totalGPU) : 0.0f;
            char progressBuf[32];
            snprintf(progressBuf, sizeof(progressBuf), "%.1f%%", ratio * 100.0f);
            ImGui::ProgressBar(ratio, ImVec2(-1.0f, 14.0f), progressBuf);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", s.avgMs);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", s.minMs);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", s.maxMs);
        }

        ImGui::EndTable();
    }
}

void ProfilerPanel::DrawCPUTable()
{
    auto& profiler = Profiler::Get();
    const auto& samples = profiler.GetCPUSamples();
    float totalCPU = profiler.GetCPUFrameTime();

    std::vector<size_t> indices(samples.size());
    for (size_t i = 0; i < samples.size(); ++i)
        indices[i] = i;

    if (m_SortByShare)
    {
        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            return samples[a].durationMs > samples[b].durationMs;
        });
    }

    static ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("CPUTable", 6, tableFlags))
    {
        ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch, 0.40f);
        ImGui::TableSetupColumn("Time (ms)", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Share", ImGuiTableColumnFlags_WidthStretch, 0.20f);
        ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Min (ms)", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableHeadersRow();

        for (size_t idx : indices)
        {
            const auto& s = samples[idx];
            if (m_Filter[0] != '\0' && s.name.find(m_Filter) == std::string::npos)
                continue;

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            if (!m_SortByShare && s.depth > 0)
                ImGui::Indent(s.depth * 14.0f);
            ImGui::Text("%s", s.name.c_str());
            if (!m_SortByShare && s.depth > 0)
                ImGui::Unindent(s.depth * 14.0f);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", s.durationMs);

            ImGui::TableNextColumn();
            float ratio = (totalCPU > 0.001f) ? std::min(s.durationMs / totalCPU, 1.0f) : 0.0f;
            char progressBuf[32];
            snprintf(progressBuf, sizeof(progressBuf), "%.1f%%", ratio * 100.0f);
            ImGui::ProgressBar(ratio, ImVec2(-1.0f, 14.0f), progressBuf);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", s.avgMs);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", s.minMs);

            ImGui::TableNextColumn();
            ImGui::Text("%.2f", s.maxMs);
        }

        ImGui::EndTable();
    }
}

void ProfilerPanel::DrawHardwareAndMemory()
{
    auto& profiler = Profiler::Get();

    ImGui::TextColored(GetAccentColor(), "OpenGL Hardware Info");
    ImGui::Separator();

    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* glslVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    ImGui::Text("GPU Device:   %s", renderer ? renderer : "Unknown");
    ImGui::Text("Vendor:       %s", vendor ? vendor : "Unknown");
    ImGui::Text("OpenGL Ver:   %s", version ? version : "Unknown");
    ImGui::Text("GLSL Ver:     %s", glslVersion ? glslVersion : "Unknown");

    ImGui::Spacing();
    ImGui::TextColored(GetAccentColor(), "Video Memory (VRAM)");
    ImGui::Separator();

    auto memInfo = profiler.GetGPUMemoryInfo();
    if (memInfo.isSupported)
    {
        int usedMB = memInfo.totalMemoryMB - memInfo.availableMemoryMB;
        float usageRatio = (memInfo.totalMemoryMB > 0) ? (static_cast<float>(usedMB) / static_cast<float>(memInfo.totalMemoryMB)) : 0.0f;

        char memBuf[64];
        snprintf(memBuf, sizeof(memBuf), "%d MB / %d MB (%.1f%%)", usedMB, memInfo.totalMemoryMB, usageRatio * 100.0f);
        ImGui::ProgressBar(usageRatio, ImVec2(-1.0f, 18.0f), memBuf);

        ImGui::Text("Available Dedicated Memory: %d MB", memInfo.availableMemoryMB);
    }
    else
    {
        ImGui::TextDisabled("Direct VRAM query not supported by graphics driver.");
    }

    ImGui::Spacing();
    ImGui::TextColored(GetAccentColor(), "Render Pipeline Statistics");
    ImGui::Separator();

    const auto& stats = profiler.GetRenderStats();
    ImGui::Columns(2, "StatColumns", false);
    ImGui::Text("Draw Calls:");
    ImGui::Text("Triangles:");
    ImGui::Text("Vertices:");
    ImGui::Text("Static Meshes:");
    ImGui::NextColumn();
    ImGui::Text("%u", stats.drawCalls);
    ImGui::Text("%u", stats.triangles);
    ImGui::Text("%u", stats.vertices);
    ImGui::Text("%u", stats.meshCount);
    ImGui::NextColumn();

    ImGui::Text("Skeletal Meshes:");
    ImGui::Text("Particles:");
    ImGui::Text("Decals:");
    ImGui::NextColumn();
    ImGui::Text("%u", stats.skeletalMeshCount);
    ImGui::Text("%u", stats.particleCount);
    ImGui::Text("%u", stats.decalCount);
    ImGui::Columns(1);
}

}
