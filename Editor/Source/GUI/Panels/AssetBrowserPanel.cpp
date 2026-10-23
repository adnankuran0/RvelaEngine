#include "AssetBrowserPanel.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "Core/Log.h"
#include "Asset/Types/PrefabAsset.h"
#include "EditorUtils.h"
#include "Core/Engine.h"
#include "Scene/Entity.h"
#include "EditorSelection.h"
#include "Commands/CommandHistory.h"
#include "Renderer/EditorCamera.h"
#include "AssetImporters/MaterialSerializer.h"
#include "AssetImporters/AnimationLibrarySerializer.h"
#include <AssetImporters/TextureImporter.h>
#include <AssetImporters/ModelImporter.h>
#include "Render/IconLibrary.h"
#include "Utils/ProjectManager.h"
#include "ImportSettingsPanel.h"
#include "ThumbnailCache.h"
#include "GUI/Style.h"

using namespace rv;

struct ReimportWindowState
{
    bool open = false;
    std::filesystem::path path;
};

static ReimportWindowState s_ReimportState;

struct DirNode
{
    std::string name;
    std::filesystem::path path;
    std::vector<DirNode> children;
};

struct BrowserItem
{
    std::filesystem::path path;
    std::string filename;
    std::string stem;
    std::string extension;
    bool isDirectory = false;
    bool isSubAsset = false;
    int subAssetCount = 0;
    uintmax_t fileSize = 0;
    ImTextureID icon = 0;
};

static bool IsModelFile(const std::filesystem::path& path)
{
    std::string ext = path.extension().string();
    return ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb";
}

static std::filesystem::path s_LastRootDirectory;
static std::filesystem::path s_CurrentDirectory;

static bool ArePathsEqual(const std::filesystem::path& p1, const std::filesystem::path& p2)
{
    if (p1.empty() && p2.empty()) return true;
    if (p1.empty() || p2.empty()) return false;
    std::error_code ec;
    bool eq = std::filesystem::equivalent(p1, p2, ec);
    if (!ec) return eq;
    return p1.lexically_normal() == p2.lexically_normal();
}

static bool IsSubpathOf(const std::filesystem::path& child, const std::filesystem::path& parent)
{
    if (child.empty() || parent.empty()) return false;
    std::error_code ec;
    auto rel = std::filesystem::relative(child, parent, ec);
    if (ec) return false;
    auto str = rel.generic_string();
    return str != ".." && str.rfind("../", 0) != 0;
}

static std::filesystem::path s_SelectedItemPath;
static bool s_SelectedIsDir = false;
static uintmax_t s_SelectedFileSize = 0;
static std::vector<std::filesystem::path> s_SelectedPaths;
static int s_LastSelectedIndex = -1;

static bool s_IsBoxSelecting = false;
static ImVec2 s_BoxSelectStartPos(0.0f, 0.0f);
static ImVec2 s_BoxSelectStartScroll(0.0f, 0.0f);
static ImVec2 s_BoxSelectStartWindowPos(0.0f, 0.0f);
static bool s_BoxSelectCtrl = false;
static bool s_BoxSelectShift = false;
static std::vector<std::filesystem::path> s_PreBoxSelectedPaths;
static bool s_DeselectOthersOnMouseRelease = false;
static int s_DeselectTargetIndex = -1;

static bool IsSelected(const std::filesystem::path& path)
{
    return std::find(s_SelectedPaths.begin(), s_SelectedPaths.end(), path) != s_SelectedPaths.end();
}

static void ClearSelectionInternal()
{
    s_SelectedPaths.clear();
    s_SelectedItemPath.clear();
    s_SelectedIsDir = false;
    s_SelectedFileSize = 0;
    s_LastSelectedIndex = -1;
    s_DeselectOthersOnMouseRelease = false;
    s_DeselectTargetIndex = -1;
}

const std::vector<std::filesystem::path>& AssetBrowserPanel::GetSelectedPaths()
{
    return s_SelectedPaths;
}

std::filesystem::path AssetBrowserPanel::GetSelectedPrimaryPath()
{
    if (!s_SelectedItemPath.empty())
        return s_SelectedItemPath;
    if (!s_SelectedPaths.empty())
        return s_SelectedPaths.front();
    return {};
}

void AssetBrowserPanel::ClearSelection()
{
    ClearSelectionInternal();
}

void AssetBrowserPanel::OpenImportSettings(const std::filesystem::path& path)
{
    s_ReimportState.open = true;
    s_ReimportState.path = path;
    ImportSettingsPanel::SetInspectedAsset(path);
}

static std::vector<std::filesystem::path> s_NavHistory;
static int s_NavHistoryIndex = -1;

static char s_SearchBuffer[256] = "";
static std::vector<BrowserItem> s_CachedItems;
static std::vector<BrowserItem> s_SearchItemResults;
static std::vector<DirNode> s_DirectoryTree;

static void SetSingleSelection(const std::filesystem::path& path)
{
    if (path.empty())
    {
        ClearSelectionInternal();
        return;
    }

    s_SelectedPaths.clear();
    s_SelectedPaths.push_back(path);
    s_SelectedItemPath = path;

    std::error_code ec;
    s_SelectedIsDir = std::filesystem::is_directory(path, ec);
    s_SelectedFileSize = s_SelectedIsDir ? 0 : std::filesystem::file_size(path, ec);
    s_LastSelectedIndex = -1;
    for (size_t i = 0; i < s_CachedItems.size(); ++i)
    {
        if (s_CachedItems[i].path == path)
        {
            s_LastSelectedIndex = (int)i;
            break;
        }
    }
    s_DeselectOthersOnMouseRelease = false;
    s_DeselectTargetIndex = -1;
}

static bool s_NeedsRefresh = true;
static bool s_DirectoryTreeDirty = true;
static bool s_BreadcrumbsDirty = true;
static std::vector<std::pair<std::string, std::filesystem::path>> s_CachedBreadcrumbs;
static float s_ThumbnailSize = 56.0f;

static bool s_RenamingActive = false;
static bool s_RenameJustCommitted = false;
static std::filesystem::path s_RenamingPath;
static char s_RenameBuffer[256] = "";
static std::string s_RenameExtension;

static void ResetInternal(const std::filesystem::path& rootDirectory)
{
    ThumbnailCache::Get().Reset();
    s_LastRootDirectory = rootDirectory;
    s_CurrentDirectory = rootDirectory;
    s_NavHistory.clear();
    if (!rootDirectory.empty() && std::filesystem::exists(rootDirectory))
    {
        s_NavHistory.push_back(rootDirectory);
        s_NavHistoryIndex = 0;
    }
    else
    {
        s_NavHistoryIndex = -1;
    }

    ClearSelectionInternal();
    s_SearchBuffer[0] = '\0';
    s_SearchItemResults.clear();
    s_CachedItems.clear();
    s_DirectoryTree.clear();
    s_CachedBreadcrumbs.clear();

    s_RenamingActive = false;
    s_RenameJustCommitted = false;
    s_RenamingPath.clear();
    s_RenameBuffer[0] = '\0';
    s_RenameExtension.clear();

    s_ReimportState.open = false;
    s_ReimportState.path.clear();

    s_IsBoxSelecting = false;
    s_PreBoxSelectedPaths.clear();
    s_DeselectOthersOnMouseRelease = false;
    s_DeselectTargetIndex = -1;

    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
    s_BreadcrumbsDirty = true;
}

void AssetBrowserPanel::Reset(const std::filesystem::path& rootDirectory)
{
    ResetInternal(rootDirectory);
}

static std::string FormatFileSize(uintmax_t bytes)
{
    if (bytes < 1024)
        return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024)
        return std::to_string(bytes / 1024) + "." + std::to_string((bytes % 1024) / 100) + " KB";
    return std::to_string(bytes / (1024 * 1024)) + "." + std::to_string(((bytes % (1024 * 1024)) * 10) / (1024 * 1024)) + " MB";
}

static bool IsEditorSupportEntry(const std::filesystem::path& path, bool isDirectory)
{
    const std::string name = path.filename().string();
    if (!name.empty() && name.front() == '.')
        return true;

    return isDirectory && name == "api" && path.parent_path().filename() == "Scripts";
}

static void NavigateTo(const std::filesystem::path& path)
{
    if (path.empty()) return;
    if (s_CurrentDirectory == path) return;
    ThumbnailCache::Get().OnDirectoryChanged();

    if (s_NavHistoryIndex >= 0 && s_NavHistoryIndex + 1 < (int)s_NavHistory.size())
    {
        s_NavHistory.erase(s_NavHistory.begin() + s_NavHistoryIndex + 1, s_NavHistory.end());
    }

    s_NavHistory.push_back(path);
    s_NavHistoryIndex = (int)s_NavHistory.size() - 1;
    s_CurrentDirectory = path;
    s_SearchBuffer[0] = '\0';
    ClearSelectionInternal();
    s_NeedsRefresh = true;
    s_BreadcrumbsDirty = true;
}

