#include "AssetBrowserPanel.h"
#include "ImGui/imgui.h"
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include "Asset/Types/PrefabAsset.h"
#include "EditorUtils.h"
#include "Core/Engine.h"
#include "Scene/Entity.h"

using namespace rv;

struct DirNode
{
    std::string name;
    std::filesystem::path path;
    std::vector<DirNode> children;
};

static std::filesystem::path s_CurrentDirectory;
static char s_SearchBuffer[256] = "";
static std::vector<std::filesystem::directory_entry> s_SearchResults;
static std::vector<std::filesystem::directory_entry> s_CachedFiles;
static std::vector<DirNode> s_DirectoryTree;

static bool s_NeedsRefresh = true;
static bool s_DirectoryTreeDirty = true;

static void BuildDirectoryTreeRecursive(const std::filesystem::path& path, std::vector<DirNode>& out)
{
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_directory())
            continue;

        DirNode node;
        node.name = entry.path().filename().string();
        node.path = entry.path();

        BuildDirectoryTreeRecursive(entry.path(), node.children);
        out.emplace_back(std::move(node));
    }
}

static void RenderDirectoryRecursiveCached(const std::vector<DirNode>& nodes, std::filesystem::path& selected)
{
    for (const auto& node : nodes)
    {
        bool opened = ImGui::TreeNode(node.name.c_str());

        if (ImGui::IsItemClicked())
        {
            selected = node.path;
            s_NeedsRefresh = true;
        }

        if (opened)
        {
            RenderDirectoryRecursiveCached(node.children, selected);
            ImGui::TreePop();
        }
    }
}

static void RefreshAssets(const std::filesystem::path& directory)
{
    s_CachedFiles.clear();
    s_CachedFiles.reserve(256);

    try
    {
        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.is_regular_file() || entry.is_directory())
                s_CachedFiles.emplace_back(entry);
        }
    }
    catch (...)
    {
    }

    s_NeedsRefresh = false;
}

static void CollectSearchResults(const std::vector<std::filesystem::directory_entry>& files, std::string_view query)
{
    s_SearchResults.clear();
    s_SearchResults.reserve(files.size());

    std::string queryLower;
    queryLower.reserve(query.size());
    std::transform(query.begin(), query.end(), std::back_inserter(queryLower), ::tolower);

    for (const auto& entry : files)
    {
        if (!entry.is_regular_file())
            continue;

        auto filename = entry.path().filename().string();

        std::string filenameLower;
        filenameLower.reserve(filename.size());
        std::transform(filename.begin(), filename.end(), std::back_inserter(filenameLower), ::tolower);

        if (filenameLower.find(queryLower) != std::string::npos)
            s_SearchResults.emplace_back(entry);
    }
}

AssetBrowserPanel::AssetBrowserPanel()
{
    folderIcon.Init();
    folderIcon.GenerateFromImage("C:\\RvelaEngine\\Resources\\Editor\\Icons\\folder.png");
    materialIcon.Init();
    materialIcon.GenerateFromImage("C:\\RvelaEngine\\Resources\\Editor\\Icons\\material.png");
    sceneIcon.Init();
    sceneIcon.GenerateFromImage("C:\\RvelaEngine\\Resources\\Editor\\Icons\\scene.png");
    scriptIcon.Init();
    scriptIcon.GenerateFromImage("C:\\RvelaEngine\\Resources\\Editor\\Icons\\script.png");
    textureIcon.Init();
    textureIcon.GenerateFromImage("C:\\RvelaEngine\\Resources\\Editor\\Icons\\texture.png");
    prefabIcon.Init();
    prefabIcon.GenerateFromImage("C:\\RvelaEngine\\Resources\\Editor\\Icons\\prefab.png");
    meshIcon.Init();
    meshIcon.GenerateFromImage("C:\\RvelaEngine\\Resources\\Editor\\Icons\\mesh.png");
}

