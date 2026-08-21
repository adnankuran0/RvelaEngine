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
#include "AssetImporters/MaterialSerializer.h"
#include <AssetImporters/TextureImporter.h>
#include <AssetImporters/ModelImporter.h>

using namespace rv;

struct ReimportWindowState
{
    bool open = false;
    std::filesystem::path path;
    std::string importerID;

    TextureImportSettings textureSettings;
    ModelImportSettings modelSettings;
};

static ReimportWindowState s_ReimportState;

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

static bool s_RenamingActive = false;
static std::filesystem::path s_RenamingPath;
static char s_RenameBuffer[256] = "";

static void DrawReimportWindow(AssetImportPipeline& importPipeline)
{
    if (!s_ReimportState.open)
        return;

    ImGui::Begin("Reimport Settings", &s_ReimportState.open);

    if (s_ReimportState.importerID == "TextureImporter")
    {
        auto& s = s_ReimportState.textureSettings;

        ImGui::Checkbox("sRGB", &s.sRGB);
        ImGui::Checkbox("Generate Mips", &s.generateMips);
        ImGui::DragInt("Max Size", &s.maxSize, 1, 1, 8192);
    }
    if (s_ReimportState.importerID == "ModelImporter")
    {
        auto& s = s_ReimportState.modelSettings;

        ImGui::DragFloat("Scale", &s.scale, 0.01f, 0.001f, 100.0f);

        ImGui::Separator();

        ImGui::Checkbox("Import Normals", &s.importNormals);
        ImGui::Checkbox("Import Tangents", &s.importTangents);
        ImGui::Checkbox("Import UVs", &s.importUVs);

        ImGui::Separator();

        ImGui::Checkbox("Import Textures", &s.importTextures);
        ImGui::Checkbox("Import Materials", &s.importMaterials);
        ImGui::Checkbox("Generate Prefab", &s.generatePrefab);
    }

    if (ImGui::Button("Reimport"))
    {
        auto& registry = AssetManager::Get().GetRegistry();

        auto metaPath = std::filesystem::path(s_ReimportState.path.string() + ".rmeta");

        AssetMeta meta;
        meta.LoadFromFile(metaPath);

        json j;

        if (s_ReimportState.importerID == "TextureImporter")
        {
            auto& s = s_ReimportState.textureSettings;
            j["sRGB"] = s.sRGB;
            j["generateMips"] = s.generateMips;
            j["maxSize"] = s.maxSize;
        }

        if (s_ReimportState.importerID == "ModelImporter")
        {
            auto& s = s_ReimportState.modelSettings;
            j["scale"] = s.scale;
            j["importNormals"] = s.importNormals;
            j["importTangents"] = s.importTangents;
            j["importUVs"] = s.importUVs;
            j["importTextures"] = s.importTextures;
            j["importMaterials"] = s.importMaterials;
            j["generatePrefab"] = s.generatePrefab;
        }

        meta.importerSettingsJson = j.dump();

        registry.SaveMeta(s_ReimportState.path, meta);

        importPipeline.Reimport(s_ReimportState.path, registry);

        s_ReimportState.open = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset"))
    {
        if (s_ReimportState.importerID == "TextureImporter")
            s_ReimportState.textureSettings = TextureImportSettings();

        if (s_ReimportState.importerID == "ModelImporter")
            s_ReimportState.modelSettings = ModelImportSettings();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        s_ReimportState.open = false;
    }

    ImGui::End();
}

static void DeletePathWithMeta(const std::filesystem::path& path)
{
    std::error_code ec;

    if (std::filesystem::is_directory(path))
    {
        std::filesystem::remove_all(path, ec);
        if (ec) { LOG_ERROR("Delete failed: {}", ec.message()); return; }
    }
    else
    {
        std::filesystem::remove(path, ec);
        if (ec) { LOG_ERROR("Delete failed: {}", ec.message()); return; }

        auto metaPath = std::filesystem::path(path.string() + ".rmeta");
        if (std::filesystem::exists(metaPath))
        {
            std::filesystem::remove(metaPath, ec);
            if (ec)
                LOG_WARN("Meta delete failed: {}", ec.message());
        }

    }

    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}

static void HandleReimport(const std::filesystem::path& path, AssetImportPipeline& importPipeline)
{
    auto metaPath = std::filesystem::path(path.string() + ".rmeta");
    if (std::filesystem::exists(metaPath))
    {
        AssetMeta meta;
        meta.LoadFromFile(metaPath);

        if (meta.importerID.empty() || meta.importerSettingsJson.empty())
        {
            importPipeline.Reimport(path, AssetManager::Get().GetRegistry());
            return;
        }
            
        s_ReimportState.open = true;
        s_ReimportState.path = path;
        s_ReimportState.importerID = meta.importerID;

        if (meta.importerID == "TextureImporter")
            s_ReimportState.textureSettings = TextureImporter::ParseSettings(meta.importerSettingsJson);

        if (meta.importerID == "ModelImporter")
            s_ReimportState.modelSettings = ModelImporter::ParseSettings(meta.importerSettingsJson);
    }
}

static void CreateEmptyTextFile(const std::filesystem::path& path, const std::string& content = "")
{
    std::ofstream f(path);
    if (!f) { LOG_ERROR("Could not create file: {}", path.string()); return; }
    f << content;
}

static void RenamePathWithMeta(const std::filesystem::path& oldPath, const std::string& newName)
{
    if (newName.empty()) return;

    auto newPath = oldPath.parent_path() / newName;

    std::error_code ec;
    std::filesystem::rename(oldPath, newPath, ec);
    if (ec) { LOG_ERROR("Rename failed: {}", ec.message()); return; }

    auto oldMeta = std::filesystem::path(oldPath.string() + ".rmeta");
    auto newMeta = std::filesystem::path(newPath.string() + ".rmeta");
    if (std::filesystem::exists(oldMeta))
    {
        std::filesystem::rename(oldMeta, newMeta, ec);
        if (ec) LOG_WARN("Meta rename failed: {}", ec.message());
    }

    LOG_INFO("Renamed: {} -> {}", oldPath.filename().string(), newName);
    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}


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
            if (entry.is_regular_file() && entry.path().extension() == ".rmeta")
                continue;

            if (entry.is_directory() && entry.path().filename().string().front() == '.')
                continue;

            if (entry.is_regular_file() || entry.is_directory())
                s_CachedFiles.emplace_back(entry);
        }

        std::stable_sort(s_CachedFiles.begin(), s_CachedFiles.end(),
            [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b)
            {
                return a.is_directory() > b.is_directory();
            });
    }
    catch (...) {}

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
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".rmeta") continue;

        auto filename = entry.path().filename().string();
        std::string filenameLower;
        filenameLower.reserve(filename.size());
        std::transform(filename.begin(), filename.end(), std::back_inserter(filenameLower), ::tolower);

        if (filenameLower.find(queryLower) != std::string::npos)
            s_SearchResults.emplace_back(entry);
    }
}