static void NavigateBack()
{
    if (s_NavHistoryIndex > 0)
    {
        ThumbnailCache::Get().OnDirectoryChanged();
        s_NavHistoryIndex--;
        s_CurrentDirectory = s_NavHistory[s_NavHistoryIndex];
        s_SearchBuffer[0] = '\0';
        ClearSelectionInternal();
        s_NeedsRefresh = true;
        s_BreadcrumbsDirty = true;
    }
}

static void NavigateForward()
{
    if (s_NavHistoryIndex + 1 < (int)s_NavHistory.size())
    {
        ThumbnailCache::Get().OnDirectoryChanged();
        s_NavHistoryIndex++;
        s_CurrentDirectory = s_NavHistory[s_NavHistoryIndex];
        s_SearchBuffer[0] = '\0';
        ClearSelectionInternal();
        s_NeedsRefresh = true;
        s_BreadcrumbsDirty = true;
    }
}

static void NavigateUp(const std::filesystem::path& rootDirectory)
{
    if (s_CurrentDirectory != rootDirectory && s_CurrentDirectory.has_parent_path())
    {
        auto parent = s_CurrentDirectory.parent_path();
        if (IsSubpathOf(parent, rootDirectory))
            NavigateTo(parent);
        else
            NavigateTo(rootDirectory);
    }
}

static void DrawReimportWindow(AssetImportPipeline& importPipeline)
{
    if (!s_ReimportState.open)
        return;

    ImGui::SetNextWindowSize(ImVec2(480, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Asset Import Settings", &s_ReimportState.open))
    {
        if (ImportSettingsPanel::DrawAssetSettingsUI(s_ReimportState.path, importPipeline, true))
        {
            s_ReimportState.open = false;
        }
    }
    ImGui::End();
}

static void DeleteSinglePathWithMeta(const std::filesystem::path& path)
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

    auto it = std::find(s_SelectedPaths.begin(), s_SelectedPaths.end(), path);
    if (it != s_SelectedPaths.end())
        s_SelectedPaths.erase(it);

    if (s_SelectedItemPath == path)
        s_SelectedItemPath = s_SelectedPaths.empty() ? std::filesystem::path() : s_SelectedPaths.back();
}

static void DeleteSelectedPathsWithMeta()
{
    if (s_SelectedPaths.empty())
    {
        if (!s_SelectedItemPath.empty())
            DeleteSinglePathWithMeta(s_SelectedItemPath);
    }
    else
    {
        auto pathsToDelete = s_SelectedPaths;
        for (const auto& path : pathsToDelete)
        {
            DeleteSinglePathWithMeta(path);
        }
    }

    auto& reg = AssetManager::Get().GetRegistry();
    reg.Scan(reg.GetAssetDir());
    EditorUtils::ClearAssetFileNameCache();

    ClearSelectionInternal();
    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}

static void DeletePathWithMeta(const std::filesystem::path& path)
{
    if (s_SelectedPaths.size() > 1 && IsSelected(path))
    {
        DeleteSelectedPathsWithMeta();
        return;
    }

    DeleteSinglePathWithMeta(path);

    auto& reg = AssetManager::Get().GetRegistry();
    reg.Scan(reg.GetAssetDir());
    EditorUtils::ClearAssetFileNameCache();

    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}

static bool MoveSingleAsset(const std::filesystem::path& sourcePath, const std::filesystem::path& targetDir)
{
    if (sourcePath.empty() || targetDir.empty())
        return false;

    std::error_code ec;
    if (!std::filesystem::exists(sourcePath, ec) || !std::filesystem::is_directory(targetDir, ec))
        return false;

    if (sourcePath.parent_path() == targetDir)
        return false;

    if (std::filesystem::is_directory(sourcePath, ec))
    {
        if (sourcePath == targetDir)
            return false;

        auto rel = std::filesystem::relative(targetDir, sourcePath, ec);
        if (!ec && !rel.empty() && rel.native()[0] != '.' && rel.string() != "..")
        {
            LOG_WARN("Cannot move folder into its own subfolder: {}", targetDir.string());
            return false;
        }
    }

    auto destPath = targetDir / sourcePath.filename();
    if (std::filesystem::exists(destPath, ec))
    {
        LOG_WARN("Cannot move: an item named '{}' already exists in destination.", destPath.filename().string());
        return false;
    }

    std::filesystem::rename(sourcePath, destPath, ec);
    if (ec)
    {
        ec.clear();
        if (std::filesystem::is_directory(sourcePath, ec))
        {
            std::filesystem::copy(sourcePath, destPath, std::filesystem::copy_options::recursive, ec);
            if (!ec) std::filesystem::remove_all(sourcePath, ec);
        }
        else
        {
            std::filesystem::copy_file(sourcePath, destPath, ec);
            if (!ec) std::filesystem::remove(sourcePath, ec);
        }

        if (ec)
        {
            LOG_ERROR("Move failed for '{}' to '{}': {}", sourcePath.string(), targetDir.string(), ec.message());
            return false;
        }
    }

    auto sourceMeta = std::filesystem::path(sourcePath.string() + ".rmeta");
    auto destMeta = std::filesystem::path(destPath.string() + ".rmeta");
    if (std::filesystem::exists(sourceMeta, ec))
    {
        ec.clear();
        std::filesystem::rename(sourceMeta, destMeta, ec);
        if (ec)
        {
            ec.clear();
            std::filesystem::copy_file(sourceMeta, destMeta, ec);
            if (!ec) std::filesystem::remove(sourceMeta, ec);
        }
    }

    auto it = std::find(s_SelectedPaths.begin(), s_SelectedPaths.end(), sourcePath);
    if (it != s_SelectedPaths.end())
    {
        if (destPath.parent_path() == s_CurrentDirectory)
            *it = destPath;
        else
            s_SelectedPaths.erase(it);
    }

    if (s_SelectedItemPath == sourcePath)
    {
        if (destPath.parent_path() == s_CurrentDirectory)
            s_SelectedItemPath = destPath;
        else
            s_SelectedItemPath = s_SelectedPaths.empty() ? std::filesystem::path() : s_SelectedPaths.back();
    }

    return true;
}

static bool MoveAssetToDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& targetDir)
{
    bool anyMoved = false;

    if (s_SelectedPaths.size() > 1 && IsSelected(sourcePath))
    {
        auto pathsToMove = s_SelectedPaths;
        for (const auto& p : pathsToMove)
        {
            if (MoveSingleAsset(p, targetDir))
                anyMoved = true;
        }
    }
    else
    {
        anyMoved = MoveSingleAsset(sourcePath, targetDir);
    }

    if (anyMoved)
    {
        auto& reg = AssetManager::Get().GetRegistry();
        reg.Scan(reg.GetAssetDir());
        EditorUtils::ClearAssetFileNameCache();

        s_NeedsRefresh = true;
        s_DirectoryTreeDirty = true;
    }

    return anyMoved;
}

static void HandleReimport(const std::filesystem::path& path, AssetImportPipeline& importPipeline)
{
    s_ReimportState.open = true;
    s_ReimportState.path = path;
    ImportSettingsPanel::SetInspectedAsset(path);
}

static void CreateEmptyTextFile(const std::filesystem::path& path, const std::string& content = "")
{
    std::ofstream f(path);
    if (!f) { LOG_ERROR("Could not create file: {}", path.string()); return; }
    f << content;
}

static std::string MakeLuaTemplate(const std::string& scriptType)
{
    return
        "---@class " + scriptType + " : ScriptInstance\n" +
        "local " + scriptType + " = {}\n\n" +
        "function " + scriptType + ":OnReady()\n" +
        "end\n\n" +
        "function " + scriptType + ":OnUpdate(dt)\n" +
        "end\n\n" +
        "return " + scriptType + "\n";
}

static std::string MakeShaderTemplate()
{
    return
        "shader_type spatial;\n"
        "render_mode lit;\n\n"
        "uniform vec4 u_Color : source_color = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "uniform float u_Metallic : hint_range(0.0, 1.0) = 0.0;\n"
        "uniform float u_Roughness : hint_range(0.0, 1.0) = 0.5;\n"
        "uniform sampler2D u_MainTexture;\n\n"
        "void vertex() {\n"
        "    // Modify VERTEX, NORMAL, UV, etc.\n"
        "}\n\n"
        "void fragment() {\n"
        "    vec4 tex = texture(u_MainTexture, UV);\n"
        "    ALBEDO = tex.rgb * u_Color.rgb;\n"
        "    ALPHA = tex.a * u_Color.a;\n"
        "    METALLIC = u_Metallic;\n"
        "    ROUGHNESS = u_Roughness;\n"
        "}\n";
}

