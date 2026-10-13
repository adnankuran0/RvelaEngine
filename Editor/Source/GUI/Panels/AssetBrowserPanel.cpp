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
#include "Renderer/EditorCamera.h"
#include "AssetImporters/MaterialSerializer.h"
#include "AssetImporters/AnimationLibrarySerializer.h"
#include <AssetImporters/TextureImporter.h>
#include <AssetImporters/ModelImporter.h>
#include "Render/IconLibrary.h"
#include "Utils/ProjectManager.h"

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

static std::filesystem::path s_CurrentDirectory;
static std::filesystem::path s_SelectedItemPath;
static bool s_SelectedIsDir = false;
static uintmax_t s_SelectedFileSize = 0;
static std::vector<std::filesystem::path> s_NavHistory;
static int s_NavHistoryIndex = -1;

static char s_SearchBuffer[256] = "";
static std::vector<BrowserItem> s_CachedItems;
static std::vector<BrowserItem> s_SearchItemResults;
static std::vector<DirNode> s_DirectoryTree;

static bool s_NeedsRefresh = true;
static bool s_DirectoryTreeDirty = true;
static bool s_BreadcrumbsDirty = true;
static std::vector<std::pair<std::string, std::filesystem::path>> s_CachedBreadcrumbs;
static float s_ThumbnailSize = 56.0f;

static bool s_RenamingActive = false;
static std::filesystem::path s_RenamingPath;
static char s_RenameBuffer[256] = "";
static std::string s_RenameExtension;

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

    if (s_NavHistoryIndex >= 0 && s_NavHistoryIndex + 1 < (int)s_NavHistory.size())
    {
        s_NavHistory.erase(s_NavHistory.begin() + s_NavHistoryIndex + 1, s_NavHistory.end());
    }

    s_NavHistory.push_back(path);
    s_NavHistoryIndex = (int)s_NavHistory.size() - 1;
    s_CurrentDirectory = path;
    s_SearchBuffer[0] = '\0';
    s_SelectedItemPath.clear();
    s_NeedsRefresh = true;
    s_BreadcrumbsDirty = true;
}

static void NavigateBack()
{
    if (s_NavHistoryIndex > 0)
    {
        s_NavHistoryIndex--;
        s_CurrentDirectory = s_NavHistory[s_NavHistoryIndex];
        s_SearchBuffer[0] = '\0';
        s_SelectedItemPath.clear();
        s_NeedsRefresh = true;
        s_BreadcrumbsDirty = true;
    }
}

static void NavigateForward()
{
    if (s_NavHistoryIndex + 1 < (int)s_NavHistory.size())
    {
        s_NavHistoryIndex++;
        s_CurrentDirectory = s_NavHistory[s_NavHistoryIndex];
        s_SearchBuffer[0] = '\0';
        s_SelectedItemPath.clear();
        s_NeedsRefresh = true;
        s_BreadcrumbsDirty = true;
    }
}

static void NavigateUp(const std::filesystem::path& rootDirectory)
{
    if (s_CurrentDirectory != rootDirectory && s_CurrentDirectory.has_parent_path())
    {
        NavigateTo(s_CurrentDirectory.parent_path());
    }
}

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

    auto& reg = AssetManager::Get().GetRegistry();
    reg.Scan(reg.GetAssetDir());
    EditorUtils::ClearAssetFileNameCache();

    if (s_SelectedItemPath == path)
        s_SelectedItemPath.clear();

    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}

static bool MoveAssetToDirectory(const std::filesystem::path& sourcePath, const std::filesystem::path& targetDir)
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

    if (s_SelectedItemPath == sourcePath)
        s_SelectedItemPath = destPath;

    auto& reg = AssetManager::Get().GetRegistry();
    reg.Scan(reg.GetAssetDir());
    EditorUtils::ClearAssetFileNameCache();

    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
    return true;
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
    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}

