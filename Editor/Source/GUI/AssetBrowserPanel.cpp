#include "AssetBrowserPanel.h"
#include "ImGui/imgui.h"
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <Assets/PrefabAsset.h>

static std::filesystem::path s_CurrentDirectory;
static char s_SearchBuffer[256] = "";
static std::vector<std::filesystem::directory_entry> s_SearchResults;

void RenderDirectoryRecursive(const std::filesystem::path& path, const std::filesystem::path& root, std::filesystem::path& selected)
{
    for (auto& entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_directory())
            continue;

        std::string name = entry.path().filename().string();
        bool opened = ImGui::TreeNode(name.c_str());

        if (ImGui::IsItemClicked())
            selected = entry.path();

        if (opened)
        {
            RenderDirectoryRecursive(entry.path(), root, selected);
            ImGui::TreePop();
        }
    }
}

void CollectSearchResults(const std::filesystem::path& root, const std::string& query)
{
    s_SearchResults.clear();

    for (auto& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file())
            continue;

        std::string filename = entry.path().filename().string();

        std::string filenameLower = filename;
        std::string queryLower = query;

        std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(), ::tolower);
        std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);

        if (filenameLower.find(queryLower) != std::string::npos)
            s_SearchResults.push_back(entry);
    }
}

void AssetBrowserPanel::Draw(Engine* engine, const std::filesystem::path& rootDirectory)
{
    if (s_CurrentDirectory.empty())
        s_CurrentDirectory = rootDirectory;

    ImGui::Begin("Asset Browser", nullptr,
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoCollapse);

    float leftPanelWidth = 200.0f;
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, leftPanelWidth);

    ImGui::BeginChild("Folders", ImVec2(0, 0), true);

    if (ImGui::Selectable(rootDirectory.filename().string().c_str(), s_CurrentDirectory == rootDirectory))
        s_CurrentDirectory = rootDirectory;

    RenderDirectoryRecursive(rootDirectory, rootDirectory, s_CurrentDirectory);

    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::BeginChild("Files", ImVec2(0, 0), true);

    if (ImGui::InputTextWithHint("##Search", "Search assets...", s_SearchBuffer, sizeof(s_SearchBuffer)))
    {
        if (s_SearchBuffer[0] != '\0')
            CollectSearchResults(s_CurrentDirectory, s_SearchBuffer);
    }

    ImGui::Separator();

    const float thumbnailSize = 50.0f;
    const float padding = 16.0f;
    const float cellSize = thumbnailSize + padding;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    if (s_SearchBuffer[0] != '\0')
    {
        for (auto& entry : s_SearchResults)
        {
            std::string filename = entry.path().filename().string();

            ImGui::Button(filename.c_str(), ImVec2(thumbnailSize, thumbnailSize));
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                std::string extension = entry.path().extension().string();
                if (extension == ".rscene")
                {
                    engine->GetSceneManager().LoadScene(*engine->GetScene(), entry.path().string());
                }
                if (extension == ".glsl")
                {
                    std::string command = "code \"" + entry.path().string() + "\"";
                    std::system(command.c_str());
                }
            }

            ImGui::TextWrapped(filename.c_str());
            ImGui::NextColumn();
        }
    }
    else
    {
        for (auto& entry : std::filesystem::directory_iterator(s_CurrentDirectory))
        {
            std::string filename = entry.path().filename().string();

            ImGui::Button(filename.c_str(), ImVec2(thumbnailSize, thumbnailSize));

            if (ImGui::BeginDragDropSource())
            {
                std::string pathStr = entry.path().string();
                ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
                ImGui::Text("Dragging %s", filename.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (entry.is_directory())
                    s_CurrentDirectory = entry.path();
                else
                {
                    std::string extension = entry.path().extension().string();
                    if (extension == ".rscene")
                    {
                        engine->GetSceneManager().LoadScene(*engine->GetScene(), entry.path().string());
                    }
                    if (extension == ".rprefab")
                    {
                        std::string pathStr = entry.path().string();
                        std::ifstream inFile(pathStr, std::ios::binary);
                        if (!inFile.is_open()) {
                            LOG_ERROR("file not opened");
                            return;
                        }
                        AssetHeader header = AssetLoader::ReadHeader(inFile, MAGIC_PREFAB);
                        std::unique_ptr<PrefabMeta> meta = AssetLoader::ReadMeta<PrefabMeta>(inFile, header);
                        engine->GetScene()->Instantiate(meta->uuid);
                    }
                    if (extension == ".glsl")
                    {
                        std::string command = "code \"" + entry.path().string() + "\"";
                        std::system(command.c_str());
                    }
                    if (extension == ".png" || extension == ".jpeg" || extension == ".jpg" || extension == ".tga")
                    {
                        std::string command = "\"" + entry.path().string() + "\"";
                        std::system(command.c_str());
                    }
                }
            }

            ImGui::TextWrapped(filename.c_str());
            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::End();
}