static std::filesystem::path MakeUniquePath(const std::filesystem::path& dir,
    const std::string& stem, const std::string& ext)
{
    auto p = dir / (stem + ext);
    int i = 1;
    while (std::filesystem::exists(p))
        p = dir / (stem + std::to_string(i++) + ext);
    return p;
}

static void BeginRenameForNew(const std::filesystem::path& path)
{
    const bool isDir = std::filesystem::is_directory(path);
    s_RenamingPath = path;
    s_RenamingActive = true;
    s_RenameExtension = isDir ? "" : path.extension().string();
    const std::string initial = isDir ? path.filename().string() : path.stem().string();
    strncpy_s(s_RenameBuffer, sizeof(s_RenameBuffer), initial.c_str(), _TRUNCATE);
    s_SearchBuffer[0] = '\0';
    SetSingleSelection(path);
    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}

static void RenamePathWithMeta(const std::filesystem::path& oldPath, const std::string& newName)
{
    if (newName.empty())
    {
        SetSingleSelection(oldPath);
        return;
    }

    auto newPath = oldPath.parent_path() / newName;
    if (newPath == oldPath)
    {
        SetSingleSelection(newPath);
        return;
    }

    if (std::filesystem::exists(newPath))
    {
        LOG_WARN("Rename skipped, already exists: {}", newName);
        SetSingleSelection(oldPath);
        return;
    }

    std::error_code ec;
    std::filesystem::rename(oldPath, newPath, ec);
    if (ec)
    {
        LOG_ERROR("Rename failed: {}", ec.message());
        SetSingleSelection(oldPath);
        return;
    }

    auto oldMeta = std::filesystem::path(oldPath.string() + ".rmeta");
    auto newMeta = std::filesystem::path(newPath.string() + ".rmeta");
    auto& reg = AssetManager::Get().GetRegistry();

    if (std::filesystem::exists(oldMeta))
    {
        std::filesystem::rename(oldMeta, newMeta, ec);
        if (ec) LOG_WARN("Meta rename failed: {}", ec.message());
    }
    else if (!std::filesystem::is_directory(newPath))
    {
        reg.GetOrCreateMeta(newPath);
    }

    if (newPath.extension() == ".lua")
    {
        std::string content;
        {
            std::ifstream in(newPath);
            std::stringstream ss;
            ss << in.rdbuf();
            content = ss.str();
        }
        if (content == MakeLuaTemplate(oldPath.stem().string()))
            CreateEmptyTextFile(newPath, MakeLuaTemplate(newPath.stem().string()));
    }

    AssetUUID assetUuid = EditorUtils::ReadUUIDFromMeta(newPath.string());
    if (assetUuid.IsValid())
    {
        reg.RegisterPath(assetUuid, newPath);
    }

    reg.Scan(reg.GetAssetDir());
    EditorUtils::ClearAssetFileNameCache();

    SetSingleSelection(newPath);

    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}

static void BuildDirectoryTreeRecursive(const std::filesystem::path& path, std::vector<DirNode>& out)
{
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(path, ec))
    {
        if (ec || !entry.is_directory() || IsEditorSupportEntry(entry.path(), true))
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
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (node.path == selected)
            flags |= ImGuiTreeNodeFlags_Selected;
        if (node.children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        bool opened = ImGui::TreeNodeEx(node.path.string().c_str(), flags, "%s", node.name.c_str());

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                std::filesystem::path srcPath((const char*)payload->Data);
                MoveAssetToDirectory(srcPath, node.path);
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            NavigateTo(node.path);
        }

        std::string treePopupId = "##TreeMenu_" + node.path.string();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup(treePopupId.c_str());
        }

        if (ImGui::BeginPopup(treePopupId.c_str()))
        {
            ImGui::TextDisabled("%s", node.name.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("New Folder"))
            {
                auto newPath = MakeUniquePath(node.path, "New Folder", "");
                std::error_code ec;
                std::filesystem::create_directory(newPath, ec);
                if (!ec)
                {
                    NavigateTo(node.path);
                    BeginRenameForNew(newPath);
                }
            }
            if (ImGui::MenuItem("Open in Explorer"))
            {
                std::string cmd = "explorer \"" + node.path.string() + "\"";
                std::system(cmd.c_str());
            }
            if (ImGui::MenuItem("Delete Folder"))
            {
                DeletePathWithMeta(node.path);
            }
            ImGui::EndPopup();
        }

        if (opened)
        {
            if (!node.children.empty())
                RenderDirectoryRecursiveCached(node.children, selected);
            ImGui::TreePop();
        }
    }
}

static ImTextureID ResolveFileIcon(const std::string& extension, bool isDirectory)
{
    auto& lib = IconLibrary::Get();
    if (isDirectory) return lib.GetIcon(EditorIcon::Folder).GetID();
    if (extension == ".rscene") return lib.GetIcon(EditorIcon::Scene).GetID();
    if (extension == ".rprefab") return lib.GetIcon(EditorIcon::Prefab).GetID();
    if (extension == ".rtex" || extension == ".png" || extension == ".jpeg" ||
        extension == ".jpg" || extension == ".tga" || extension == ".hdr" || extension == ".Texture")
        return lib.GetIcon(EditorIcon::Texture).GetID();
    if (extension == ".mp3" || extension == ".ogg" || extension == ".wav" || extension == ".Audio")
        return lib.GetIcon(EditorIcon::Audio).GetID();
    if (extension == ".rmat" || extension == ".Material") return lib.GetIcon(EditorIcon::Material).GetID();
    if (extension == ".rmesh" || extension == ".glb" || extension == ".fbx" || extension == ".obj" || extension == ".gltf" || extension == ".Mesh")
        return lib.GetIcon(EditorIcon::Mesh).GetID();
    if (extension == ".ranimlib" || extension == ".Animation") return lib.GetIcon(EditorIcon::AnimLib).GetID();
    if (extension == ".rskmesh" || extension == ".SkeletalMesh") return lib.GetIcon(EditorIcon::Skinned).GetID();
    if (extension == ".rskeleton" || extension == ".Skeleton") return lib.GetIcon(EditorIcon::Skeleton).GetID();
    if (extension == ".rshader" || extension == ".glsl")
        return lib.GetIcon(EditorIcon::Shader).GetID();
    if (extension == ".lua")
        return lib.GetIcon(EditorIcon::Script).GetID();

    return lib.GetIcon(EditorIcon::Script).GetID();
}

static std::pair<std::string, std::string> SplitFileNameTwoLines(const std::string& name, float maxWidth)
{
    if (ImGui::CalcTextSize(name.c_str()).x <= maxWidth)
    {
        return { name, "" };
    }

    size_t bestSplit = 0;
    for (size_t i = 1; i < name.size(); ++i)
    {
        if (ImGui::CalcTextSize(name.substr(0, i).c_str()).x > maxWidth)
            break;

        char c = name[i];
        char prev = name[i - 1];
        if (prev == '.' || prev == '_' || prev == '-' || prev == ' ' || (isupper(c) && !isupper(prev)))
        {
            bestSplit = i;
        }
    }

    if (bestSplit == 0)
    {
        for (size_t i = 1; i < name.size(); ++i)
        {
            if (ImGui::CalcTextSize(name.substr(0, i).c_str()).x > maxWidth)
            {
                bestSplit = (i > 1) ? (i - 1) : 1;
                break;
            }
        }
    }

    std::string line1 = name.substr(0, bestSplit);
    std::string line2 = name.substr(bestSplit);

    if (ImGui::CalcTextSize(line2.c_str()).x > maxWidth)
    {
        while (line2.size() > 3 && ImGui::CalcTextSize((line2 + "...").c_str()).x > maxWidth)
        {
            line2.pop_back();
        }
        line2 += "...";
    }

    return { line1, line2 };
}

