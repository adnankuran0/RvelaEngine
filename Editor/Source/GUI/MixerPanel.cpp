#include "MixerPanel.h"
#include "ImGui/imgui.h"
#include "glm/glm.hpp"
#include "Audio/AudioManager.h"
#include "Audio/AudioBus.h"

namespace rv {

    void MixerPanel::Draw()
    {
        ImGui::Begin("Audio Mixer");

        if (ImGui::Button(" + Add Bus "))
        {
            AudioManager::Get().CreateBus();
        }

        ImGui::Separator();

        ImGui::BeginChild("MixerStrips", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        auto& busses = AudioManager::Get().GetBusses();
        uint32_t busToDelete = 0;

        for (const auto& bus : busses)
        {
            uint32_t busID = bus->GetID();

            bool isMaster = (bus->GetName() == "Master");

            ImGui::PushID(busID);
            ImGui::BeginGroup();

            const float stripWidth = 70.0f;

            if (m_RenamingBusID == busID)
            {
                ImGui::SetNextItemWidth(stripWidth);
                ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    if (strlen(m_RenameBuffer) > 0)
                        bus->SetName(m_RenameBuffer);
                    m_RenamingBusID = 0;
                }
                if (ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    m_RenamingBusID = 0;
                }
            }
            else
            {
                std::string name = bus->GetName();
                std::string displayName = name.length() > 9 ? name.substr(0, 8) + "." : name;

                float textWidth = ImGui::CalcTextSize(displayName.c_str()).x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (stripWidth - textWidth) * 0.5f);

                if (isMaster)
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", displayName.c_str());
                else
                    ImGui::Text("%s", displayName.c_str());

                if (!isMaster && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    m_RenamingBusID = busID;
                    strncpy_s(m_RenameBuffer, sizeof(m_RenameBuffer), name.c_str(), _TRUNCATE);
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

            if (m_BusVolumes.find(busID) == m_BusVolumes.end())
            {
                m_BusVolumes[busID] = 1.0f;
            }
            float vol = m_BusVolumes[busID];

            const float sliderWidth = 20.0f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (stripWidth - sliderWidth) * 0.5f);

            if (ImGui::VSliderFloat("##vol", ImVec2(sliderWidth, 180.0f), &vol, 0.0f, 2.0f, ""))
            {
                m_BusVolumes[busID] = vol;
                AudioManager::Get().SetBusVolume(busID, vol);
            }

            char volText[16];
            snprintf(volText, sizeof(volText), "%.2f", vol);
            float volTextWidth = ImGui::CalcTextSize(volText).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (stripWidth - volTextWidth) * 0.5f);
            ImGui::TextDisabled("%s", volText);

            ImGui::Spacing();

            if (!isMaster)
            {
                ImGui::PushItemWidth(stripWidth); 

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

                std::string displayParentName = parentName.length() > 7 ? parentName.substr(0, 6) + "." : parentName;

                if (ImGui::BeginCombo("##ParentBus", displayParentName.c_str()))
                {
                    for (const auto& pBus : busses)
                    {
                        if (pBus->GetID() == busID) continue;

                        bool isSelected = (pBus->GetID() == parentID);
                        if (ImGui::Selectable(pBus->GetName().c_str(), isSelected))
                        {
                            AudioManager::Get().SetParentBus(busID, pBus->GetID());
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
            }
            else
            {
                std::string outText = "Output";
                float outTextWidth = ImGui::CalcTextSize(outText.c_str()).x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (stripWidth - outTextWidth) * 0.5f);
                ImGui::TextDisabled("%s", outText.c_str());
            }

            ImGui::EndGroup();

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(10.0f, 0.0f));
            ImGui::SameLine();
            ImGui::PopID();
        }

        if (busToDelete != 0)
        {
            AudioManager::Get().DestroyBus(busToDelete);
            m_BusVolumes.erase(busToDelete);
            if (m_RenamingBusID == busToDelete)
                m_RenamingBusID = 0;
        }

        ImGui::EndChild();
        ImGui::End();
    }

} // namespace rv