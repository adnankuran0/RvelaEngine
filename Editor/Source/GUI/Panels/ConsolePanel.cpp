#include "ConsolePanel.h"
#include "Core/EditorConsoleSink.h"
#include "ImGui/imgui.h"

using namespace rv;
void ConsolePanel::Draw()
{
    ImGui::Begin("Console");

    auto& sink = EditorConsoleSink::Get();

    if (ImGui::Button("Clear"))
        sink.Clear();

    ImGui::SameLine();
    ImGui::Checkbox("Auto-Scroll", &m_AutoScroll);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::InputTextWithHint("##Filter", "Filter...", m_FilterBuffer, sizeof(m_FilterBuffer));

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    char buf[32];
    snprintf(buf, sizeof(buf), "Info (%u)", sink.GetInfoCount());
    ImGui::Checkbox(buf, &m_ShowInfo);
    ImGui::SameLine();

    snprintf(buf, sizeof(buf), "Warn (%u)", sink.GetWarnCount());
    ImGui::Checkbox(buf, &m_ShowWarn);
    ImGui::SameLine();

    snprintf(buf, sizeof(buf), "Error (%u)", sink.GetErrorCount());
    ImGui::Checkbox(buf, &m_ShowError);
    ImGui::SameLine();

    snprintf(buf, sizeof(buf), "Lua (%u)", sink.GetLuaCount());
    ImGui::Checkbox(buf, &m_ShowLua);

    ImGui::Separator();

    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    std::string searchFilter(m_FilterBuffer);

    sink.ForEachMessage([&](const std::vector<LogMessage>& messages)
        {
            for (size_t i = 0; i < messages.size(); ++i)
            {
                const auto& msg = messages[i];

                if (!m_ShowTrace && (msg.level == LogLevel::Trace || msg.level == LogLevel::Debug)) continue;
                if (!m_ShowInfo && msg.level == LogLevel::Info) continue;
                if (!m_ShowWarn && msg.level == LogLevel::Warn) continue;
                if (!m_ShowError && (msg.level == LogLevel::Error || msg.level == LogLevel::Critical)) continue;
                if (!m_ShowLua && msg.level == LogLevel::Lua) continue;

                if (!searchFilter.empty() && msg.text.find(searchFilter) == std::string::npos)
                    continue;

                ImVec4 color;
                switch (msg.level)
                {
                case LogLevel::Trace:
                case LogLevel::Debug:    color = ImVec4(0.50f, 0.50f, 0.50f, 1.0f); break;
                case LogLevel::Info:     color = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); break;
                case LogLevel::Warn:     color = ImVec4(1.00f, 0.80f, 0.20f, 1.0f); break;
                case LogLevel::Error:
                case LogLevel::Critical: color = ImVec4(1.00f, 0.30f, 0.30f, 1.0f); break;
                case LogLevel::Lua:      color = ImVec4(0.35f, 0.80f, 1.00f, 1.0f); break;
                }

                ImGui::PushID(static_cast<int>(i));

                if (msg.count > 1)
                {
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "[%u]", msg.count);
                    ImGui::SameLine();
                }

                if (msg.level == LogLevel::Lua)
                {
                    ImGui::TextColored(ImVec4(0.75f, 0.40f, 1.00f, 1.0f), "[LUA]");
                    ImGui::SameLine();
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(msg.text.c_str());
                ImGui::PopStyleColor();

                ImGui::PopID();
            }
        });

    if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}