static void RefreshAssets(const std::filesystem::path& directory)
{
    s_CachedItems.clear();
    s_CachedItems.reserve(128);

    try
    {
        if (IsModelFile(directory))
        {
            auto metaPath = AssetMeta::GetMetaPath(directory);
            AssetMeta meta;
            if (meta.LoadFromFile(metaPath))
            {
                for (const auto& sub : meta.subAssets)
                {
                    if (!sub.uuid.IsValid()) continue;
                    BrowserItem item;
                    item.path = directory.string() + "#" + sub.uuid.ToString();
                    item.filename = sub.name.empty() ? (sub.type + "_" + std::to_string(sub.index)) : sub.name;
                    item.stem = item.filename;
                    item.extension = "." + sub.type;
                    item.isDirectory = false;
                    item.isSubAsset = true;
                    item.fileSize = 0;
                    item.icon = ResolveFileIcon(item.extension, false);
                    s_CachedItems.emplace_back(std::move(item));
                }
            }
            s_NeedsRefresh = false;
            return;
        }

        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
        {
            if (ec) break;
            const bool isDir = entry.is_directory();
            if (IsEditorSupportEntry(entry.path(), isDir))
                continue;

            auto ext = entry.path().extension().string();
            if (!isDir && ext == ".rmeta")
                continue;

            BrowserItem item;
            item.path = entry.path();
            item.filename = entry.path().filename().string();
            item.stem = entry.path().stem().string();
            item.extension = ext;
            item.isDirectory = isDir;
            item.fileSize = isDir ? 0 : entry.file_size(ec);
            item.icon = ResolveFileIcon(ext, isDir);

            if (!isDir && IsModelFile(entry.path()))
            {
                auto metaPath = AssetMeta::GetMetaPath(entry.path());
                AssetMeta meta;
                if (meta.LoadFromFile(metaPath))
                {
                    item.subAssetCount = (int)meta.subAssets.size();
                }
            }

            s_CachedItems.emplace_back(std::move(item));
        }

        std::stable_sort(s_CachedItems.begin(), s_CachedItems.end(),
            [](const BrowserItem& a, const BrowserItem& b)
            {
                if (a.isDirectory != b.isDirectory)
                    return a.isDirectory > b.isDirectory;
                return a.filename < b.filename;
            });
    }
    catch (...) {}

    s_SelectedPaths.erase(
        std::remove_if(s_SelectedPaths.begin(), s_SelectedPaths.end(),
            [](const std::filesystem::path& p) {
                std::error_code ec;
                std::string pStr = p.string();
                size_t hashPos = pStr.find('#');
                if (hashPos != std::string::npos)
                    return !std::filesystem::exists(pStr.substr(0, hashPos), ec);
                return !std::filesystem::exists(p, ec);
            }),
        s_SelectedPaths.end()
    );

    if (!IsSelected(s_SelectedItemPath))
        s_SelectedItemPath = s_SelectedPaths.empty() ? std::filesystem::path() : s_SelectedPaths.back();

    if (!s_SelectedItemPath.empty())
    {
        for (size_t i = 0; i < s_CachedItems.size(); ++i)
        {
            if (s_CachedItems[i].path == s_SelectedItemPath)
            {
                s_SelectedIsDir = s_CachedItems[i].isDirectory;
                s_SelectedFileSize = s_CachedItems[i].fileSize;
                s_LastSelectedIndex = (int)i;
                break;
            }
        }
    }

    s_NeedsRefresh = false;
}

static void CollectSearchResults(const std::vector<BrowserItem>& items, std::string_view query)
{
    s_SearchItemResults.clear();
    s_SearchItemResults.reserve(items.size());

    std::string queryLower;
    queryLower.reserve(query.size());
    std::transform(query.begin(), query.end(), std::back_inserter(queryLower), ::tolower);

    for (const auto& item : items)
    {
        std::string filenameLower = item.filename;
        std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(), ::tolower);

        if (filenameLower.find(queryLower) != std::string::npos)
            s_SearchItemResults.emplace_back(item);
    }
}

static void OpenInVSCode(const std::filesystem::path& targetPath)
{
    std::filesystem::path scriptsDir = ProjectManager::GetAssetDirectory() / "Scripts";
    std::string cmd;
    if (std::filesystem::is_directory(targetPath))
    {
        cmd = "code \"" + targetPath.string() + "\"";
    }
    else if (targetPath.extension() == ".lua" && std::filesystem::exists(scriptsDir))
    {
        cmd = "code \"" + scriptsDir.string() + "\" \"" + targetPath.string() + "\"";
    }
    else
    {
        cmd = "code \"" + targetPath.string() + "\"";
    }
    std::system(cmd.c_str());
}

static void OpenBrowserItem(const BrowserItem& item, Engine* engine)
{
    if (item.isDirectory)
    {
        NavigateTo(item.path);
    }
    else if (IsModelFile(item.path) && item.subAssetCount > 0)
    {
        NavigateTo(item.path);
    }
    else if (item.isSubAsset && (item.extension == ".Mesh" || item.extension == ".SkeletalMesh"))
    {
        AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(item.path.string());
        if (uuid.IsValid() && engine)
        {
            glm::vec3 spawnPos(0.0f);
            if (auto* cam = engine->GetEditorCamera())
                spawnPos = cam->Position + cam->Front * 5.0f;
            Entity e = engine->GetActiveScene().CreateEntity(item.stem);
            e.GetComponent<TransformComponent>().SetPosition(spawnPos);
            if (item.extension == ".Mesh")
            {
                e.AddComponent<MeshComponent>().SetMesh(uuid);
                e.AddComponent<MeshRendererComponent>();
            }
            else
            {
                e.AddComponent<SkeletalMeshComponent>().SetMesh(uuid);
                e.AddComponent<SkeletalMeshRendererComponent>();
            }
            EditorSelection::Get().Select(e.GetHandle());
        }
    }
    else if (item.extension == ".rscene")
    {
        if (engine)
        {
            CommandHistory::Get().Clear();
            EditorSelection::Get().Clear();
            engine->GetSceneManager().LoadScene(item.path.string());
        }
    }
    else if (item.extension == ".rprefab")
    {
        AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(item.path.string());
        if (uuid.IsValid() && engine)
        {
            glm::vec3 spawnPos(0.0f);
            if (auto* cam = engine->GetEditorCamera())
                spawnPos = cam->Position + cam->Front * 5.0f;
            Entity inst = engine->GetActiveScene().Instantiate(uuid, spawnPos);
            if (inst.GetHandle() != entt::null)
                EditorSelection::Get().Select(inst.GetHandle());
        }
    }
    else if (item.extension == ".glsl" || item.extension == ".lua" || item.extension == ".rshader")
    {
        if (item.extension == ".lua")
            OpenInVSCode(item.path);
        else
        {
            std::string command = "code \"" + item.path.string() + "\"";
            std::system(command.c_str());
        }
    }
    else if (item.extension == ".png" || item.extension == ".jpeg" ||
             item.extension == ".jpg" || item.extension == ".tga")
    {
        std::string command = "\"" + item.path.string() + "\"";
        std::system(command.c_str());
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
                auto newPath = MakeUniquePath(currentDir, "New Folder", "");
                std::error_code ec;
                std::filesystem::create_directory(newPath, ec);
                if (!ec) BeginRenameForNew(newPath);
                else LOG_ERROR("Create folder failed: {}", ec.message());
            }

            ImGui::Separator();

            auto& assetReg = AssetManager::Get().GetRegistry();

            if (ImGui::MenuItem("Material"))
            {
                auto newPath = MakeUniquePath(currentDir, "NewMaterial", ".rmat");
                MaterialSerializer::CreateNew(newPath, assetReg);
                assetReg.Scan(assetReg.GetAssetDir());
                BeginRenameForNew(newPath);
            }

            if (ImGui::MenuItem("AnimationLibrary"))
            {
                auto newPath = MakeUniquePath(currentDir, "NewAnimationLibrary", ".ranimlib");
                AnimationLibrarySerializer::CreateNew(newPath, assetReg);
                assetReg.Scan(assetReg.GetAssetDir());
                BeginRenameForNew(newPath);
            }

            if (ImGui::MenuItem("Scene"))
            {
                auto newPath = MakeUniquePath(currentDir, "NewScene", ".rscene");
                auto scene = engine->GetSceneManager().CreateScene("NewScene");
                engine->GetSceneManager().SaveScene(*scene, newPath.string());
                assetReg.GetOrCreateMeta(newPath);
                assetReg.Scan(assetReg.GetAssetDir());
                BeginRenameForNew(newPath);
            }

            if (ImGui::MenuItem("Lua Script"))
            {
                auto newPath = MakeUniquePath(currentDir, "NewScript", ".lua");
                CreateEmptyTextFile(newPath, MakeLuaTemplate(newPath.stem().string()));
                assetReg.GetOrCreateMeta(newPath);
                assetReg.Scan(assetReg.GetAssetDir());
                BeginRenameForNew(newPath);
            }

            if (ImGui::MenuItem("Shader"))
            {
                auto newPath = MakeUniquePath(currentDir, "NewShader", ".rshader");
                CreateEmptyTextFile(newPath, MakeShaderTemplate());
                assetReg.GetOrCreateMeta(newPath);
                assetReg.Scan(assetReg.GetAssetDir());
                BeginRenameForNew(newPath);
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Refresh"))
        {
            auto& reg = AssetManager::Get().GetRegistry();
            reg.Scan(reg.GetAssetDir());
            EditorUtils::ClearAssetFileNameCache();
            s_NeedsRefresh = true;
            s_DirectoryTreeDirty = true;
        }

        if (ImGui::MenuItem("Open in Explorer"))
        {
            std::string cmd = "explorer \"" + currentDir.string() + "\"";
            std::system(cmd.c_str());
        }

        if (ImGui::MenuItem("Open in VS Code"))
        {
            OpenInVSCode(currentDir);
        }

        ImGui::EndPopup();
    }
}