void AssetBrowserPanel::Draw(Engine* engine, const std::filesystem::path& rootDirectory)
{
    if (s_CurrentDirectory.empty())
    {
        s_CurrentDirectory = rootDirectory;
        s_NeedsRefresh = true;
        s_DirectoryTreeDirty = true;
    }

    if (s_DirectoryTreeDirty)
    {
        s_DirectoryTree.clear();
        BuildDirectoryTreeRecursive(rootDirectory, s_DirectoryTree);
        s_DirectoryTreeDirty = false;
    }

    ImGui::Begin("Asset Browser", nullptr,
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoCollapse);

    constexpr float leftPanelWidth = 200.0f;
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, leftPanelWidth);

    ImGui::BeginChild("Folders", ImVec2(0, 0), true);

    auto rootName = rootDirectory.filename().string();
    if (ImGui::Selectable(rootName.c_str(), s_CurrentDirectory == rootDirectory))
    {
        s_CurrentDirectory = rootDirectory;
        s_NeedsRefresh = true;
    }

    RenderDirectoryRecursiveCached(s_DirectoryTree, s_CurrentDirectory);

    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::BeginChild("Files", ImVec2(0, 0), true);

    bool isAtRoot = s_CurrentDirectory == rootDirectory;

    if (isAtRoot)
        ImGui::BeginDisabled();

    if (ImGui::Button(" ^ "))
    {
        s_CurrentDirectory = s_CurrentDirectory.parent_path();
        s_NeedsRefresh = true;
    }

    if (isAtRoot)
        ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::PushItemWidth(-110.0f);
    bool searchChanged = ImGui::InputTextWithHint("##Search", "Search assets...", s_SearchBuffer, sizeof(s_SearchBuffer));
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Refresh", ImVec2(70.0f, 0)))
    {
        s_NeedsRefresh = true;
        s_DirectoryTreeDirty = true;
    }

    if (s_NeedsRefresh)
        RefreshAssets(s_CurrentDirectory);

    if (searchChanged && s_SearchBuffer[0] != '\0')
        CollectSearchResults(s_CachedFiles, s_SearchBuffer);

    ImGui::Separator();

    constexpr float thumbnailSize = 50.0f;
    constexpr float padding = 16.0f;
    constexpr float cellSize = thumbnailSize + padding;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    const auto& filesToShow = (s_SearchBuffer[0] != '\0') ? s_SearchResults : s_CachedFiles;

    for (const auto& entry : filesToShow)
    {
        ImTextureID icon;
        auto extension = entry.path().extension();

        if (extension == ".rscene") icon = sceneIcon.GetID();
        else if (extension == ".rprefab") icon = prefabIcon.GetID();
        else if (entry.is_directory()) icon = folderIcon.GetID();
        else if (extension == ".rtex" || extension == ".png" || extension == ".jpeg" || extension == ".jpg" || extension == ".tga") icon = textureIcon.GetID();
        else if (extension == ".rmat") icon = materialIcon.GetID();
        else if (extension == ".rmesh") icon = meshIcon.GetID();
        else icon = scriptIcon.GetID();

        auto filename = entry.path().filename().string();

        ImGui::ImageButton(filename.c_str(), icon, ImVec2(thumbnailSize, thumbnailSize));

        if (ImGui::BeginDragDropSource())
        {
            auto pathStr = entry.path().string();
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
            else if (extension == ".rscene")
            {
                engine->GetSceneManager().LoadScene(entry.path().string());
            }
            else if (extension == ".rprefab")
            {
                auto pathStr = entry.path().string();
                AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(entry.path().string());
                if (uuid.IsValid())
                    engine->GetActiveScene().Instantiate(uuid);
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

        ImGui::TextWrapped(filename.c_str());
        ImGui::NextColumn();
    }

    ImGui::Columns(1);
    ImGui::EndChild();
    ImGui::End();
}

