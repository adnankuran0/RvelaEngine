#include "ImportSettingsPanel.h"
#include "AssetImporters/TextureImporter.h"
#include "AssetImporters/ModelImporter.h"
#include "Asset/AssetImportPipeline.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetRegistry.h"
#include "Asset/AssetMeta.h"
#include "AssetBrowserPanel.h"
#include "Render/IconLibrary.h"
#include "Core/Log.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>

using namespace rv;
using json = nlohmann::json;

static std::filesystem::path s_CurrentAssetPath;
static TextureImportSettings s_CurrentTextureSettings;
static ModelImportSettings   s_CurrentModelSettings;
static bool s_SettingsDirty = false;
static std::string s_StatusMessage;
static float s_StatusMessageTimer = 0.0f;

static bool IsTextureExtension(const std::string& ext)
{
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
           ext == ".tga" || ext == ".bmp" || ext == ".hdr";
}

static bool IsModelExtension(const std::string& ext)
{
    return ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb";
}

static void FormatFileSize(uintmax_t bytes, char* outBuf, size_t bufSize)
{
    if (bytes < 1024)
        snprintf(outBuf, bufSize, "%llu B", (unsigned long long)bytes);
    else if (bytes < 1024 * 1024)
        snprintf(outBuf, bufSize, "%.1f KB", (double)bytes / 1024.0);
    else
        snprintf(outBuf, bufSize, "%.2f MB", (double)bytes / (1024.0 * 1024.0));
}

void ImportSettingsPanel::SetInspectedAsset(const std::filesystem::path& assetPath)
{
    if (s_CurrentAssetPath == assetPath)
        return;

    s_CurrentAssetPath = assetPath;
    s_SettingsDirty = false;
    s_StatusMessage.clear();

    if (s_CurrentAssetPath.empty() || !std::filesystem::exists(s_CurrentAssetPath))
        return;

    std::string ext = s_CurrentAssetPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto& registry = AssetManager::Get().GetRegistry();
    AssetMeta meta = registry.GetOrCreateMeta(s_CurrentAssetPath);

    if (IsTextureExtension(ext))
    {
        s_CurrentTextureSettings = TextureImporter::ParseSettings(meta.importerSettingsJson);
    }
    else if (IsModelExtension(ext))
    {
        s_CurrentModelSettings = ModelImporter::ParseSettings(meta.importerSettingsJson);
    }
}

const std::filesystem::path& ImportSettingsPanel::GetInspectedAsset()
{
    return s_CurrentAssetPath;
}

void ImportSettingsPanel::Draw(AssetImportPipeline& importPipeline)
{
    if (!m_IsOpen)
        return;

    ImGui::Begin("Import", &m_IsOpen);

    if (s_CurrentAssetPath.empty() || !std::filesystem::exists(s_CurrentAssetPath))
    {
        auto selected = AssetBrowserPanel::GetSelectedPrimaryPath();
        if (!selected.empty() && std::filesystem::exists(selected) && !std::filesystem::is_directory(selected))
        {
            std::string ext = selected.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (IsTextureExtension(ext) || IsModelExtension(ext))
            {
                SetInspectedAsset(selected);
            }
        }
    }

    if (s_CurrentAssetPath.empty() || !std::filesystem::exists(s_CurrentAssetPath))
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y * 0.35f));
        const char* msg = "Select an asset in the Asset Browser to edit import settings.";
        float textWidth = ImGui::CalcTextSize(msg).x;
        if (textWidth < size.x)
            ImGui::SetCursorPosX((size.x - textWidth) * 0.5f);
        ImGui::TextDisabled("%s", msg);

        ImGui::End();
        return;
    }

    DrawAssetSettingsUI(s_CurrentAssetPath, importPipeline, false);

    ImGui::End();
}

