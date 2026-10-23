#include "EditorFileDialog.h"
#include <imgui.h>
#include "Render/IconLibrary.h"
#include "Renderer/Texture.h"
#include "Utils/ProjectManager.h"
#include "Core/Log.h"
#include "Style.h"
#include <algorithm>
#include <system_error>

namespace rv {

static std::string FormatFileSize(uintmax_t bytes)
{
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
    return std::to_string(bytes / (1024 * 1024)) + " MB";
}

static ImTextureID ResolveFileIcon(const std::string& extension, bool isDirectory)
{
    auto& lib = IconLibrary::Get();
    if (isDirectory) return (ImTextureID)(intptr_t)lib.GetIcon(EditorIcon::Folder).GetID();
    if (extension == ".rscene") return (ImTextureID)(intptr_t)lib.GetIcon(EditorIcon::Scene).GetID();
    if (extension == ".rprefab") return (ImTextureID)(intptr_t)lib.GetIcon(EditorIcon::Prefab).GetID();
    if (extension == ".rmat" || extension == ".Material") return (ImTextureID)(intptr_t)lib.GetIcon(EditorIcon::Material).GetID();
    if (extension == ".rshader" || extension == ".glsl")
        return (ImTextureID)(intptr_t)lib.GetIcon(EditorIcon::Shader).GetID();
    if (extension == ".lua")
        return (ImTextureID)(intptr_t)lib.GetIcon(EditorIcon::Script).GetID();
    if (extension == ".rmesh" || extension == ".glb" || extension == ".fbx" || extension == ".obj")
        return (ImTextureID)(intptr_t)lib.GetIcon(EditorIcon::Mesh).GetID();
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga" || extension == ".rtex")
        return (ImTextureID)(intptr_t)lib.GetIcon(EditorIcon::Texture).GetID();
    if (extension == ".mp3" || extension == ".wav" || extension == ".ogg")
        return (ImTextureID)(intptr_t)lib.GetIcon(EditorIcon::Audio).GetID();
    return (ImTextureID)(intptr_t)lib.GetIcon(EditorIcon::Script).GetID();
}

static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

void EditorFileDialog::Open(
    const std::string& title,
    FileDialogMode mode,
    const std::vector<std::string>& filters,
    const std::filesystem::path& initialDir,
    Callback onConfirm,
    const std::string& defaultFileName
)
{
    Get().InternalOpen(title, mode, filters, initialDir, onConfirm, defaultFileName);
}

void EditorFileDialog::Close()
{
    Get().m_IsOpen = false;
    Get().m_OnConfirm = nullptr;
}

bool EditorFileDialog::IsOpen()
{
    return Get().m_IsOpen;
}

void EditorFileDialog::Draw()
{
    Get().InternalDraw();
}

void EditorFileDialog::InternalOpen(
    const std::string& title,
    FileDialogMode mode,
    const std::vector<std::string>& filters,
    const std::filesystem::path& initialDir,
    Callback onConfirm,
    const std::string& defaultFileName
)
{
    m_IsOpen = true;
    m_ShouldOpenPopup = true;
    m_Title = title;
    m_Mode = mode;
    m_Filters = filters;
    m_SelectedFilterIndex = 0;
    m_OnConfirm = onConfirm;
    m_SelectedItemIndex = -1;
    m_SearchBuffer[0] = '\0';

    if (!defaultFileName.empty())
    {
        strncpy_s(m_FileNameBuffer, defaultFileName.c_str(), sizeof(m_FileNameBuffer) - 1);
    }
    else
    {
        m_FileNameBuffer[0] = '\0';
    }

    std::filesystem::path startDir = initialDir;
    if (startDir.empty() || !std::filesystem::exists(startDir))
    {
        if (ProjectManager::IsProjectLoaded())
        {
            startDir = ProjectManager::GetAssetDirectory();
        }
        else
        {
            startDir = std::filesystem::current_path();
        }
    }

    m_CurrentDirectory = std::filesystem::weakly_canonical(startDir);

    m_History.clear();
    m_History.push_back(m_CurrentDirectory);
    m_HistoryIndex = 0;

    RefreshFileList();
}

void EditorFileDialog::RefreshFileList()
{
    m_CachedItems.clear();
    m_SelectedItemIndex = -1;

    std::error_code ec;
    if (!std::filesystem::exists(m_CurrentDirectory, ec) || !std::filesystem::is_directory(m_CurrentDirectory, ec))
    {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory, std::filesystem::directory_options::skip_permission_denied, ec))
    {
        FileItem item;
        item.filename = entry.path().filename().string();
        item.path = entry.path();
        item.isDirectory = entry.is_directory(ec);
        item.extension = entry.path().extension().string();

        if (!item.isDirectory)
        {
            item.fileSize = entry.file_size(ec);
        }

        if (m_Mode == FileDialogMode::SelectFolder && !item.isDirectory)
        {
            continue;
        }

        if (!item.isDirectory && !m_Filters.empty())
        {
            std::string extLower = ToLower(item.extension);
            bool matched = false;
            for (const auto& f : m_Filters)
            {
                if (f == "*.*" || f == "*" || ToLower(f) == extLower)
                {
                    matched = true;
                    break;
                }
            }
            if (!matched)
            {
                continue;
            }
        }

        m_CachedItems.push_back(std::move(item));
    }