static void DrawItemContextMenu(const std::filesystem::path& itemPath, AssetImportPipeline& importPipeline)
{
    std::string popupId = "##ItemMenu_" + itemPath.filename().string();

    if (ImGui::BeginPopup(popupId.c_str()))
    {
        if (s_SelectedPaths.size() > 1)
        {
            ImGui::TextDisabled("%d items selected", (int)s_SelectedPaths.size());
        }
        else
        {
            ImGui::TextDisabled("%s", itemPath.filename().string().c_str());
        }
        ImGui::Separator();

        if (s_SelectedPaths.size() <= 1)
        {
            if (ImGui::MenuItem("Rename", "F2"))
            {
                BeginRenameForNew(itemPath);
            }
        }

        if (ImGui::MenuItem("Delete", "Del"))
        {
            DeletePathWithMeta(itemPath);
        }

        ImGui::Separator();

        if (s_SelectedPaths.size() <= 1)
        {
            if (ImGui::MenuItem("Copy Path"))
                ImGui::SetClipboardText(itemPath.string().c_str());

            if (ImGui::MenuItem("Copy Relative Path"))
            {
                std::error_code ec;
                auto rel = std::filesystem::relative(itemPath, ProjectManager::GetAssetDirectory(), ec);
                if (!ec) ImGui::SetClipboardText(rel.string().c_str());
            }
        }
        else
        {
            if (ImGui::MenuItem("Copy Paths"))
            {
                std::string allPaths;
                for (size_t i = 0; i < s_SelectedPaths.size(); ++i)
                {
                    if (i > 0) allPaths += "\n";
                    allPaths += s_SelectedPaths[i].string();
                }
                ImGui::SetClipboardText(allPaths.c_str());
            }

            if (ImGui::MenuItem("Copy Relative Paths"))
            {
                std::string allRel;
                std::error_code ec;
                for (size_t i = 0; i < s_SelectedPaths.size(); ++i)
                {
                    if (i > 0) allRel += "\n";
                    auto rel = std::filesystem::relative(s_SelectedPaths[i], ProjectManager::GetAssetDirectory(), ec);
                    if (!ec) allRel += rel.string();
                    else allRel += s_SelectedPaths[i].string();
                }
                ImGui::SetClipboardText(allRel.c_str());
            }
        }

        ImGui::Separator();

        if (s_SelectedPaths.size() <= 1)
        {
            if (ImGui::MenuItem("Open in Explorer"))
            {
                auto dir = std::filesystem::is_directory(itemPath)
                    ? itemPath
                    : itemPath.parent_path();
                std::string cmd = "explorer \"" + dir.string() + "\"";
                std::system(cmd.c_str());
            }

            if (std::filesystem::is_directory(itemPath))
            {
                if (ImGui::MenuItem("Open in VS Code"))
                {
                    OpenInVSCode(itemPath);
                }
            }
            else
            {
                auto ext = itemPath.extension();
                if (ext == ".glsl" || ext == ".lua")
                {
                    if (ImGui::MenuItem("Open in VS Code"))
                    {
                        OpenInVSCode(itemPath);
                    }
                }

                if (ImGui::MenuItem("Import Settings..."))
                    HandleReimport(itemPath, importPipeline);

                if (ImGui::MenuItem("Reimport"))
                    importPipeline.Reimport(itemPath, AssetManager::Get().GetRegistry());
            }
        }
        else
        {
            if (ImGui::MenuItem("Open in Explorer"))
            {
                auto dir = std::filesystem::is_directory(itemPath)
                    ? itemPath
                    : itemPath.parent_path();
                std::string cmd = "explorer \"" + dir.string() + "\"";
                std::system(cmd.c_str());
            }

            bool anyReimportable = false;
            for (const auto& p : s_SelectedPaths)
            {
                if (!std::filesystem::is_directory(p))
                {
                    auto metaP = std::filesystem::path(p.string() + ".rmeta");
                    if (std::filesystem::exists(metaP))
                    {
                        anyReimportable = true;
                        break;
                    }
                }
            }

            if (anyReimportable && ImGui::MenuItem("Reimport All"))
            {
                for (const auto& p : s_SelectedPaths)
                {
                    if (!std::filesystem::is_directory(p))
                        HandleReimport(p, importPipeline);
                }
            }
        }

        ImGui::EndPopup();
    }
}