static void RenamePathWithMeta(const std::filesystem::path& oldPath, const std::string& newName)
{
    if (newName.empty()) return;

    auto newPath = oldPath.parent_path() / newName;
    if (newPath == oldPath) return;

    if (std::filesystem::exists(newPath))
    {
        LOG_WARN("Rename skipped, already exists: {}", newName);
        return;
    }

    std::error_code ec;
    std::filesystem::rename(oldPath, newPath, ec);
    if (ec) { LOG_ERROR("Rename failed: {}", ec.message()); return; }

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

    if (s_SelectedItemPath == oldPath)
        s_SelectedItemPath = newPath;

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
    if (extension == ".lua" || extension == ".glsl" || extension == ".rshader")
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
        ImGui::TextDisabled("%s", itemPath.filename().string().c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Rename", "F2"))
        {
            BeginRenameForNew(itemPath);
        }

        if (ImGui::MenuItem("Delete", "Del"))
        {
            DeletePathWithMeta(itemPath);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Copy Path"))
            ImGui::SetClipboardText(itemPath.string().c_str());

        if (ImGui::MenuItem("Copy Relative Path"))
        {
            std::error_code ec;
            auto rel = std::filesystem::relative(itemPath, ProjectManager::GetAssetDirectory(), ec);
            if (!ec) ImGui::SetClipboardText(rel.string().c_str());
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

            if (ImGui::MenuItem("Reimport"))
                HandleReimport(itemPath, importPipeline);
        }

        ImGui::EndPopup();
    }
}

void AssetBrowserPanel::Draw(Engine* engine, const std::filesystem::path& rootDirectory, AssetImportPipeline& importPipeline)
{
    if (s_CurrentDirectory.empty() || !std::filesystem::exists(s_CurrentDirectory))
    {
        s_CurrentDirectory = rootDirectory;
        s_NavHistory.clear();
        s_NavHistory.push_back(rootDirectory);
        s_NavHistoryIndex = 0;
        s_NeedsRefresh = true;
        s_DirectoryTreeDirty = true;
    }

    if (s_DirectoryTreeDirty)
    {
        s_DirectoryTree.clear();
        BuildDirectoryTreeRecursive(rootDirectory, s_DirectoryTree);
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
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.35f, 0.52f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.95f, 1.0f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.78f, 0.85f, 1.0f));
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

    ImGui::BeginChild("FilesGridArea", ImVec2(0.0f, -24.0f), false);

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::GetIO().KeyCtrl)
    {
        float wheel = ImGui::GetIO().MouseWheel;
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

    for (size_t i = 0; i < itemsToShow.size(); ++i)
    {
        const auto& item = itemsToShow[i];
        ImGui::PushID(item.filename.c_str());

        ImGui::BeginGroup();

        const ImVec2 cellPos = ImGui::GetCursorScreenPos();
        const float itemRelativeY = cellPos.y - childScreenPos.y + scrollY;
        const bool isVisible = (itemRelativeY >= clipMinY && itemRelativeY <= clipMaxY);

        const bool isSelected = (s_SelectedItemPath == item.path);

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
        const bool isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        const bool isRightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
        const bool isDoubleClicked = isItemHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            auto pathStr = item.path.string();
            ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
            if (item.icon)
            {
                ImGui::Image(item.icon, ImVec2(20.0f, 20.0f));
                ImGui::SameLine();
            }
            ImGui::TextUnformatted(item.filename.c_str());
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
            drawList->AddRectFilled(cellPos, ImVec2(cellPos.x + cardWidth, cellPos.y + cardHeight),
                IM_COL32(66, 133, 244, 80), 4.0f);
            drawList->AddRect(cellPos, ImVec2(cellPos.x + cardWidth, cellPos.y + cardHeight),
                IM_COL32(90, 155, 255, 230), 4.0f, 0, 1.5f);
        }
        else if (isItemHovered)
        {
            drawList->AddRectFilled(cellPos, ImVec2(cellPos.x + cardWidth, cellPos.y + cardHeight),
                IM_COL32(255, 255, 255, 25), 4.0f);
        }

        // Draw Icon Centered
        const float iconPosX = cellPos.x + (cardWidth - thumbnailSize) * 0.5f;
        const float iconPosY = cellPos.y + padTop;
        if (item.icon)
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
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                s_RenamingActive = false;
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
            drawList->AddRectFilled(ImVec2(badgeX, badgeY), ImVec2(badgeX + badgeW, badgeY + badgeH), IM_COL32(30, 110, 220, 220), 4.0f);
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
            s_SelectedItemPath = item.path;
            s_SelectedIsDir = item.isDirectory;
            s_SelectedFileSize = item.fileSize;
        }

        if (isRightClicked)
        {
            s_SelectedItemPath = item.path;
            s_SelectedIsDir = item.isDirectory;
            s_SelectedFileSize = item.fileSize;
            ImGui::OpenPopup(("##ItemMenu_" + item.filename).c_str());
        }
        DrawItemContextMenu(item.path, importPipeline);

        if (isDoubleClicked)
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
                if (uuid.IsValid())
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
                engine->GetSceneManager().LoadScene(item.path.string());
            }
            else if (item.extension == ".rprefab")
            {
                AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(item.path.string());
                if (uuid.IsValid())
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

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        !ImGui::IsAnyItemHovered())
    {
        ImGui::OpenPopup("##BrowserBgMenu");
    }
    DrawBackgroundContextMenu(engine, s_CurrentDirectory);

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemHovered())
    {
        s_SelectedItemPath.clear();
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_F2) && !s_SelectedItemPath.empty() && !s_RenamingActive)
        {
            BeginRenameForNew(s_SelectedItemPath);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !s_SelectedItemPath.empty() && !s_RenamingActive)
        {
            DeletePathWithMeta(s_SelectedItemPath);
            s_SelectedItemPath.clear();
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

    ImGui::Separator();
    {
        ImGui::TextDisabled("%d item%s", (int)itemsToShow.size(), itemsToShow.size() == 1 ? "" : "s");

        if (!s_SelectedItemPath.empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            std::string selName = s_SelectedItemPath.filename().string();
            if (s_SelectedIsDir)
            {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s (Folder)", selName.c_str());
            }
            else
            {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s (%s)", selName.c_str(), FormatFileSize(s_SelectedFileSize).c_str());
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

    s_NeedsRefresh = true;
    s_DirectoryTreeDirty = true;
}

void AssetBrowserPanel::RequestRename(const std::filesystem::path& path)
{
    if (path.has_parent_path())
        NavigateTo(path.parent_path());
    s_SelectedItemPath = path;
    BeginRenameForNew(path);
}