    std::sort(m_CachedItems.begin(), m_CachedItems.end(), [](const FileItem& a, const FileItem& b) {
        if (a.isDirectory != b.isDirectory)
            return a.isDirectory > b.isDirectory;
        return ToLower(a.filename) < ToLower(b.filename);
    });
}

void EditorFileDialog::NavigateTo(const std::filesystem::path& newPath)
{
    std::error_code ec;
    if (!std::filesystem::exists(newPath, ec) || !std::filesystem::is_directory(newPath, ec))
        return;

    auto canon = std::filesystem::weakly_canonical(newPath);
    if (canon == m_CurrentDirectory)
        return;

    m_CurrentDirectory = canon;

    if (m_HistoryIndex + 1 < (int)m_History.size())
    {
        m_History.erase(m_History.begin() + m_HistoryIndex + 1, m_History.end());
    }
    m_History.push_back(m_CurrentDirectory);
    m_HistoryIndex = (int)m_History.size() - 1;

    m_SelectedItemIndex = -1;
    RefreshFileList();
}

void EditorFileDialog::GoBack()
{
    if (m_HistoryIndex > 0)
    {
        m_HistoryIndex--;
        m_CurrentDirectory = m_History[m_HistoryIndex];
        m_SelectedItemIndex = -1;
        RefreshFileList();
    }
}

void EditorFileDialog::GoForward()
{
    if (m_HistoryIndex + 1 < (int)m_History.size())
    {
        m_HistoryIndex++;
        m_CurrentDirectory = m_History[m_HistoryIndex];
        m_SelectedItemIndex = -1;
        RefreshFileList();
    }
}

void EditorFileDialog::GoUp()
{
    if (m_CurrentDirectory.has_parent_path() && m_CurrentDirectory != m_CurrentDirectory.parent_path())
    {
        NavigateTo(m_CurrentDirectory.parent_path());
    }
}

void EditorFileDialog::ConfirmSelection(const std::filesystem::path& targetPath)
{
    auto callback = m_OnConfirm;
    m_IsOpen = false;
    m_OnConfirm = nullptr;
    ImGui::CloseCurrentPopup();

    if (callback)
    {
        callback(targetPath);
    }
}