void AssetBrowserPanel::Draw(Engine* engine, const std::filesystem::path& rootDirectory, AssetImportPipeline& importPipeline)
{
    s_RenameJustCommitted = false;

    bool rootChanged = !ArePathsEqual(rootDirectory, s_LastRootDirectory);
    bool currentInvalid = s_CurrentDirectory.empty() || 
                          !std::filesystem::exists(s_CurrentDirectory) || 
                          !IsSubpathOf(s_CurrentDirectory, rootDirectory);

    if (rootChanged || currentInvalid)
    {
        ResetInternal(rootDirectory);
    }

    if (s_DirectoryTreeDirty)
    {
        s_DirectoryTree.clear();
        if (!rootDirectory.empty() && std::filesystem::exists(rootDirectory))
        {
            BuildDirectoryTreeRecursive(rootDirectory, s_DirectoryTree);
        }
        s_DirectoryTreeDirty = false;
    }

    if (!ImGui::Begin("Asset Browser", nullptr,
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    constexpr float leftPanelWidth = 200.0f;
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, leftPanelWidth);

    // Directory Tree
    ImGui::BeginChild("FoldersTree", ImVec2(0, 0), true);

    auto rootName = rootDirectory.filename().string();
    if (rootName.empty()) rootName = "Assets";

    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
    if (s_CurrentDirectory == rootDirectory)
        rootFlags |= ImGuiTreeNodeFlags_Selected;

    bool rootOpened = ImGui::TreeNodeEx("##rootFolderNode", rootFlags, "%s", rootName.c_str());
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
        {
            std::filesystem::path srcPath((const char*)payload->Data);
            MoveAssetToDirectory(srcPath, rootDirectory);
        }
        ImGui::EndDragDropTarget();
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        NavigateTo(rootDirectory);
    }

    if (rootOpened)
    {
        RenderDirectoryRecursiveCached(s_DirectoryTree, s_CurrentDirectory);
        ImGui::TreePop();
    }

    ImGui::EndChild();
    ImGui::NextColumn();

    // Navigation Area
    ImGui::BeginChild("FilesContainer", ImVec2(0, 0), true);

    // Toolbar Row
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

        ImTextureID backIcon = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::ArrowBack).GetID();
        ImTextureID fwdIcon = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::ArrowForward).GetID();
        ImTextureID upIcon = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::ArrowUp).GetID();
        const ImVec2 navIconSize(16.0f, 16.0f);
        const ImVec4 tintWhite(1.0f, 1.0f, 1.0f, 1.0f);
        const ImVec4 bgTrans(0.0f, 0.0f, 0.0f, 0.0f);

        bool canGoBack = (s_NavHistoryIndex > 0);
        if (!canGoBack) ImGui::BeginDisabled();
        if (ImGui::ImageButton("##navBack", backIcon, navIconSize, ImVec2(0, 0), ImVec2(1, 1), bgTrans, tintWhite))
        {
            NavigateBack();
        }
        if (!canGoBack) ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Back (Alt+Left)");

        ImGui::SameLine();
        bool canGoForward = (s_NavHistoryIndex + 1 < (int)s_NavHistory.size());
        if (!canGoForward) ImGui::BeginDisabled();
        if (ImGui::ImageButton("##navFwd", fwdIcon, navIconSize, ImVec2(0, 0), ImVec2(1, 1), bgTrans, tintWhite))
        {
            NavigateForward();
        }
        if (!canGoForward) ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Forward (Alt+Right)");

        ImGui::SameLine();
        bool canGoUp = (s_CurrentDirectory != rootDirectory);
        if (!canGoUp) ImGui::BeginDisabled();
        if (ImGui::ImageButton("##navUp", upIcon, navIconSize, ImVec2(0, 0), ImVec2(1, 1), bgTrans, tintWhite))
        {
            NavigateUp(rootDirectory);
        }
        if (!canGoUp) ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Up to parent folder (Alt+Up)");

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // Breadcrumb trail
        if (s_BreadcrumbsDirty)
        {
            s_CachedBreadcrumbs.clear();
            s_CachedBreadcrumbs.push_back({ rootName, rootDirectory });

            std::error_code ec;
            auto relPath = std::filesystem::relative(s_CurrentDirectory, rootDirectory, ec);
            if (!ec && !relPath.empty() && relPath != ".")
            {
                std::filesystem::path accum = rootDirectory;
                for (const auto& part : relPath)
                {
                    accum /= part;
                    s_CachedBreadcrumbs.push_back({ part.string(), accum });
                }
            }
            s_BreadcrumbsDirty = false;
        }

        const auto& breadcrumbs = s_CachedBreadcrumbs;
        for (size_t i = 0; i < breadcrumbs.size(); ++i)
        {
            const auto& [label, path] = breadcrumbs[i];
            bool isCurrent = (i == breadcrumbs.size() - 1);

            if (isCurrent)
            {
                ImVec4 acc = GetAccentColor();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(acc.x, acc.y, acc.z, 0.35f));
                ImGui::PushStyleColor(ImGuiCol_Text, GetAccentHoverColor());
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            }

            std::string btnId = label + "##crumb_" + std::to_string(i);
            if (ImGui::Button(btnId.c_str(), ImVec2(0.0f, 24.0f)))
            {
                if (!isCurrent)
                    NavigateTo(path);
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    std::filesystem::path srcPath((const char*)payload->Data);
                    MoveAssetToDirectory(srcPath, path);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopStyleColor(2);

            if (i + 1 < breadcrumbs.size())
            {
                ImGui::SameLine();
                ImGui::TextDisabled(">");
                ImGui::SameLine();
            }
        }

        const float searchWidth = 130.0f;
        const float refreshWidth = 55.0f;
        const float zoomWidth = 80.0f;
        const float totalRightWidth = searchWidth + refreshWidth + zoomWidth + 70.0f;
        const float availWidth = ImGui::GetContentRegionAvail().x;

        if (availWidth > totalRightWidth)
        {
            ImGui::SameLine(ImGui::GetCursorPosX() + availWidth - totalRightWidth);
        }
        else
        {
            ImGui::SameLine();
        }

        ImGui::SetNextItemWidth(searchWidth);
        bool searchChanged = ImGui::InputTextWithHint("##Search", "Search...", s_SearchBuffer, sizeof(s_SearchBuffer));

        if (s_SearchBuffer[0] != '\0')
        {
            ImGui::SameLine();
            if (ImGui::Button("X##clearSearch", ImVec2(20.0f, 24.0f)))
            {
                s_SearchBuffer[0] = '\0';
                searchChanged = true;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clear search");
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh", ImVec2(refreshWidth, 24.0f)))
        {
            s_NeedsRefresh = true;
            s_DirectoryTreeDirty = true;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Zoom:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(zoomWidth);
        ImGui::SliderFloat("##ThumbnailSize", &s_ThumbnailSize, 28.0f, 128.0f, "%.0f px");

        ImGui::PopStyleVar(3);

        if (s_NeedsRefresh)
            RefreshAssets(s_CurrentDirectory);

        if (searchChanged && s_SearchBuffer[0] != '\0')
            CollectSearchResults(s_CachedItems, s_SearchBuffer);
    }

    ImGui::Separator();

    ThumbnailCache::Get().BeginFrame();
    ImGui::BeginChild("FilesGridArea", ImVec2(0.0f, -24.0f), false);

    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && io.KeyCtrl)
    {
        float wheel = io.MouseWheel;
        if (wheel != 0.0f)
        {
            s_ThumbnailSize = std::clamp(s_ThumbnailSize + wheel * 6.0f, 28.0f, 128.0f);
        }
    }

    const auto& itemsToShow = (s_SearchBuffer[0] != '\0') ? s_SearchItemResults : s_CachedItems;

    const float thumbnailSize = s_ThumbnailSize;
    const float lineHeight = ImGui::GetTextLineHeight();
    const float cardWidth = std::round(std::max(thumbnailSize + 16.0f, thumbnailSize * 1.25f));
    const float padTop = 4.0f;
    const float padLabel = 3.0f;
    const float cardHeight = thumbnailSize + padTop + padLabel + lineHeight * 2.0f + 4.0f;
    const float cardSpacing = 6.0f;

    const float scrollY = ImGui::GetScrollY();
    const float windowVisibleHeight = ImGui::GetWindowHeight();
    const float clipMinY = scrollY - cardHeight;
    const float clipMaxY = scrollY + windowVisibleHeight + cardHeight;
    const ImVec2 childScreenPos = ImGui::GetCursorScreenPos();

    bool isDraggingBox = s_IsBoxSelecting && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f);

    if (isDraggingBox)
    {
        const float autoScrollMargin = 25.0f;
        const float autoScrollSpeed = 8.0f;
        const float winMinY = childScreenPos.y;
        const float winMaxY = childScreenPos.y + windowVisibleHeight;

        if (io.MousePos.y < winMinY + autoScrollMargin && ImGui::GetScrollY() > 0.0f)
        {
            ImGui::SetScrollY(std::max(0.0f, ImGui::GetScrollY() - autoScrollSpeed));
        }
        else if (io.MousePos.y > winMaxY - autoScrollMargin && ImGui::GetScrollY() < ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollY(std::min(ImGui::GetScrollMaxY(), ImGui::GetScrollY() + autoScrollSpeed));
        }
    }

    ImVec2 boxStartScreen(
        s_BoxSelectStartPos.x + (s_BoxSelectStartScroll.x - ImGui::GetScrollX()) + (childScreenPos.x - s_BoxSelectStartWindowPos.x),
        s_BoxSelectStartPos.y + (s_BoxSelectStartScroll.y - ImGui::GetScrollY()) + (childScreenPos.y - s_BoxSelectStartWindowPos.y)
    );
    ImVec2 boxMin = ImMin(boxStartScreen, io.MousePos);
    ImVec2 boxMax = ImMax(boxStartScreen, io.MousePos);

    std::vector<std::filesystem::path> currentFrameSelected;
    if (isDraggingBox)
        currentFrameSelected.reserve(itemsToShow.size());

    for (size_t i = 0; i < itemsToShow.size(); ++i)
    {
        const auto& item = itemsToShow[i];
        ImGui::PushID(item.filename.c_str());

        ImGui::BeginGroup();

        const ImVec2 cellPos = ImGui::GetCursorScreenPos();
        const float itemRelativeY = cellPos.y - childScreenPos.y + scrollY;
        const bool isRenamingThisItem = (s_RenamingActive && s_RenamingPath == item.path);
        if (isRenamingThisItem)
        {
            ImGui::SetScrollHereY(0.5f);
        }
        const bool isVisible = isRenamingThisItem || (itemRelativeY >= clipMinY && itemRelativeY <= clipMaxY);

        const ImVec2 cardMin = cellPos;
        const ImVec2 cardMax = ImVec2(cellPos.x + cardWidth, cellPos.y + cardHeight);

        bool inBox = false;
        if (isDraggingBox)
        {
            inBox = (boxMin.x <= cardMax.x && boxMax.x >= cardMin.x &&
                     boxMin.y <= cardMax.y && boxMax.y >= cardMin.y);
        }

        bool isSelected = false;
        if (isDraggingBox)
        {
            if (s_BoxSelectCtrl || s_BoxSelectShift)
            {
                bool wasPreSelected = std::find(s_PreBoxSelectedPaths.begin(), s_PreBoxSelectedPaths.end(), item.path) != s_PreBoxSelectedPaths.end();
                isSelected = wasPreSelected || inBox;
            }
            else
            {
                isSelected = inBox;
            }

            if (isSelected)
            {
                currentFrameSelected.push_back(item.path);
            }
        }
        else
        {
            isSelected = IsSelected(item.path);
        }

        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
        ImGui::Selectable("##card", isSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(cardWidth, cardHeight));
        ImGui::PopStyleColor(3);

        if (!isVisible)
        {
            ImGui::EndGroup();
            ImGui::PopID();

            if (i + 1 < itemsToShow.size())
            {
                const float lastRight = ImGui::GetItemRectMax().x;
                const float nextRight = lastRight + cardSpacing + cardWidth;
                const float windowMaxX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

                if (nextRight <= windowMaxX)
                {
                    ImGui::SameLine(0.0f, cardSpacing);
                }
            }
            continue;
        }

        const bool isItemHovered = ImGui::IsItemHovered();
        bool isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        const bool isRightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
        const bool isDoubleClicked = isItemHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        if (s_IsBoxSelecting)
            isClicked = false;

        ImTextureID thumb = 0;
        if (!item.isDirectory && IsSupportedThumbnailAsset(item.path))
        {
            thumb = ThumbnailCache::Get().GetThumbnail(item.path);
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            s_DeselectOthersOnMouseRelease = false;
            auto pathStr = item.path.string();
            ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
            ImTextureID dragIcon = (thumb != 0) ? thumb : item.icon;
            if (dragIcon)
            {
                ImGui::Image(dragIcon, ImVec2(20.0f, 20.0f));
                ImGui::SameLine();
            }
            if (s_SelectedPaths.size() > 1 && IsSelected(item.path))
            {
                std::string dragLabel = item.filename + " (+" + std::to_string(s_SelectedPaths.size() - 1) + " items)";
                ImGui::TextUnformatted(dragLabel.c_str());
            }
            else
            {
                ImGui::TextUnformatted(item.filename.c_str());
            }
            ImGui::EndDragDropSource();
        }

        if (item.isDirectory)
        {
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    std::filesystem::path srcPath((const char*)payload->Data);
                    MoveAssetToDirectory(srcPath, item.path);
                }
                ImGui::EndDragDropTarget();
            }
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        if (isSelected)
        {
            ImVec4 acc = GetAccentColor();
            ImU32 fillCol = ImGui::ColorConvertFloat4ToU32(ImVec4(acc.x, acc.y, acc.z, 0.28f));
            ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(ImVec4(acc.x, acc.y, acc.z, 0.90f));
            drawList->AddRectFilled(cellPos, ImVec2(cellPos.x + cardWidth, cellPos.y + cardHeight),
                fillCol, 4.0f);
            drawList->AddRect(cellPos, ImVec2(cellPos.x + cardWidth, cellPos.y + cardHeight),
                borderCol, 4.0f, 0, 1.5f);
        }
        else if (isItemHovered)
        {
            drawList->AddRectFilled(cellPos, ImVec2(cellPos.x + cardWidth, cellPos.y + cardHeight),
                IM_COL32(255, 255, 255, 25), 4.0f);
        }

        // Draw Icon Centered
        const float iconPosX = cellPos.x + (cardWidth - thumbnailSize) * 0.5f;
        const float iconPosY = cellPos.y + padTop;
        if (thumb != 0)
        {
            drawList->AddRectFilled(ImVec2(iconPosX, iconPosY), ImVec2(iconPosX + thumbnailSize, iconPosY + thumbnailSize),
                IM_COL32(20, 20, 24, 200), 4.0f);
            drawList->AddImage(thumb, ImVec2(iconPosX, iconPosY), ImVec2(iconPosX + thumbnailSize, iconPosY + thumbnailSize));
            drawList->AddRect(ImVec2(iconPosX, iconPosY), ImVec2(iconPosX + thumbnailSize, iconPosY + thumbnailSize),
                IM_COL32(255, 255, 255, 30), 4.0f);
        }
        else if (item.icon)
        {
            drawList->AddImage(item.icon, ImVec2(iconPosX, iconPosY), ImVec2(iconPosX + thumbnailSize, iconPosY + thumbnailSize));
        }

        // Rename or File Label
        const float labelY = cellPos.y + padTop + thumbnailSize + padLabel;
        if (s_RenamingActive && s_RenamingPath == item.path)
        {
            ImGui::SetCursorScreenPos(ImVec2(cellPos.x + 2.0f, labelY));
            ImGui::SetNextItemWidth(cardWidth - 4.0f);
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##renameInput", s_RenameBuffer, sizeof(s_RenameBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                RenamePathWithMeta(s_RenamingPath, std::string(s_RenameBuffer) + s_RenameExtension);
                s_RenamingActive = false;
                s_RenameJustCommitted = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                s_RenamingActive = false;
                SetSingleSelection(s_RenamingPath);
            }
        }
        else
        {
            const float maxTextWidth = cardWidth - 4.0f;
            auto [line1, line2] = SplitFileNameTwoLines(item.filename, maxTextWidth);

            float text1Width = ImGui::CalcTextSize(line1.c_str()).x;
            float text1X = std::max(cellPos.x + 2.0f, cellPos.x + (cardWidth - text1Width) * 0.5f);

            if (line2.empty())
            {
                drawList->AddText(ImVec2(text1X, labelY + lineHeight * 0.35f), IM_COL32(230, 230, 235, 255), line1.c_str());
            }
            else
            {
                drawList->AddText(ImVec2(text1X, labelY), IM_COL32(230, 230, 235, 255), line1.c_str());

                float text2Width = ImGui::CalcTextSize(line2.c_str()).x;
                float text2X = std::max(cellPos.x + 2.0f, cellPos.x + (cardWidth - text2Width) * 0.5f);
                drawList->AddText(ImVec2(text2X, labelY + lineHeight), IM_COL32(195, 195, 200, 255), line2.c_str());
            }
        }

        if (item.subAssetCount > 0)
        {
            std::string badgeText = std::to_string(item.subAssetCount);
            ImVec2 badgeTextSize = ImGui::CalcTextSize(badgeText.c_str());
            float badgeW = badgeTextSize.x + 8.0f;
            float badgeH = 16.0f;
            float badgeX = cellPos.x + cardWidth - badgeW - 3.0f;
            float badgeY = cellPos.y + 3.0f;
            ImVec4 acc = GetAccentColor();
            ImU32 badgeCol = ImGui::ColorConvertFloat4ToU32(ImVec4(acc.x, acc.y, acc.z, 0.85f));
            drawList->AddRectFilled(ImVec2(badgeX, badgeY), ImVec2(badgeX + badgeW, badgeY + badgeH), badgeCol, 4.0f);
            drawList->AddText(ImVec2(badgeX + 4.0f, badgeY + 1.0f), IM_COL32(255, 255, 255, 255), badgeText.c_str());
        }

        if (isItemHovered)
        {
            if (item.subAssetCount > 0)
            {
                ImGui::SetTooltip("%s\nModel (%d sub-assets)\nDouble-click to view parts", item.filename.c_str(), item.subAssetCount);
            }
            else if (item.isSubAsset)
            {
                std::string typeName = item.extension.size() > 1 ? item.extension.substr(1) : "Sub-Asset";
                ImGui::SetTooltip("%s\n%s\nDrag to scene or inspector slot", item.filename.c_str(), typeName.c_str());
            }
            else
            {
                ImGui::SetTooltip("%s\n%s", item.filename.c_str(),
                    item.isDirectory ? "Folder" : FormatFileSize(item.fileSize).c_str());
            }
        }

        if (isClicked)
        {
            if (s_RenamingActive && item.path != s_RenamingPath)
            {
                RenamePathWithMeta(s_RenamingPath, std::string(s_RenameBuffer) + s_RenameExtension);
                s_RenamingActive = false;
            }

            if (io.KeyCtrl)
            {
                auto it = std::find(s_SelectedPaths.begin(), s_SelectedPaths.end(), item.path);
                if (it != s_SelectedPaths.end())
                {
                    s_SelectedPaths.erase(it);
                    if (s_SelectedItemPath == item.path)
                    {
                        s_SelectedItemPath = s_SelectedPaths.empty() ? std::filesystem::path() : s_SelectedPaths.back();
                    }
                }
                else
                {
                    s_SelectedPaths.push_back(item.path);
                    s_SelectedItemPath = item.path;
                    s_SelectedIsDir = item.isDirectory;
                    s_SelectedFileSize = item.fileSize;
                }
                s_LastSelectedIndex = (int)i;
                s_DeselectOthersOnMouseRelease = false;
            }
            else if (io.KeyShift)
            {
                if (s_LastSelectedIndex >= 0 && s_LastSelectedIndex < (int)itemsToShow.size())
                {
                    int startIdx = std::min(s_LastSelectedIndex, (int)i);
                    int endIdx = std::max(s_LastSelectedIndex, (int)i);
                    s_SelectedPaths.clear();
                    for (int k = startIdx; k <= endIdx; ++k)
                    {
                        s_SelectedPaths.push_back(itemsToShow[k].path);
                    }
                }
                else
                {
                    s_SelectedPaths.clear();
                    s_SelectedPaths.push_back(item.path);
                    s_LastSelectedIndex = (int)i;
                }
                s_SelectedItemPath = item.path;
                s_SelectedIsDir = item.isDirectory;
                s_SelectedFileSize = item.fileSize;
                s_DeselectOthersOnMouseRelease = false;
                if (!item.isDirectory)
                    ImportSettingsPanel::SetInspectedAsset(item.path);
            }
            else
            {
                if (!IsSelected(item.path))
                {
                    s_SelectedPaths.clear();
                    s_SelectedPaths.push_back(item.path);
                    s_SelectedItemPath = item.path;
                    s_SelectedIsDir = item.isDirectory;
                    s_SelectedFileSize = item.fileSize;
                    s_LastSelectedIndex = (int)i;
                    s_DeselectOthersOnMouseRelease = false;
                    if (!item.isDirectory)
                        ImportSettingsPanel::SetInspectedAsset(item.path);
                }
                else
                {
                    if (s_SelectedPaths.size() > 1)
                    {
                        s_DeselectOthersOnMouseRelease = true;
                        s_DeselectTargetIndex = (int)i;
                    }
                    else
                    {
                        s_LastSelectedIndex = (int)i;
                    }
                }
            }
        }

        if (isRightClicked)
        {
            if (!IsSelected(item.path))
            {
                s_SelectedPaths.clear();
                s_SelectedPaths.push_back(item.path);
                s_SelectedItemPath = item.path;
                s_SelectedIsDir = item.isDirectory;
                s_SelectedFileSize = item.fileSize;
                s_LastSelectedIndex = (int)i;
            }
            else
            {
                s_SelectedItemPath = item.path;
                s_SelectedIsDir = item.isDirectory;
                s_SelectedFileSize = item.fileSize;
            }
            ImGui::OpenPopup(("##ItemMenu_" + item.filename).c_str());
        }
        DrawItemContextMenu(item.path, importPipeline);

        if (isDoubleClicked)
        {
            OpenBrowserItem(item, engine);
        }

        ImGui::EndGroup();
        ImGui::PopID();

        if (i + 1 < itemsToShow.size())
        {
            const float lastRight = ImGui::GetItemRectMax().x;
            const float nextRight = lastRight + cardSpacing + cardWidth;
            const float windowMaxX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

            if (nextRight <= windowMaxX)
            {
                ImGui::SameLine(0.0f, cardSpacing);
            }
        }
    }

    if (isDraggingBox)
    {
        s_SelectedPaths = std::move(currentFrameSelected);
        if (!s_SelectedPaths.empty())
        {
            s_SelectedItemPath = s_SelectedPaths.back();
            auto itLead = std::find_if(itemsToShow.begin(), itemsToShow.end(),
                [](const BrowserItem& bi) { return bi.path == s_SelectedItemPath; });
            if (itLead != itemsToShow.end())
            {
                s_SelectedIsDir = itLead->isDirectory;
                s_SelectedFileSize = itLead->fileSize;
                s_LastSelectedIndex = (int)std::distance(itemsToShow.begin(), itLead);
            }
        }
        else
        {
            s_SelectedItemPath.clear();
            s_SelectedIsDir = false;
            s_SelectedFileSize = 0;
            s_LastSelectedIndex = -1;
        }
    }

    if (s_DeselectOthersOnMouseRelease)
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f))
        {
            s_DeselectOthersOnMouseRelease = false;
            s_DeselectTargetIndex = -1;
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (s_DeselectTargetIndex >= 0 && s_DeselectTargetIndex < (int)itemsToShow.size())
            {
                const auto& tgt = itemsToShow[s_DeselectTargetIndex];
                s_SelectedPaths.clear();
                s_SelectedPaths.push_back(tgt.path);
                s_SelectedItemPath = tgt.path;
                s_SelectedIsDir = tgt.isDirectory;
                s_SelectedFileSize = tgt.fileSize;
                s_LastSelectedIndex = s_DeselectTargetIndex;
            }
            s_DeselectOthersOnMouseRelease = false;
            s_DeselectTargetIndex = -1;
        }
    }

    if (isDraggingBox)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 gridMin = childScreenPos;
        ImVec2 gridMax = ImVec2(childScreenPos.x + ImGui::GetWindowWidth(), childScreenPos.y + windowVisibleHeight);
        ImVec2 clampedMin(std::clamp(boxMin.x, gridMin.x, gridMax.x), std::clamp(boxMin.y, gridMin.y, gridMax.y));
        ImVec2 clampedMax(std::clamp(boxMax.x, gridMin.x, gridMax.x), std::clamp(boxMax.y, gridMin.y, gridMax.y));

        if (clampedMax.x > clampedMin.x && clampedMax.y > clampedMin.y)
        {
            ImVec4 acc = GetAccentColor();
            ImU32 fillCol = ImGui::ColorConvertFloat4ToU32(ImVec4(acc.x, acc.y, acc.z, 0.20f));
            ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(ImVec4(acc.x, acc.y, acc.z, 0.85f));
            dl->AddRectFilled(clampedMin, clampedMax, fillCol);
            dl->AddRect(clampedMin, clampedMax, borderCol, 0.0f, 0, 1.5f);
        }
    }

    if (s_IsBoxSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        s_IsBoxSelecting = false;
    }

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        !ImGui::IsAnyItemHovered())
    {
        ClearSelectionInternal();
        ImGui::OpenPopup("##BrowserBgMenu");
    }
    DrawBackgroundContextMenu(engine, s_CurrentDirectory);

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemHovered())
    {
        s_IsBoxSelecting = true;
        s_BoxSelectStartPos = io.MousePos;
        s_BoxSelectStartScroll = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
        s_BoxSelectStartWindowPos = childScreenPos;
        s_BoxSelectCtrl = io.KeyCtrl;
        s_BoxSelectShift = io.KeyShift;
        s_PreBoxSelectedPaths = (s_BoxSelectCtrl || s_BoxSelectShift) ? s_SelectedPaths : std::vector<std::filesystem::path>{};
        if (!s_BoxSelectCtrl && !s_BoxSelectShift)
        {
            ClearSelectionInternal();
        }
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
    {
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A) && !s_RenamingActive)
        {
            s_SelectedPaths.clear();
            for (const auto& itm : itemsToShow)
            {
                s_SelectedPaths.push_back(itm.path);
            }
            if (!s_SelectedPaths.empty())
            {
                s_SelectedItemPath = s_SelectedPaths.front();
                s_SelectedIsDir = itemsToShow.front().isDirectory;
                s_SelectedFileSize = itemsToShow.front().fileSize;
                s_LastSelectedIndex = 0;
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape) && !s_RenamingActive)
        {
            s_IsBoxSelecting = false;
            ClearSelectionInternal();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F2) && !s_SelectedItemPath.empty() && !s_RenamingActive)
        {
            BeginRenameForNew(s_SelectedItemPath);
        }

        const bool enterPressed = ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
        if (enterPressed && !s_RenamingActive && !s_RenameJustCommitted && !io.WantTextInput && !s_SelectedItemPath.empty())
        {
            auto it = std::find_if(itemsToShow.begin(), itemsToShow.end(),
                [](const BrowserItem& bi) { return bi.path == s_SelectedItemPath; });
            if (it != itemsToShow.end())
            {
                OpenBrowserItem(*it, engine);
            }
            else
            {
                BrowserItem tempItem;
                tempItem.path = s_SelectedItemPath;
                tempItem.filename = s_SelectedItemPath.filename().string();
                tempItem.stem = s_SelectedItemPath.stem().string();
                tempItem.extension = s_SelectedItemPath.extension().string();
                std::error_code ec;
                tempItem.isDirectory = std::filesystem::is_directory(s_SelectedItemPath, ec);
                OpenBrowserItem(tempItem, engine);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !s_SelectedPaths.empty() && !s_RenamingActive)
        {
            DeleteSelectedPathsWithMeta();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && !s_RenamingActive)
        {
            NavigateBack();
        }

        if ((ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt)) && ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
        {
            NavigateBack();
        }

        if ((ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt)) && ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        {
            NavigateForward();
        }

        if ((ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt)) && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            NavigateUp(rootDirectory);
        }
    }

    ImGui::EndChild();
    ThumbnailCache::Get().EndFrame();

    ImGui::Separator();
    {
        ImGui::TextDisabled("%d item%s", (int)itemsToShow.size(), itemsToShow.size() == 1 ? "" : "s");

        if (!s_SelectedPaths.empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            if (s_SelectedPaths.size() == 1)
            {
                std::string selName = s_SelectedPaths[0].filename().string();
                if (s_SelectedIsDir)
                {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s (Folder)", selName.c_str());
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s (%s)", selName.c_str(), FormatFileSize(s_SelectedFileSize).c_str());
                }
            }
            else
            {
                uintmax_t totalSize = 0;
                for (const auto& p : s_SelectedPaths)
                {
                    std::error_code ec;
                    if (!std::filesystem::is_directory(p, ec))
                    {
                        totalSize += std::filesystem::file_size(p, ec);
                    }
                }

                std::string info = std::to_string(s_SelectedPaths.size()) + " items selected";
                if (totalSize > 0)
                    info += " (" + FormatFileSize(totalSize) + ")";
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", info.c_str());
            }
        }
    }

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

    if (!toImport.empty())
        SetSingleSelection(toImport.back());

    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}

void AssetBrowserPanel::RequestRename(const std::filesystem::path& path)
{
    if (path.has_parent_path())
        NavigateTo(path.parent_path());
    s_SelectedPaths.clear();
    s_SelectedPaths.push_back(path);
    s_SelectedItemPath = path;
    BeginRenameForNew(path);
}

void AssetBrowserPanel::RequestRefresh()
{
    ThumbnailCache::Get().OnDirectoryChanged();
    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}