bool ImportSettingsPanel::DrawAssetSettingsUI(
    const std::filesystem::path& assetPath,
    AssetImportPipeline& importPipeline,
    bool isEmbedded)
{
    if (assetPath != s_CurrentAssetPath)
    {
        SetInspectedAsset(assetPath);
    }

    std::string ext = assetPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    bool isTexture = IsTextureExtension(ext);
    bool isModel = IsModelExtension(ext);

    if (!isTexture && !isModel)
    {
        ImGui::TextDisabled("Import settings not available for this file type: %s", ext.c_str());
        return false;
    }

    ImGui::PushID(assetPath.string().c_str());

    ImTextureID iconId = 0;
    if (isTexture)
        iconId = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Texture).GetID();
    else if (isModel)
        iconId = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Mesh).GetID();

    float headerIconSize = 36.0f;
    if (iconId)
    {
        ImGui::Image(iconId, ImVec2(headerIconSize, headerIconSize), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::SameLine();
    }

    ImGui::BeginGroup();
    std::string filename = assetPath.filename().string();
    ImGui::TextUnformatted(filename.c_str());

    std::error_code ec;
    uintmax_t fileSize = std::filesystem::file_size(assetPath, ec);
    char sizeBuf[32];
    FormatFileSize(fileSize, sizeBuf, sizeof(sizeBuf));

    const char* typeBadge = isTexture ? "Texture" : "Model";
    ImGui::TextDisabled("%s  |  %s  |  %s", typeBadge, ext.c_str(), sizeBuf);
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    bool didReimport = false;

    if (isTexture)
    {
        auto& s = s_CurrentTextureSettings;

        const char* presets[] = {
            "Custom",
            "Default",
            "Normal Map (DirectX)",
            "Normal Map (OpenGL)",
            "Roughness / Metallic / Mask",
            "Uncompressed",
            "Sprite"
        };
        static int currentPreset = 0;
        ImGui::TextUnformatted("Preset:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##TexturePreset", &currentPreset, presets, IM_ARRAYSIZE(presets)))
        {
            s_SettingsDirty = true;
            switch (currentPreset)
            {
            case 1: // Default
                s.textureType = TextureImportType::Color;
                s.sRGB = true;
                s.generateMips = true;
                s.compress = true;
                s.compressionFormat = TextureCompressionFormat::Auto;
                s.flipY = true;
                s.invertGreenChannel = false;
                s.normalizeNormals = true;
                break;
            case 2: // Normal Map DirectX
                s.textureType = TextureImportType::NormalMap;
                s.sRGB = false;
                s.generateMips = true;
                s.compress = true;
                s.compressionFormat = TextureCompressionFormat::BC5;
                s.flipY = true;
                s.invertGreenChannel = false;
                s.normalizeNormals = true;
                break;
            case 3: // Normal Map OpenGL
                s.textureType = TextureImportType::NormalMap;
                s.sRGB = false;
                s.generateMips = true;
                s.compress = true;
                s.compressionFormat = TextureCompressionFormat::BC5;
                s.flipY = true;
                s.invertGreenChannel = true;
                s.normalizeNormals = true;
                break;
            case 4: // Mask / Roughness
                s.textureType = TextureImportType::GrayscaleMask;
                s.sRGB = false;
                s.generateMips = true;
                s.compress = true;
                s.compressionFormat = TextureCompressionFormat::BC4;
                s.flipY = true;
                s.invertGreenChannel = false;
                break;
            case 5: // Uncompressed
                s.textureType = TextureImportType::Color;
                s.sRGB = true;
                s.generateMips = true;
                s.compress = false;
                s.compressionFormat = TextureCompressionFormat::Uncompressed;
                s.flipY = true;
                s.invertGreenChannel = false;
                break;
            case 6: // Sprite
                s.textureType = TextureImportType::Color;
                s.sRGB = true;
                s.generateMips = false;
                s.compress = false;
                s.compressionFormat = TextureCompressionFormat::Uncompressed;
                s.flipY = true;
                s.invertGreenChannel = false;
                break;
            default:
                break;
            }
        }

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Color Space", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char* typeItems[] = { "Auto", "Albedo", "Normal Map", "Grayscale", "HDR" };
            int typeIdx = static_cast<int>(s.textureType);
            ImGui::TextUnformatted("Import As:");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::Combo("##TextureType", &typeIdx, typeItems, IM_ARRAYSIZE(typeItems)))
            {
                s.textureType = static_cast<TextureImportType>(typeIdx);
                if (s.textureType == TextureImportType::Color) s.sRGB = true;
                else if (s.textureType == TextureImportType::NormalMap || s.textureType == TextureImportType::GrayscaleMask) s.sRGB = false;
                s_SettingsDirty = true;
                currentPreset = 0;
            }

            if (ImGui::Checkbox("sRGB", &s.sRGB))
            {
                s_SettingsDirty = true;
                currentPreset = 0;
            }
        }

        if (ImGui::CollapsingHeader("Mipmaps", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Generate Mipmaps", &s.generateMips))
            {
                s_SettingsDirty = true;
                currentPreset = 0;
            }

            if (s.generateMips)
            {
                const char* filters[] = { "Linear / Box Filter", "Box Filter" };
                int filterIdx = static_cast<int>(s.mipFilter);
                ImGui::TextUnformatted("Mip Filter:");
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##MipFilter", &filterIdx, filters, IM_ARRAYSIZE(filters)))
                {
                    s.mipFilter = static_cast<TextureMipFilter>(filterIdx);
                    s_SettingsDirty = true;
                    currentPreset = 0;
                }
            }
        }

        if (ImGui::CollapsingHeader("Compression", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Enable VRAM Compression", &s.compress))
            {
                s_SettingsDirty = true;
                currentPreset = 0;
            }

            if (s.compress)
            {
                const char* formats[] = {
                    "Auto",
                    "BC1 / DXT1 (Opaque)",
                    "BC3 / DXT5 (Alpha)",
                    "BC4 (Grayscale)",
                    "BC5 (Normal Map)",
                    "Uncompressed"
                };
                int formatIdx = static_cast<int>(s.compressionFormat);
                ImGui::TextUnformatted("Format:");
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##CompressFormat", &formatIdx, formats, IM_ARRAYSIZE(formats)))
                {
                    s.compressionFormat = static_cast<TextureCompressionFormat>(formatIdx);
                    s_SettingsDirty = true;
                    currentPreset = 0;
                }
            }
        }

        if (ImGui::CollapsingHeader("Processing", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char* sizeItems[] = { "Original", "256", "512", "1024", "2048", "4096", "8192" };
            int currentSizeIdx = 0;
            if (s.maxSize == 256) currentSizeIdx = 1;
            else if (s.maxSize == 512) currentSizeIdx = 2;
            else if (s.maxSize == 1024) currentSizeIdx = 3;
            else if (s.maxSize == 2048) currentSizeIdx = 4;
            else if (s.maxSize == 4096) currentSizeIdx = 5;
            else if (s.maxSize == 8192) currentSizeIdx = 6;

            ImGui::TextUnformatted("Max Resolution:");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::Combo("##MaxSize", &currentSizeIdx, sizeItems, IM_ARRAYSIZE(sizeItems)))
            {
                int sizes[] = { 0, 256, 512, 1024, 2048, 4096, 8192 };
                s.maxSize = sizes[currentSizeIdx];
                s_SettingsDirty = true;
                currentPreset = 0;
            }

            if (ImGui::Checkbox("Flip Vertically", &s.flipY))
            {
                s_SettingsDirty = true;
                currentPreset = 0;
            }

            if (s.textureType == TextureImportType::NormalMap || s.textureType == TextureImportType::Auto)
            {
                if (ImGui::Checkbox("Invert Normal Green Channel (OpenGL <-> DirectX)", &s.invertGreenChannel))
                {
                    s_SettingsDirty = true;
                    currentPreset = 0;
                }

                if (ImGui::Checkbox("Normalize Normals", &s.normalizeNormals))
                {
                    s_SettingsDirty = true;
                    currentPreset = 0;
                }
            }
        }
    }
    else if (isModel)
    {
        auto& s = s_CurrentModelSettings;

        const char* presets[] = {
            "Custom",
            "Default",
            "Static Prop",
            "Animated Rig",
            "Flat Shaded"
        };
        static int currentModelPreset = 0;
        ImGui::TextUnformatted("Preset:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##ModelPreset", &currentModelPreset, presets, IM_ARRAYSIZE(presets)))
        {
            s_SettingsDirty = true;
            switch (currentModelPreset)
            {
            case 1: // Default
                s = ModelImportSettings();
                break;
            case 2: // Static Prop
                s = ModelImportSettings();
                s.importSkeleton = false;
                s.importAnimations = false;
                s.importSkeletalMeshes = false;
                break;
            case 3: // Animated Rig
                s = ModelImportSettings();
                s.importSkeleton = true;
                s.importAnimations = true;
                s.importSkeletalMeshes = true;
                break;
            case 4: // Flat shaded
                s = ModelImportSettings();
                s.normalMode = ModelNormalMode::GenerateFlat;
                s.importTangents = false;
                break;
            default:
                break;
            }
        }

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextUnformatted("Scale Factor:");

            float btnSize = ImGui::GetFrameHeight();
            if (ImGui::Button("R##ResetScale", ImVec2(btnSize, btnSize)))
            {
                s.scale = 1.0f;
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset Scale to 1.0");

            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragFloat("##Scale", &s.scale, 0.01f, 0.0001f, 1000.0f, "%.4f"))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Preserve FBX Pivots", &s.preservePivots))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }
        }

        if (ImGui::CollapsingHeader("Geometry", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Import Normals", &s.importNormals))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            const char* normalModes[] = { "Keep Existing Normals", "Generate Smooth Normals", "Generate Flat Normals" };
            int modeIdx = static_cast<int>(s.normalMode);
            ImGui::TextUnformatted("Normal Generation:");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::Combo("##NormalMode", &modeIdx, normalModes, IM_ARRAYSIZE(normalModes)))
            {
                s.normalMode = static_cast<ModelNormalMode>(modeIdx);
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Calculate Tangents & Bitangents", &s.importTangents))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Import UVs", &s.importUVs))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Flip UVs", &s.flipUVs))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Optimize Mesh", &s.optimizeMesh))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Join Identical Vertices", &s.joinIdenticalVertices))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Remove Degenerate Triangles", &s.removeDegenerateTriangles))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Triangulate Polygons", &s.triangulate))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Generate Bounding Boxes", &s.generateBoundingBoxes))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }
        }

        if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Import Skeleton", &s.importSkeleton))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Import Animations", &s.importAnimations))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Import Skeletal Meshes", &s.importSkeletalMeshes))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }
        }

        if (ImGui::CollapsingHeader("Materials & Textures", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Import Materials", &s.importMaterials))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }

            if (ImGui::Checkbox("Import Embedded Textures", &s.importTextures))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }
        }

        if (ImGui::CollapsingHeader("Prefab", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Generate Prefab", &s.generatePrefab))
            {
                s_SettingsDirty = true;
                currentModelPreset = 0;
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float btnWidth = 110.0f;
    float btnHeight = 28.0f;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.55f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.65f, 0.42f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.48f, 0.30f, 1.0f));

    if (ImGui::Button("Reimport", ImVec2(btnWidth, btnHeight)))
    {
        auto& registry = AssetManager::Get().GetRegistry();
        AssetMeta meta = registry.GetOrCreateMeta(assetPath);

        if (isTexture)
        {
            json j;
            j["textureType"] = static_cast<int>(s_CurrentTextureSettings.textureType);
            j["sRGB"] = s_CurrentTextureSettings.sRGB;
            j["generateMips"] = s_CurrentTextureSettings.generateMips;
            j["mipFilter"] = static_cast<int>(s_CurrentTextureSettings.mipFilter);
            j["maxSize"] = s_CurrentTextureSettings.maxSize;
            j["compress"] = s_CurrentTextureSettings.compress;
            j["compressionFormat"] = static_cast<int>(s_CurrentTextureSettings.compressionFormat);
            j["flipY"] = s_CurrentTextureSettings.flipY;
            j["invertGreenChannel"] = s_CurrentTextureSettings.invertGreenChannel;
            j["normalizeNormals"] = s_CurrentTextureSettings.normalizeNormals;
            meta.importerSettingsJson = j.dump(4);
            meta.importerID = "TextureImporter";
        }
        else if (isModel)
        {
            json j;
            j["scale"] = s_CurrentModelSettings.scale;
            j["preservePivots"] = s_CurrentModelSettings.preservePivots;
            j["importNormals"] = s_CurrentModelSettings.importNormals;
            j["normalMode"] = static_cast<int>(s_CurrentModelSettings.normalMode);
            j["importTangents"] = s_CurrentModelSettings.importTangents;
            j["importUVs"] = s_CurrentModelSettings.importUVs;
            j["flipUVs"] = s_CurrentModelSettings.flipUVs;
            j["optimizeMesh"] = s_CurrentModelSettings.optimizeMesh;
            j["joinIdenticalVertices"] = s_CurrentModelSettings.joinIdenticalVertices;
            j["removeDegenerateTriangles"] = s_CurrentModelSettings.removeDegenerateTriangles;
            j["triangulate"] = s_CurrentModelSettings.triangulate;
            j["generateBoundingBoxes"] = s_CurrentModelSettings.generateBoundingBoxes;
            j["importSkeleton"] = s_CurrentModelSettings.importSkeleton;
            j["importAnimations"] = s_CurrentModelSettings.importAnimations;
            j["importSkeletalMeshes"] = s_CurrentModelSettings.importSkeletalMeshes;
            j["importTextures"] = s_CurrentModelSettings.importTextures;
            j["importMaterials"] = s_CurrentModelSettings.importMaterials;
            j["generatePrefab"] = s_CurrentModelSettings.generatePrefab;
            meta.importerSettingsJson = j.dump(4);
            meta.importerID = "ModelImporter";
        }

        registry.SaveMeta(assetPath, meta);
        importPipeline.Reimport(assetPath, registry);
        s_SettingsDirty = false;
        s_StatusMessage = "Reimported successfully!";
        s_StatusMessageTimer = 3.0f;
        didReimport = true;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::Button("Reset to Defaults", ImVec2(130.0f, btnHeight)))
    {
        if (isTexture)
            s_CurrentTextureSettings = TextureImportSettings();
        else if (isModel)
            s_CurrentModelSettings = ModelImportSettings();
        s_SettingsDirty = true;
    }

    if (s_SettingsDirty)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.95f, 0.70f, 0.25f, 1.0f), "* Unsaved changes");
    }
    else if (!s_StatusMessage.empty())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.40f, 0.85f, 0.50f, 1.0f), "%s", s_StatusMessage.c_str());
    }

    ImGui::PopID();
    return didReimport;
}
