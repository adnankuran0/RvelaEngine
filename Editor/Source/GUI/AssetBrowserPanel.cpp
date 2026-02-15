#include "AssetBrowserPanel.h"
#include "ImGui/imgui.h"
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <Assets/PrefabAsset.h>
#include "Core/Engine.h"
#include "Scene/Entity.h"

namespace rv {

static std::filesystem::path s_CurrentDirectory;
static char s_SearchBuffer[256] = "";
static std::vector<std::filesystem::directory_entry> s_SearchResults;
static std::vector<std::filesystem::directory_entry> s_CachedFiles; // Cached file list
static bool s_NeedsRefresh = true; // Flag to indicate when refresh is needed

void RenderDirectoryRecursive(const std::filesystem::path& path, const std::filesystem::path& root, std::filesystem::path& selected)
{
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_directory())
            continue;

        // Avoid string copy - use c_str() directly
        const auto& filename = entry.path().filename().string();
        bool opened = ImGui::TreeNode(filename.c_str());

        if (ImGui::IsItemClicked())
        {
            selected = entry.path();
            s_NeedsRefresh = true; // Mark for refresh when directory changes
        }

        if (opened)
        {
            RenderDirectoryRecursive(entry.path(), root, selected);
            ImGui::TreePop();
        }
    }
}

void RefreshAssets(const std::filesystem::path& directory)
{
    s_CachedFiles.clear();
    s_CachedFiles.reserve(256); // Reserve space to avoid reallocations

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.is_regular_file() || entry.is_directory())
                s_CachedFiles.push_back(entry);
        }
    }
    catch (const std::filesystem::filesystem_error& ex)
    {
        // Handle directory access errors gracefully
    }

    s_NeedsRefresh = false;
}

void CollectSearchResults(const std::vector<std::filesystem::directory_entry>& files, std::string_view query)
{
    s_SearchResults.clear();
    s_SearchResults.reserve(files.size()); // Reserve space

    // Convert query to lowercase once
    std::string queryLower;
    queryLower.reserve(query.size());
    std::transform(query.begin(), query.end(), std::back_inserter(queryLower), ::tolower);

    for (const auto& entry : files)
    {
        if (!entry.is_regular_file())
            continue;

        const auto& filename = entry.path().filename();

        // Use string_view and avoid temporary string creation
        std::string filenameLower;
        filenameLower.reserve(filename.string().size());
        const std::string& filenameStr = filename.string();
        std::transform(filenameStr.begin(), filenameStr.end(), std::back_inserter(filenameLower), ::tolower);

        if (filenameLower.find(queryLower) != std::string::npos)
            s_SearchResults.push_back(entry);
    }
}

void AssetBrowserPanel::Draw(Engine* engine, const std::filesystem::path& rootDirectory)
{
    if (s_CurrentDirectory.empty())
    {
        s_CurrentDirectory = rootDirectory;
        s_NeedsRefresh = true;
    }

    ImGui::Begin("Asset Browser", nullptr,
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoCollapse);

    constexpr float leftPanelWidth = 200.0f;
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, leftPanelWidth);

    ImGui::BeginChild("Folders", ImVec2(0, 0), true);

    const auto& rootFilename = rootDirectory.filename().string();
    if (ImGui::Selectable(rootFilename.c_str(), s_CurrentDirectory == rootDirectory))
    {
        s_CurrentDirectory = rootDirectory;
        s_NeedsRefresh = true;
    }

    std::filesystem::path previousDirectory = s_CurrentDirectory;
    RenderDirectoryRecursive(rootDirectory, rootDirectory, s_CurrentDirectory);

    // Check if directory changed
    if (s_CurrentDirectory != previousDirectory)
        s_NeedsRefresh = true;

    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::BeginChild("Files", ImVec2(0, 0), true);

    // Search input and refresh button on the same line
    ImGui::PushItemWidth(-80.0f); // Leave space for refresh button
    bool searchChanged = ImGui::InputTextWithHint("##Search", "Search assets...", s_SearchBuffer, sizeof(s_SearchBuffer));
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Refresh", ImVec2(70.0f, 0)))
    {
        s_NeedsRefresh = true;
    }

    // Refresh assets if needed
    if (s_NeedsRefresh)
    {
        RefreshAssets(s_CurrentDirectory);
    }

    // Update search results if search text changed
    if (searchChanged && s_SearchBuffer[0] != '\0')
    {
        CollectSearchResults(s_CachedFiles, s_SearchBuffer);
    }

    ImGui::Separator();

    constexpr float thumbnailSize = 50.0f;
    constexpr float padding = 16.0f;
    constexpr float cellSize = thumbnailSize + padding;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    // Use search results or cached files
    const auto& filesToShow = (s_SearchBuffer[0] != '\0') ? s_SearchResults : s_CachedFiles;

    for (const auto& entry : filesToShow)
    {
        const auto& filename = entry.path().filename().string();

        ImGui::Button(filename.c_str(), ImVec2(thumbnailSize, thumbnailSize));

        if (ImGui::BeginDragDropSource())
        {
            const std::string& pathStr = entry.path().string();
            ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
            ImGui::Text("Dragging %s", filename.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (entry.is_directory())
            {
                s_CurrentDirectory = entry.path();
                s_NeedsRefresh = true;
            }
            else
            {
                // Use string_view to avoid copying extension
                const auto& extension = entry.path().extension();

                if (extension == ".rscene")
                {
                    engine->GetSceneManager().LoadScene(entry.path().string());
                }
                else if (extension == ".rprefab")
                {
                    const std::string& pathStr = entry.path().string();
                    std::ifstream inFile(pathStr, std::ios::binary);
                    if (!inFile.is_open()) {
                        LOG_ERROR("file not opened");
                        return;
                    }
                    AssetHeader header = AssetLoader::ReadHeader(inFile, MAGIC_PREFAB);
                    std::unique_ptr<PrefabMeta> meta = AssetLoader::ReadMeta<PrefabMeta>(inFile, header);
                    engine->GetActiveScene().Instantiate(meta->uuid);
                }
                else if (extension == ".glsl")
                {
                    std::string command = "code \"" + entry.path().string() + "\"";
                    std::system(command.c_str());
                }
                else if (extension == ".png" || extension == ".jpeg" || extension == ".jpg" || extension == ".tga")
                {
                    std::string command = "\"" + entry.path().string() + "\"";
                    std::system(command.c_str());
                }
            }
        }

        ImGui::TextWrapped(filename.c_str());
        ImGui::NextColumn();
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::End();
}

}