void EditorFileDialog::InternalDraw()
{
    if (!m_IsOpen)
        return;

    if (m_ShouldOpenPopup)
    {
        ImGui::OpenPopup(m_Title.c_str());
        m_ShouldOpenPopup = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(820, 520), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(600, 380), ImVec2(1920, 1080));

    bool modalOpen = true;
    if (ImGui::BeginPopupModal(m_Title.c_str(), &modalOpen, ImGuiWindowFlags_NoCollapse))
    {
        if (!modalOpen)
        {
            Close();
            ImGui::EndPopup();
            return;
        }

        auto& iconLib = IconLibrary::Get();
        ImTextureID backIcon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::ArrowBack).GetID();
        ImTextureID fwdIcon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::ArrowForward).GetID();
        ImTextureID upIcon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::ArrowUp).GetID();
        const ImVec2 navBtnSize(22.0f, 22.0f);
        const ImVec4 bgTrans(0.0f, 0.0f, 0.0f, 0.0f);
        const ImVec4 tintWhite(1.0f, 1.0f, 1.0f, 1.0f);

        bool canGoBack = (m_HistoryIndex > 0);
        if (!canGoBack) ImGui::BeginDisabled();
        if (ImGui::ImageButton("##navBack", backIcon, navBtnSize, ImVec2(0, 0), ImVec2(1, 1), bgTrans, tintWhite))
        {
            GoBack();
        }
        if (!canGoBack) ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Back");

        ImGui::SameLine();
        bool canGoFwd = (m_HistoryIndex + 1 < (int)m_History.size());
        if (!canGoFwd) ImGui::BeginDisabled();
        if (ImGui::ImageButton("##navFwd", fwdIcon, navBtnSize, ImVec2(0, 0), ImVec2(1, 1), bgTrans, tintWhite))
        {
            GoForward();
        }
        if (!canGoFwd) ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Forward");

        ImGui::SameLine();
        bool canGoUp = (m_CurrentDirectory.has_parent_path() && m_CurrentDirectory != m_CurrentDirectory.parent_path());
        if (!canGoUp) ImGui::BeginDisabled();
        if (ImGui::ImageButton("##navUp", upIcon, navBtnSize, ImVec2(0, 0), ImVec2(1, 1), bgTrans, tintWhite))
        {
            GoUp();
        }
        if (!canGoUp) ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Up one folder");

        ImGui::SameLine();
        if (ImGui::Button("Refresh", ImVec2(0, 24)))
        {
            RefreshFileList();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // Breadcrumb buttons
        std::vector<std::pair<std::string, std::filesystem::path>> breadcrumbs;
        if (ProjectManager::IsProjectLoaded())
        {
            auto projectRoot = std::filesystem::weakly_canonical(ProjectManager::GetProjectPath());
            std::error_code ec;
            auto rel = std::filesystem::relative(m_CurrentDirectory, projectRoot, ec);
            if (!ec && !rel.empty() && rel != ".")
            {
                breadcrumbs.push_back({ projectRoot.filename().string(), projectRoot });
                std::filesystem::path accum = projectRoot;
                for (const auto& part : rel)
                {
                    accum /= part;
                    breadcrumbs.push_back({ part.string(), accum });
                }
            }
        }

        if (breadcrumbs.empty())
        {
            std::filesystem::path accum;
            for (const auto& part : m_CurrentDirectory)
            {
                accum /= part;
                std::string s = part.string();
                if (s.empty() || s == "\\" || s == "/") continue;
                breadcrumbs.push_back({ s, accum });
            }
        }

        float searchBoxWidth = 180.0f;
        float breadcrumbAvail = ImGui::GetContentRegionAvail().x - searchBoxWidth - 10.0f;
        ImGui::BeginChild("##BreadcrumbsScroll", ImVec2(breadcrumbAvail, 26), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        for (size_t i = 0; i < breadcrumbs.size(); ++i)
        {
            if (i > 0)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("/");
                ImGui::SameLine();
            }

            bool isLast = (i == breadcrumbs.size() - 1);
            if (isLast)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.95f, 1.0f, 1.0f));
            }
            std::string bId = breadcrumbs[i].first + "##crumb_" + std::to_string(i);
            if (ImGui::SmallButton(bId.c_str()))
            {
                if (!isLast)
                    NavigateTo(breadcrumbs[i].second);
            }
            if (isLast)
            {
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(searchBoxWidth);
        ImGui::InputTextWithHint("##SearchInput", "Search...", m_SearchBuffer, sizeof(m_SearchBuffer));

        ImGui::Separator();

        const float footerHeight = 76.0f;
        const float sidebarWidth = 150.0f;

        ImGui::BeginChild("##QuickAccessSidebar", ImVec2(sidebarWidth, -footerHeight), true);
        ImGui::TextColored(GetAccentColor(), "Quick Access");
        ImGui::Separator();
        ImGui::Spacing();

        if (ProjectManager::IsProjectLoaded())
        {
            auto assetDir = ProjectManager::GetAssetDirectory();
            auto projectRoot = ProjectManager::GetProjectPath();

            if (ImGui::Selectable(" Assets", m_CurrentDirectory == assetDir))
            {
                NavigateTo(assetDir);
            }

            auto scenesDir = assetDir / "Scenes";
            if (std::filesystem::exists(scenesDir))
            {
                if (ImGui::Selectable(" Scenes", m_CurrentDirectory == scenesDir))
                {
                    NavigateTo(scenesDir);
                }
            }

            auto matsDir = assetDir / "Materials";
            if (std::filesystem::exists(matsDir))
            {
                if (ImGui::Selectable(" Materials", m_CurrentDirectory == matsDir))
                {
                    NavigateTo(matsDir);
                }
            }

            auto scriptsDir = assetDir / "Scripts";
            if (std::filesystem::exists(scriptsDir))
            {
                if (ImGui::Selectable(" Scripts", m_CurrentDirectory == scriptsDir))
                {
                    NavigateTo(scriptsDir);
                }
            }

            auto prefabsDir = assetDir / "Prefabs";
            if (std::filesystem::exists(prefabsDir))
            {
                if (ImGui::Selectable(" Prefabs", m_CurrentDirectory == prefabsDir))
                {
                    NavigateTo(prefabsDir);
                }
            }

            ImGui::Separator();
            if (ImGui::Selectable(" Project Root", m_CurrentDirectory == projectRoot))
            {
                NavigateTo(projectRoot);
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##FileBrowserList", ImVec2(0, -footerHeight), true);

        std::string searchFilter = ToLower(m_SearchBuffer);

        if (ImGui::BeginTable("##FileTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < m_CachedItems.size(); ++i)
            {
                const auto& item = m_CachedItems[i];

                if (!searchFilter.empty() && ToLower(item.filename).find(searchFilter) == std::string::npos)
                {
                    continue;
                }

                ImGui::TableNextRow();

                bool isSelected = (m_SelectedItemIndex == (int)i);

                ImGui::TableSetColumnIndex(0);
                ImTextureID icon = ResolveFileIcon(item.extension, item.isDirectory);
                ImGui::Image(icon, ImVec2(18.0f, 18.0f));
                ImGui::SameLine();

                std::string selLabel = "##item_" + std::to_string(i);
                if (ImGui::Selectable(selLabel.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 18.0f)))
                {
                    m_SelectedItemIndex = (int)i;
                    if (!item.isDirectory)
                    {
                        strncpy_s(m_FileNameBuffer, item.filename.c_str(), sizeof(m_FileNameBuffer) - 1);
                    }

                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        if (item.isDirectory)
                        {
                            NavigateTo(item.path);
                            break;
                        }
                        else
                        {
                            if (m_Mode == FileDialogMode::OpenFile)
                            {
                                ConfirmSelection(item.path);
                                break;
                            }
                            else if (m_Mode == FileDialogMode::SaveFile)
                            {
                                strncpy_s(m_FileNameBuffer, item.filename.c_str(), sizeof(m_FileNameBuffer) - 1);
                                ConfirmSelection(item.path);
                                break;
                            }
                        }
                    }
                }

                ImGui::SameLine();
                ImGui::TextUnformatted(item.filename.c_str());

                ImGui::TableSetColumnIndex(1);
                if (item.isDirectory)
                {
                    ImGui::TextDisabled("Folder");
                }
                else
                {
                    if (item.extension == ".rscene") ImGui::Text("Scene");
                    else if (item.extension == ".rmat") ImGui::Text("Material");
                    else if (item.extension == ".rprefab") ImGui::Text("Prefab");
                    else if (!item.extension.empty()) ImGui::Text("%s", item.extension.c_str());
                    else ImGui::TextDisabled("File");
                }

                ImGui::TableSetColumnIndex(2);
                if (item.isDirectory)
                {
                    ImGui::TextDisabled("-");
                }
                else
                {
                    ImGui::TextDisabled("%s", FormatFileSize(item.fileSize).c_str());
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Spacing();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("File name:");
        ImGui::SameLine();

        float filterComboWidth = 160.0f;
        float inputWidth = ImGui::GetContentRegionAvail().x - filterComboWidth - 10.0f;
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputText("##FileNameBottomInput", m_FileNameBuffer, sizeof(m_FileNameBuffer));

        ImGui::SameLine();
        ImGui::SetNextItemWidth(filterComboWidth);
        std::string filterLabel = "All Files (*.*)";
        if (!m_Filters.empty())
        {
            filterLabel = "";
            for (size_t f = 0; f < m_Filters.size(); ++f)
            {
                if (f > 0) filterLabel += ", ";
                filterLabel += "*" + m_Filters[f];
            }
        }
        if (ImGui::BeginCombo("##FilterCombo", filterLabel.c_str()))
        {
            ImGui::Selectable(filterLabel.c_str(), true);
            ImGui::EndCombo();
        }

        // Action Buttons Row
        ImGui::Spacing();
        float buttonWidth = 100.0f;
        float spacing = 10.0f;
        float rightAlignX = ImGui::GetContentRegionAvail().x - (buttonWidth * 2.0f + spacing);
        if (rightAlignX > 0)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + rightAlignX);
        }

        const char* confirmBtnText = (m_Mode == FileDialogMode::SaveFile) ? "Save" :
                                    (m_Mode == FileDialogMode::SelectFolder) ? "Select Folder" : "Open";

        bool canConfirm = false;
        std::filesystem::path candidatePath;
        if (m_Mode == FileDialogMode::SelectFolder)
        {
            candidatePath = m_CurrentDirectory;
            canConfirm = true;
        }
        else if (m_Mode == FileDialogMode::OpenFile)
        {
            if (m_FileNameBuffer[0] != '\0')
            {
                candidatePath = m_CurrentDirectory / m_FileNameBuffer;
                canConfirm = std::filesystem::exists(candidatePath);
            }
        }
        else if (m_Mode == FileDialogMode::SaveFile)
        {
            if (m_FileNameBuffer[0] != '\0')
            {
                std::string fname = m_FileNameBuffer;
                if (!m_Filters.empty() && !m_Filters[0].empty() && m_Filters[0] != "*.*")
                {
                    std::string expectedExt = m_Filters[0];
                    if (!fname.ends_with(expectedExt))
                    {
                        fname += expectedExt;
                    }
                }
                candidatePath = m_CurrentDirectory / fname;
                canConfirm = true;
            }
        }

        if (!canConfirm) ImGui::BeginDisabled();
        ImGui::PushStyleColor(ImGuiCol_Button, GetAccentColor());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GetAccentHoverColor());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, GetAccentActiveColor());
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        if (ImGui::Button(confirmBtnText, ImVec2(buttonWidth, 26.0f)))
        {
            ConfirmSelection(candidatePath);
        }

        ImGui::PopStyleColor(4);
        if (!canConfirm) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 26.0f)))
        {
            Close();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

}
