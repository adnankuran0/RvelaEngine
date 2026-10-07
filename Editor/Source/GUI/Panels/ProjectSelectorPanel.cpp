#include "ProjectSelectorPanel.h"
#include "Core/Engine.h"
#include "Utils/ProjectManager.h"
#include "GUI/Dialogs.h"
#include <imgui.h>
#include <filesystem>
#include <vector>

using namespace rv;

bool ProjectSelectorPanel::Draw(Engine* engine)
{
    if (!m_IsOpen)
        return false;

    bool projectSelected = false;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

    bool isModal = !ProjectManager::IsProjectLoaded();

    bool visible = false;

    if (isModal)
    {
        ImGui::SetNextWindowPos(vp->Pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(vp->Size, ImGuiCond_Always);
        flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 20.0f));

        ImGui::OpenPopup("Project Selector##Modal");
        visible = ImGui::BeginPopupModal("Project Selector##Modal", nullptr, flags);

        ImGui::PopStyleVar(3);
    }
    else
    {
        ImVec2 center = vp->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(750.0f, 500.0f), ImGuiCond_Appearing);
        visible = ImGui::Begin("Project Selector", &m_IsOpen, flags);
    }

    if (visible)
    {
        ImGui::TextColored(ImVec4(0.62f, 0.56f, 0.80f, 1.0f), "RvelaEngine - Project Hub");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("ProjectSelectorTabs"))
        {
            if (ImGui::BeginTabItem("Recent Projects"))
            {
                ImGui::Spacing();
                std::vector<std::string> recents = ProjectManager::GetRecentProjects();

                if (recents.empty())
                {
                    ImGui::TextDisabled("No recent projects found.");
                }
                else
                {
                    ImGui::Text("Click a project to open:");
                    ImGui::Spacing();

                    float listHeight = ImGui::GetContentRegionAvail().y - 45.0f;
                    std::string projectToRemove = "";

                    if (ImGui::BeginChild("##RecentListChild", ImVec2(-1.0f, listHeight > 100.0f ? listHeight : 100.0f), true))
                    {
                        for (size_t i = 0; i < recents.size(); ++i)
                        {
                            const auto& projPath = recents[i];
                            std::filesystem::path p(projPath);
                            std::string projName = p.stem().string();
                            bool exists = std::filesystem::exists(p);

                            ImGui::PushID(static_cast<int>(i));

                            std::string label = projName + "  (" + p.parent_path().string() + ")";
                            if (!exists)
                            {
                                label += " [Missing]";
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.4f, 0.4f, 1.0f));
                            }

                            if (ImGui::Selectable(label.c_str(), false))
                            {
                                if (!exists)
                                {
                                    m_ErrorMessage = "Project file not found on disk: " + projPath;
                                }
                                else if (engine->OpenProject(projPath))
                                {
                                    m_IsOpen = false;
                                    projectSelected = true;
                                    if (isModal) ImGui::CloseCurrentPopup();
                                }
                                else
                                {
                                    m_ErrorMessage = "Failed to open project: " + projPath;
                                }
                            }

                            if (!exists)
                            {
                                ImGui::PopStyleColor();
                            }

                            if (ImGui::BeginPopupContextItem())
                            {
                                if (ImGui::MenuItem("Remove from Recent"))
                                {
                                    projectToRemove = projPath;
                                }
                                ImGui::EndPopup();
                            }

                            ImGui::PopID();
                        }
                    }
                    ImGui::EndChild();

                    if (!projectToRemove.empty())
                    {
                        ProjectManager::RemoveRecentProject(projectToRemove);
                    }
                }

                ImGui::Spacing();
                if (ImGui::Button("Browse Other Project...", ImVec2(200.0f, 30.0f)))
                {
                    std::string chosen = Dialogs::OpenProjectDialog();
                    if (!chosen.empty())
                    {
                        std::filesystem::path p(chosen);
                        if (p.parent_path().filename().string() == "TemplateProject")
                        {
                            m_ErrorMessage = "TemplateProject is read-only template and cannot be opened directly.";
                        }
                        else if (engine->OpenProject(chosen))
                        {
                            m_IsOpen = false;
                            projectSelected = true;
                            if (isModal) ImGui::CloseCurrentPopup();
                        }
                        else
                        {
                            m_ErrorMessage = "Failed to open project: " + chosen;
                        }
                    }
                }

                ImGui::EndTabItem();
            }

            // TAB 2: Create New Project
            if (ImGui::BeginTabItem("New Project"))
            {
                ImGui::Spacing();

                ImGui::Text("Project Name:");
                ImGui::InputText("##ProjName", m_NewProjectName, sizeof(m_NewProjectName));

                ImGui::Spacing();
                ImGui::Text("Location:");
                ImGui::InputText("##ProjLocation", m_NewProjectPath, sizeof(m_NewProjectPath));
                ImGui::SameLine();
                if (ImGui::Button("Browse..."))
                {
                    std::string folder = Dialogs::SelectFolderDialog("Select Project Parent Folder");
                    if (!folder.empty())
                    {
                        strncpy_s(m_NewProjectPath, folder.c_str(), sizeof(m_NewProjectPath) - 1);
                    }
                }

                ImGui::Spacing();
                std::string fullPathPreview = "";
                if (strlen(m_NewProjectPath) > 0 && strlen(m_NewProjectName) > 0)
                {
                    fullPathPreview = (std::filesystem::path(m_NewProjectPath) / m_NewProjectName).string();
                    ImGui::TextDisabled("Project will be created at:\n%s", fullPathPreview.c_str());
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Create Project", ImVec2(160.0f, 32.0f)))
                {
                    std::string nameStr = m_NewProjectName;
                    std::string pathStr = m_NewProjectPath;

                    if (nameStr.empty())
                    {
                        m_ErrorMessage = "Project name cannot be empty.";
                    }
                    else if (pathStr.empty() || !std::filesystem::exists(pathStr))
                    {
                        m_ErrorMessage = "Please select a valid parent folder.";
                    }
                    else
                    {
                        if (ProjectManager::CreateProject(nameStr, pathStr))
                        {
                            std::filesystem::path projFile = std::filesystem::path(pathStr) / nameStr / (nameStr + ".rproj");
                            if (engine->OpenProject(projFile.string()))
                            {
                                m_IsOpen = false;
                                projectSelected = true;
                                if (isModal) ImGui::CloseCurrentPopup();
                            }
                        }
                        else
                        {
                            m_ErrorMessage = "Failed to create project.";
                        }
                    }
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        if (!m_ErrorMessage.empty())
        {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", m_ErrorMessage.c_str());
        }

        if (isModal)
        {
            ImGui::EndPopup();
        }
        else
        {
            ImGui::End();
        }
    }

    return projectSelected;
}