static void DrawBackgroundContextMenu(Engine* engine, const std::filesystem::path& currentDir)
{
    if (ImGui::BeginPopup("##BrowserBgMenu"))
    {
        if (ImGui::BeginMenu("New"))
        {
            if (ImGui::MenuItem("Folder"))
            {
                auto newPath = currentDir / "New Folder";
                std::error_code ec;
                int i = 1;
                while (std::filesystem::exists(newPath))
                    newPath = currentDir / ("New Folder " + std::to_string(i++));

                std::filesystem::create_directory(newPath, ec);
                if (!ec)
                {
                    s_RenamingPath = newPath;
                    s_RenamingActive = true;
                    strncpy_s(s_RenameBuffer, sizeof(s_RenameBuffer),
                        newPath.filename().string().c_str(), _TRUNCATE);
                    s_NeedsRefresh = true;
                    s_DirectoryTreeDirty = true;
                }
                else LOG_ERROR("Create folder failed: {}", ec.message());
            }

            ImGui::Separator();

            auto& assetReg = AssetManager::Get().GetRegistry();

            if (ImGui::MenuItem("Material"))
            {
                auto newPath = currentDir / "NewMaterial.rmat";
                int i = 1;
                while (std::filesystem::exists(newPath))
                    newPath = currentDir / ("NewMaterial" + std::to_string(i++) + ".rmat");
                MaterialSerializer::CreateNew(newPath, assetReg);
                assetReg.Scan(assetReg.GetAssetDir());
                s_NeedsRefresh = true;
            }

            if (ImGui::MenuItem("Scene"))
            {
                auto newPath = currentDir / "NewScene.rscene";
                int i = 1;
                while (std::filesystem::exists(newPath))
                    newPath = currentDir / ("NewScene" + std::to_string(i++) + ".rscene");
                auto scene = engine->GetSceneManager().CreateScene("NewScene");
                engine->GetSceneManager().SaveScene(*scene,newPath.string());
                assetReg.Scan(assetReg.GetAssetDir());
                s_NeedsRefresh = true;
                
            }

            if (ImGui::MenuItem("Lua Script"))
            {
                auto newPath = currentDir / "NewScript.lua";
                int i = 1;
                while (std::filesystem::exists(newPath))
                    newPath = currentDir / ("NewScript" + std::to_string(i++) + ".lua");
                CreateEmptyTextFile(newPath, "-- New Script\n");
                s_NeedsRefresh = true;

            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Refresh"))
        {
            s_NeedsRefresh = true;
            s_DirectoryTreeDirty = true;
        }

        if (ImGui::MenuItem("Open in Explorer"))
        {
            std::string cmd = "explorer \"" + currentDir.string() + "\"";
            std::system(cmd.c_str());
        }

        ImGui::EndPopup();
    }
}

static void DrawItemContextMenu(const std::filesystem::path& itemPath, AssetImportPipeline& importPipeline)
{
    std::string popupId = "##ItemMenu_" + itemPath.filename().string();

    if (ImGui::BeginPopup(popupId.c_str()))
    {
        ImGui::TextDisabled("%s", itemPath.filename().string().c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Rename"))
        {
            s_RenamingPath = itemPath;
            s_RenamingActive = true;
            strncpy_s(s_RenameBuffer, sizeof(s_RenameBuffer),
                itemPath.filename().string().c_str(), _TRUNCATE);
        }

        if (ImGui::MenuItem("Delete"))
        {
            DeletePathWithMeta(itemPath);
        }

        

        ImGui::Separator();

        if (ImGui::MenuItem("Open in Explorer"))
        {
            auto dir = std::filesystem::is_directory(itemPath)
                ? itemPath
                : itemPath.parent_path();
            std::string cmd = "explorer \"" + dir.string() + "\"";
            std::system(cmd.c_str());
        }

        if (!std::filesystem::is_directory(itemPath))
        {
            auto ext = itemPath.extension();
            if (ext == ".glsl" || ext == ".lua")
            {
                if (ImGui::MenuItem("Open in VS Code"))
                {
                    std::string cmd = "code \"" + itemPath.string() + "\"";
                    std::system(cmd.c_str());
                }
            }

            if (ImGui::MenuItem("Copy Path"))
                ImGui::SetClipboardText(itemPath.string().c_str());

            if (ImGui::MenuItem("Reimport"))
                HandleReimport(itemPath,importPipeline);
        }

        ImGui::EndPopup();
    }
}


AssetBrowserPanel::AssetBrowserPanel()
{
    folderIcon.Init();
    folderIcon.GenerateFromImage(EDITOR_PATH("Icons\\folder.png").GetAbsoluteStr());
    materialIcon.Init();
    materialIcon.GenerateFromImage(EDITOR_PATH("Icons\\material.png").GetAbsoluteStr());
    sceneIcon.Init();
    sceneIcon.GenerateFromImage(EDITOR_PATH("Icons\\scene.png").GetAbsoluteStr());
    scriptIcon.Init();
    scriptIcon.GenerateFromImage(EDITOR_PATH("Icons\\script.png").GetAbsoluteStr());
    textureIcon.Init();
    textureIcon.GenerateFromImage(EDITOR_PATH("Icons\\texture.png").GetAbsoluteStr());
    prefabIcon.Init();
    prefabIcon.GenerateFromImage(EDITOR_PATH("Icons\\prefab.png").GetAbsoluteStr());
    meshIcon.Init();
    meshIcon.GenerateFromImage(EDITOR_PATH("Icons\\mesh.png").GetAbsoluteStr());
}


void AssetBrowserPanel::Draw(Engine* engine, const std::filesystem::path& rootDirectory, AssetImportPipeline& importPipeline)
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

    bool isAtRoot = (s_CurrentDirectory == rootDirectory);

    if (isAtRoot) ImGui::BeginDisabled();
    if (ImGui::Button(" ^ "))
    {
        s_CurrentDirectory = s_CurrentDirectory.parent_path();
        s_NeedsRefresh = true;
    }
    if (isAtRoot) ImGui::EndDisabled();

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
    int   columnCount = static_cast<int>(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    const auto& filesToShow = (s_SearchBuffer[0] != '\0') ? s_SearchResults : s_CachedFiles;

    for (const auto& entry : filesToShow)
    {
        auto extension = entry.path().extension();
        auto filename = entry.path().filename().string();

        if (s_RenamingActive && s_RenamingPath == entry.path())
        {
            ImGui::SetNextItemWidth(cellSize - 4.0f);
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##rename", s_RenameBuffer, sizeof(s_RenameBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll))
            {
                RenamePathWithMeta(s_RenamingPath, s_RenameBuffer);
                s_RenamingActive = false;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                s_RenamingActive = false;

            ImGui::NextColumn();
            continue;
        }

        ImTextureID icon;
        if (extension == ".rscene") icon = sceneIcon.GetID();
        else if (extension == ".rprefab") icon = prefabIcon.GetID();
        else if (entry.is_directory()) icon = folderIcon.GetID();
        else if (extension == ".rtex" ||
            extension == ".png" || extension == ".jpeg" ||
            extension == ".jpg" || extension == ".tga") icon = textureIcon.GetID();
        else if (extension == ".rmat") icon = materialIcon.GetID();
        else if (extension == ".rmesh") icon = meshIcon.GetID();
        else icon = scriptIcon.GetID();

        ImGui::ImageButton(filename.c_str(), icon, ImVec2(thumbnailSize, thumbnailSize));

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup(("##ItemMenu_" + filename).c_str());
        DrawItemContextMenu(entry.path(),importPipeline);

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
                AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(entry.path().string());
                if (uuid.IsValid())
                    engine->GetActiveScene().Instantiate(uuid);
            }
            else if (extension == ".glsl" || extension == ".lua")
            {
                std::string command = "code \"" + entry.path().string() + "\"";
                std::system(command.c_str());
            }
            else if (extension == ".png" || extension == ".jpeg" ||
                extension == ".jpg" || extension == ".tga")
            {
                std::string command = "\"" + entry.path().string() + "\"";
                std::system(command.c_str());
            }
        }

        ImGui::TextWrapped(filename.c_str());
        ImGui::NextColumn();
    }

    ImGui::Columns(1);

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        !ImGui::IsAnyItemHovered())
    {
        ImGui::OpenPopup("##BrowserBgMenu");
    }
    DrawBackgroundContextMenu(engine,s_CurrentDirectory);

    ImGui::EndChild();
    ImGui::End();

    DrawReimportWindow(importPipeline);
}

void AssetBrowserPanel::HandleFileDrop(FileDroppedEvent& event, AssetImportPipeline& importPipeline)
{
    if (s_CurrentDirectory.empty())
        return;

    std::vector<std::filesystem::path> toImport;

    for (const auto& sourcePath : event.GetPaths())
    {
        auto destPath = s_CurrentDirectory / sourcePath.filename();

        if (sourcePath == destPath)
            continue;

        std::error_code ec;

        if (std::filesystem::is_directory(sourcePath))
        {
            std::filesystem::copy(
                sourcePath, destPath,
                std::filesystem::copy_options::recursive |
                std::filesystem::copy_options::overwrite_existing,
                ec);

            if (ec) { LOG_ERROR("Folder copy failed: {}", ec.message()); continue; }

            for (const auto& entry : std::filesystem::recursive_directory_iterator(destPath, ec))
            {
                if (entry.is_regular_file())
                    toImport.push_back(entry.path());
            }
        }
        else
        {
            std::filesystem::copy_file(
                sourcePath, destPath,
                std::filesystem::copy_options::overwrite_existing,
                ec);

            if (ec) { LOG_ERROR("File copy failed: {}", ec.message()); continue; }
            toImport.push_back(destPath);
        }
    }

    for (const auto& path : toImport)
        importPipeline.ImportAsset(path, AssetManager::Get().GetRegistry());

    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}