#include "MixerPanel.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "glm/glm.hpp"
#include "Audio/AudioManager.h"
#include "Audio/AudioBus.h"
#include "GUI/Style.h"
#include "GUI/AudioEffectUI.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace rv {

void MixerPanel::Draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
    ImGui::Begin("Audio Mixer");

    float availWidth = ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleColor(ImGuiCol_Button, GetAccentColor());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetAccentHoverColor());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetAccentActiveColor());

    if (ImGui::Button("+ Add Bus", ImVec2(120.0f, 26.0f)))
    {
        AudioManager::Get().CreateBus();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::Button(m_ShowEffectsRack ? "Hide FX Rack" : "Show FX Rack", ImVec2(120.0f, 26.0f)))
    {
        m_ShowEffectsRack = !m_ShowEffectsRack;
    }

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    auto& busses = AudioManager::Get().GetBusses();

    AudioBus* selectedBus = AudioManager::Get().GetBus(m_SelectedBusID);
    if (!selectedBus && !busses.empty())
    {
        m_SelectedBusID = busses[0]->GetID();
        selectedBus = busses[0].get();
    }

    if (selectedBus)
    {
        ImGui::TextDisabled("| Selected Bus: %s", selectedBus->GetName().c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    availWidth = ImGui::GetContentRegionAvail().x;
    float availableHeight = ImGui::GetContentRegionAvail().y;

    float rackWidth = 0.0f;
    float stripsWidth = availWidth;

    if (m_ShowEffectsRack && availWidth > 450.0f)
    {
        rackWidth = std::clamp(availWidth * 0.35f, 280.0f, 400.0f);
        stripsWidth = availWidth - rackWidth - 10.0f;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.12f, 0.6f));
    ImGui::BeginChild("MixerStrips", ImVec2(stripsWidth, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PopStyleColor();

    uint32_t busToDelete = 0;

    const float stripWidth = 92.0f;
    const float faderWidth = 24.0f;

    float faderHeight = std::clamp(availableHeight - 175.0f, 100.0f, 360.0f);

    for (size_t i = 0; i < busses.size(); ++i)
    {
        const auto& bus = busses[i];
        uint32_t busID = bus->GetID();
        bool isMaster = (busID == 0);
        bool isSelected = (busID == m_SelectedBusID);

        ImGui::PushID(busID);

        ImVec4 cardBg = isMaster ? ImVec4(0.17f, 0.17f, 0.25f, 0.95f) : ImVec4(0.13f, 0.13f, 0.18f, 0.90f);
        if (isSelected)
            cardBg = isMaster ? ImVec4(0.20f, 0.20f, 0.32f, 1.0f) : ImVec4(0.16f, 0.16f, 0.24f, 1.0f);

        ImVec4 cardBorder = isSelected ? GetAccentColor() : (isMaster ? ImVec4(0.40f, 0.38f, 0.65f, 0.8f) : ImVec4(0.25f, 0.25f, 0.35f, 0.35f));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, cardBg);
        ImGui::PushStyleColor(ImGuiCol_Border, cardBorder);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, isSelected ? 2.0f : 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 8.0f));

        std::string stripChildId = "Strip_" + std::to_string(busID);
        ImGui::BeginChild(stripChildId.c_str(), ImVec2(stripWidth, availableHeight - 10.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_SelectedBusID = busID;
        }

        if (m_RenamingBusID == busID)
        {
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                if (std::strlen(m_RenameBuffer) > 0)
                    bus->SetName(m_RenameBuffer);
                m_RenamingBusID = UINT32_MAX;
            }
            if (ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                m_RenamingBusID = UINT32_MAX;
            }
        }
        else
        {
            std::string name = bus->GetName();
            float textWidth = ImGui::CalcTextSize(name.c_str()).x;
            float contentWidth = ImGui::GetContentRegionAvail().x;

            if (textWidth < contentWidth)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (contentWidth - textWidth) * 0.5f);

            if (isMaster)
                ImGui::TextColored(ImVec4(0.72f, 0.65f, 0.95f, 1.0f), "%s", name.c_str());
            else
                ImGui::TextUnformatted(name.c_str());

            if (ImGui::IsItemHovered())
            {
                if (!isMaster && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    m_RenamingBusID = busID;
                    strncpy_s(m_RenameBuffer, sizeof(m_RenameBuffer), name.c_str(), _TRUNCATE);
                }
            }
        }

        if (!isMaster && ImGui::BeginPopupContextItem("BusContextMenu"))
        {
            if (ImGui::MenuItem("Rename"))
            {
                m_RenamingBusID = busID;
                strncpy_s(m_RenameBuffer, sizeof(m_RenameBuffer), bus->GetName().c_str(), _TRUNCATE);
            }
            if (ImGui::MenuItem("Delete"))
            {
                busToDelete = busID;
            }
            ImGui::EndPopup();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float vol = AudioManager::Get().GetBusVolume(busID);

        float curContentWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (curContentWidth - faderWidth) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.09f, 0.09f, 0.13f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.12f, 0.12f, 0.17f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.12f, 0.17f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, GetAccentColor());
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, GetAccentActiveColor());
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

        if (ImGui::VSliderFloat("##vol", ImVec2(faderWidth, faderHeight), &vol, 0.0f, 2.0f, ""))
        {
            AudioManager::Get().SetBusVolume(busID, vol);
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            vol = 1.0f;
            AudioManager::Get().SetBusVolume(busID, vol);
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);

        ImGui::Spacing();

        char volText[16];
        std::snprintf(volText, sizeof(volText), "%.2f", vol);
        float volTextWidth = ImGui::CalcTextSize(volText).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (curContentWidth - volTextWidth) * 0.5f);

        if (vol > 1.0f)
            ImGui::TextColored(ImVec4(0.95f, 0.40f, 0.40f, 1.0f), "%s", volText);
        else if (vol == 0.0f)
            ImGui::TextDisabled("MUTE");
        else
            ImGui::TextDisabled("%s", volText);

        ImGui::Spacing();

        // FX Button
        size_t fxCount = bus->GetEffects().size();
        char fxBtnText[32];
        if (fxCount > 0)
            std::snprintf(fxBtnText, sizeof(fxBtnText), "FX (%zu)", fxCount);
        else
            std::snprintf(fxBtnText, sizeof(fxBtnText), "+ FX");

        if (fxCount > 0 || isSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? GetAccentColor() : ImVec4(0.24f, 0.26f, 0.38f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetAccentHoverColor());
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetAccentActiveColor());
        }

        if (ImGui::Button(fxBtnText, ImVec2(-FLT_MIN, 22.0f)))
        {
            m_SelectedBusID = busID;
            m_ShowEffectsRack = true;
        }

        if (fxCount > 0 || isSelected)
        {
            ImGui::PopStyleColor(3);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (!isMaster)
        {
            uint32_t parentID = bus->GetParentBusID();
            std::string parentName = "Master";

            for (const auto& pBus : busses)
            {
                if (pBus->GetID() == parentID)
                {
                    parentName = pBus->GetName();
                    break;
                }
            }

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##ParentBus", parentName.c_str(), ImGuiComboFlags_None))
            {
                for (const auto& pBus : busses)
                {
                    if (pBus->GetID() == busID) continue;

                    bool isSelectedParent = (pBus->GetID() == parentID);
                    if (ImGui::Selectable(pBus->GetName().c_str(), isSelectedParent))
                    {
                        AudioManager::Get().SetParentBus(busID, pBus->GetID());
                    }

                    if (isSelectedParent)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Routing to: %s", parentName.c_str());
        }
        else
        {
            const char* outText = "OUTPUT";
            float outTextWidth = ImGui::CalcTextSize(outText).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (curContentWidth - outTextWidth) * 0.5f);
            ImGui::TextColored(ImVec4(0.55f, 0.57f, 0.70f, 0.8f), "%s", outText);
        }

        ImGui::EndChild();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);

        ImGui::PopID();

        if (i + 1 < busses.size())
        {
            ImGui::SameLine(0, 8.0f);
        }
    }

    if (busToDelete != 0)
    {
        AudioManager::Get().DestroyBus(busToDelete);
        if (m_RenamingBusID == busToDelete)
            m_RenamingBusID = UINT32_MAX;
        if (m_SelectedBusID == busToDelete)
            m_SelectedBusID = 0;
    }

    ImGui::EndChild();

    // Right Side: Effects Rack Panel
    if (m_ShowEffectsRack && rackWidth > 0.0f)
    {
        ImGui::SameLine(0, 10.0f);
        DrawEffectsRack(selectedBus);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void MixerPanel::DrawEffectsRack(AudioBus* bus)
{
    if (!bus) return;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.15f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, GetAccentColor());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));

    ImGui::BeginChild("EffectsRackPanel", ImVec2(0.0f, 0.0f), true);

    ImGui::TextColored(GetAccentColor(), "%s - Effects Rack", bus->GetName().c_str());
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
    if (ImGui::Button("X", ImVec2(20.0f, 20.0f)))
    {
        m_ShowEffectsRack = false;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    std::string rackId = "BusRack_" + std::to_string(bus->GetID());
    UI::DrawAudioEffectsList(
        rackId,
        bus->GetEffects(),
        [bus](std::shared_ptr<AudioEffect> fx) {
            bus->AddEffect(fx);
        },
        [bus](size_t index) {
            bus->RemoveEffect(index);
        },
        [bus](size_t fromIndex, size_t toIndex) {
            bus->MoveEffect(fromIndex, toIndex);
        }
    );

    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

} // namespace rv
