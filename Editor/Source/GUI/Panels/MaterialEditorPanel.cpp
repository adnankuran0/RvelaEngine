#include "MaterialEditorPanel.h"
#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Scene/Components/MaterialComponent.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetRegistry.h"
#include "Asset/Types/ShaderAsset.h"
#include "Asset/Types/TextureAsset.h"
#include "AssetImporters/MaterialSerializer.h"
#include "GUI/Panels/Inspector/InspectorUIUtils.h"
#include "GUI/Panels/ThumbnailCache.h"
#include "GUI/Panels/AssetBrowserPanel.h"
#include "EditorUtils.h"
#include "ImGui/imgui.h"
#include <array>
#include <algorithm>

namespace rv {

MaterialEditorPanel* MaterialEditorPanel::s_Instance = nullptr;

MaterialEditorPanel::MaterialEditorPanel()
    : EditorPanel("Material Editor", true)
{
    s_Instance = this;
}

void MaterialEditorPanel::OpenMaterial(AssetUUID uuid)
{
    if (s_Instance)
    {
        s_Instance->LoadMaterial(uuid);
        s_Instance->Open();
    }
}

void MaterialEditorPanel::OpenMaterial(const std::filesystem::path& path)
{
    AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(path.string());
    if (uuid.IsValid())
    {
        OpenMaterial(uuid);
    }
}

void MaterialEditorPanel::SetMaterial(AssetUUID uuid)
{
    if (s_Instance)
    {
        s_Instance->LoadMaterial(uuid);
    }
}

AssetUUID MaterialEditorPanel::GetCurrentMaterialUUID()
{
    return s_Instance ? s_Instance->m_CurrentMaterialUUID : AssetUUID::Invalid();
}

void MaterialEditorPanel::Reset()
{
    if (s_Instance)
    {
        s_Instance->m_CurrentMaterialUUID = AssetUUID::Invalid();
        s_Instance->m_CurrentMaterial = nullptr;
        s_Instance->m_CurrentMaterialPath.clear();
        s_Instance->m_IsDirty = false;
    }
}

void MaterialEditorPanel::LoadMaterial(AssetUUID uuid)
{
    if (!uuid.IsValid())
    {
        m_CurrentMaterialUUID = AssetUUID::Invalid();
        m_CurrentMaterial = nullptr;
        m_CurrentMaterialPath.clear();
        m_IsDirty = false;
        return;
    }

    if (m_CurrentMaterialUUID == uuid && m_CurrentMaterial)
        return;

    m_CurrentMaterialUUID = uuid;
    m_CurrentMaterialPath = AssetManager::Get().GetRegistry().GetPath(uuid);
    m_CurrentMaterial = AssetManager::Get().GetAsset<MaterialAsset>(uuid);
    m_IsDirty = false;
}

bool MaterialEditorPanel::SaveCurrentMaterial()
{
    if (!m_CurrentMaterial || m_CurrentMaterialPath.empty())
        return false;

    bool success = MaterialSerializer::Save(m_CurrentMaterial, m_CurrentMaterialPath);
    if (success)
    {
        m_IsDirty = false;
    }
    return success;
}

void MaterialEditorPanel::RevertCurrentMaterial()
{
    if (!m_CurrentMaterial || m_CurrentMaterialPath.empty())
        return;

    MaterialSerializer::Load(m_CurrentMaterial, m_CurrentMaterialPath);
    m_IsDirty = false;
}

void MaterialEditorPanel::MarkDirty(Engine* engine)
{
    m_IsDirty = true;

    if (m_AutoSave)
    {
        SaveCurrentMaterial();
    }

    if (engine && m_CurrentMaterialUUID.IsValid())
    {
        auto& reg = engine->GetActiveScene().GetRegistry();
        auto view = reg.view<MaterialComponent>();
        for (auto e : view)
        {
            auto& mc = view.get<MaterialComponent>(e);
            if (mc.GetMaterialUUID() == m_CurrentMaterialUUID)
            {
                mc.GetInstance().RebuildCache();
            }
        }
    }
}

void MaterialEditorPanel::Draw(Engine* engine)
{
    if (!m_IsOpen)
        return;

    if (!m_CurrentMaterial || m_CurrentMaterialPath.empty() || !std::filesystem::exists(m_CurrentMaterialPath))
    {
        auto selected = AssetBrowserPanel::GetSelectedPrimaryPath();
        if (!selected.empty() && std::filesystem::exists(selected) && !std::filesystem::is_directory(selected))
        {
            if (selected.extension() == ".rmat")
            {
                AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(selected.string());
                if (uuid.IsValid())
                {
                    LoadMaterial(uuid);
                }
            }
        }
    }

    std::string title = "Material Editor";
    title += "###MaterialEditor";

    ImGui::SetNextWindowSize(ImVec2(420.0f, 600.0f), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_MenuBar))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
        {
            std::string pathStr((const char*)payload->Data);
            if (pathStr.ends_with(".rmat"))
            {
                AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                if (uuid.IsValid())
                {
                    LoadMaterial(uuid);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (!m_CurrentMaterial)
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y * 0.35f));
        const char* msg = "Select a material in the Asset Browser to edit.";
        float textWidth = ImGui::CalcTextSize(msg).x;
        if (textWidth < size.x)
            ImGui::SetCursorPosX((size.x - textWidth) * 0.5f);
        ImGui::TextDisabled("%s", msg);

        ImGui::End();
        return;
    }

    {
        std::string selectorLabel = m_CurrentMaterialPath.stem().string();

        float availWidth = ImGui::GetContentRegionAvail().x;
        float saveBtnWidth = 55.0f;
        float autoSaveWidth = 90.0f;
        float spacing = 4.0f;
        float slotWidth = std::max(120.0f, availWidth - saveBtnWidth - autoSaveWidth - (spacing * 2.0f));

        if (ImGui::Button(selectorLabel.c_str(), ImVec2(slotWidth, 24.0f)))
        {
            ImGui::OpenPopup("SelectMatEditorPopup");
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                std::string pathStr((const char*)payload->Data);
                if (pathStr.ends_with(".rmat"))
                {
                    AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                    if (uuid.IsValid())
                    {
                        LoadMaterial(uuid);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::SameLine(0, spacing);
        ImGui::BeginDisabled(m_CurrentMaterial == nullptr);
        if (ImGui::Button("Save", ImVec2(saveBtnWidth, 24.0f)))
        {
            SaveCurrentMaterial();
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, spacing);
        ImGui::Checkbox("Auto-Save", &m_AutoSave);

        if (ImGui::BeginPopup("SelectMatEditorPopup"))
        {
            static char filterBuf[128] = "";
            ImGui::TextDisabled("Select Material Asset");
            ImGui::Separator();
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
                filterBuf[0] = '\0';
            }
            ImGui::SetNextItemWidth(320.0f);
            ImGui::InputTextWithHint("##MatEditorSearch", "Search material...", filterBuf, sizeof(filterBuf));
            ImGui::Separator();

            auto matList = EditorUtils::GetAvailableMaterialAssets();
            std::string filterStr = filterBuf;
            std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

            ImGui::BeginChild("MatEditorListScroll", ImVec2(320.0f, 240.0f), true);
            for (const auto& item : matList)
            {
                std::string itemLower = item.name + " " + item.path;
                std::transform(itemLower.begin(), itemLower.end(), itemLower.begin(), ::tolower);
                if (!filterStr.empty() && itemLower.find(filterStr) == std::string::npos)
                    continue;

                bool isSelected = (m_CurrentMaterialUUID == item.uuid);
                std::string displayLabel = item.name + " (" + item.path + ")";
                if (ImGui::Selectable(displayLabel.c_str(), isSelected))
                {
                    LoadMaterial(item.uuid);
                    ImGui::CloseCurrentPopup();
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    {
        ImTextureID matThumb = 0;
        if (!m_CurrentMaterialPath.empty())
        {
            matThumb = ThumbnailCache::Get().GetThumbnail(m_CurrentMaterialPath);
        }

        float previewSize = 48.0f;
        float startY = ImGui::GetCursorPosY();

        if (matThumb != 0)
        {
            ImGui::Image(matThumb, ImVec2(previewSize, previewSize));
            ImGui::SameLine(0.0f, 10.0f);
        }

        std::string matName = m_CurrentMaterialPath.stem().string();
        if (matThumb != 0)
        {
            ImGui::SetCursorPosY(startY + (previewSize - ImGui::GetTextLineHeight()) * 0.5f);
        }
        ImGui::TextUnformatted(matName.c_str());
        if (matThumb != 0)
        {
            ImGui::SetCursorPosY(startY + previewSize);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto* asset = m_CurrentMaterial.get();

    auto DrawTextureSlot = [&](const char* label, const char* idSuffix, AssetUUID& texUUID, bool& useMap) {
        UI::PropertyLabel(label);

        std::string texName = "None (Drop Texture)";
        if (texUUID.IsValid())
        {
            auto p = AssetManager::Get().GetRegistry().GetPath(texUUID);
            if (!p.empty()) texName = p.filename().string();
            else texName = "Texture Asset";
        }

        float clearBtnWidth = texUUID.IsValid() ? 24.0f : 0.0f;
        float btnWidth = texUUID.IsValid() ? ImGui::GetContentRegionAvail().x - clearBtnWidth - 4.0f : -FLT_MIN;

        ImTextureID thumb = 0;
        if (texUUID.IsValid())
        {
            auto path = AssetManager::Get().GetRegistry().GetPath(texUUID);
            if (!path.empty())
                thumb = ThumbnailCache::Get().GetThumbnail(path);
        }

        if (thumb != 0)
        {
            ImGui::Image(thumb, ImVec2(22.0f, 22.0f));
            ImGui::SameLine();
        }

        std::string btnId = texName + "##" + idSuffix;
        ImGui::Button(btnId.c_str(), ImVec2(btnWidth, 22.0f));

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                std::string pathStr((const char*)payload->Data);
                static constexpr std::array exts = { ".png", ".jpg", ".jpeg", ".tga", ".hdr", ".rtex" };
                bool isImage = std::any_of(exts.begin(), exts.end(), [&](const char* e) { return pathStr.ends_with(e); });
                if (isImage)
                {
                    AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                    if (uuid.IsValid())
                    {
                        texUUID = uuid;
                        useMap = true;
                        MarkDirty(engine);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (texUUID.IsValid())
        {
            ImGui::SameLine();
            std::string clearId = "X##" + std::string(idSuffix);
            if (ImGui::Button(clearId.c_str(), ImVec2(22.0f, 22.0f)))
            {
                texUUID = AssetUUID::Invalid();
                useMap = false;
                MarkDirty(engine);
            }
        }
    };

    {
        if (UI::BeginPropertyTable("MatShaderTable"))
        {
            UI::PropertyLabel("Shader");
            std::string shaderBtnLabel = "Default (PBR)";
            if (asset->customShaderUUID.IsValid())
            {
                auto sPath = AssetManager::Get().GetRegistry().GetPath(asset->customShaderUUID);
                if (!sPath.empty()) shaderBtnLabel = sPath.filename().string();
                else shaderBtnLabel = "Shader Asset";
            }

            float sClearWidth = asset->customShaderUUID.IsValid() ? 26.0f : 0.0f;
            float sSlotWidth = asset->customShaderUUID.IsValid() ? ImGui::GetContentRegionAvail().x - sClearWidth : -FLT_MIN;

            ImGui::Button(shaderBtnLabel.c_str(), ImVec2(sSlotWidth, 22.0f));
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    std::string pathStr((const char*)payload->Data);
                    if (pathStr.ends_with(".rshader"))
                    {
                        AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                        if (uuid.IsValid())
                        {
                            asset->customShaderUUID = uuid;
                            asset->customUniforms.clear();
                            MarkDirty(engine);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (asset->customShaderUUID.IsValid())
            {
                ImGui::SameLine();
                if (ImGui::Button("X##matshaderclear", ImVec2(sClearWidth - 4.0f, 22.0f)))
                {
                    asset->customShaderUUID = AssetUUID::Invalid();
                    asset->customUniforms.clear();
                    MarkDirty(engine);
                }
            }

            UI::EndPropertyTable();
        }

        if (asset->customShaderUUID.IsValid())
        {
            Ref<ShaderAsset> shaderAsset = AssetManager::Get().GetAsset<ShaderAsset>(asset->customShaderUUID);
            if (shaderAsset)
            {
                ImGui::Spacing();
                ImGui::TextUnformatted("Shader Status:");
                ImGui::SameLine();
                if (shaderAsset->compileSuccess)
                {
                    ImGui::TextColored(shaderAsset->isLit ? ImVec4(0.3f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                        shaderAsset->isLit ? "[Lit - PBR]" : "[Unlit]");
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[Compile Error]");
                }

                ImGui::SameLine();
                if (ImGui::SmallButton("Recompile##matshaderrecompile"))
                {
                    auto sPath = AssetManager::Get().GetRegistry().GetPath(asset->customShaderUUID);
                    shaderAsset->ReloadAndCompile(sPath);
                }

                if (!shaderAsset->compileSuccess && !shaderAsset->compileError.empty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                    ImGui::TextWrapped("Error:\n%s", shaderAsset->compileError.c_str());
                    ImGui::PopStyleColor();
                }

                if (!shaderAsset->uniforms.empty() && ImGui::CollapsingHeader("Shader Parameters##customuniformshdr", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (UI::BeginPropertyTable("MatCustomUniformsTable"))
                    {
                        for (const auto& uDef : shaderAsset->uniforms)
                        {
                            if (uDef.engineTexture != EngineTextureType::None)
                                continue;

                            UI::PropertyLabel(uDef.displayName.c_str());
                            CustomUniformValue uVal;
                            auto it = asset->customUniforms.find(uDef.name);
                            if (it != asset->customUniforms.end())
                            {
                                uVal = it->second;
                            }
                            else
                            {
                                uVal.type = uDef.type;
                                uVal.floatVal = uDef.floatVal;
                                uVal.intVal = uDef.intVal;
                                uVal.boolVal = uDef.boolVal;
                                uVal.vec2Val = uDef.vec2Val;
                                uVal.vec3Val = uDef.vec3Val;
                                uVal.vec4Val = uDef.vec4Val;
                                uVal.textureUUID = uDef.textureUUID;
                            }

                            bool changed = false;
                            std::string uId = "##cu_" + uDef.name;

                            switch (uDef.type)
                            {
                            case ShaderUniformType::Float:
                            {
                                if (uDef.hasRange)
                                {
                                    if (ImGui::SliderFloat(uId.c_str(), &uVal.floatVal, uDef.minVal, uDef.maxVal))
                                        changed = true;
                                }
                                else
                                {
                                    if (ImGui::DragFloat(uId.c_str(), &uVal.floatVal, 0.05f))
                                        changed = true;
                                }
                                break;
                            }
                            case ShaderUniformType::Int:
                            {
                                if (ImGui::DragInt(uId.c_str(), &uVal.intVal))
                                    changed = true;
                                break;
                            }
                            case ShaderUniformType::Bool:
                            {
                                if (ImGui::Checkbox(uId.c_str(), &uVal.boolVal))
                                    changed = true;
                                break;
                            }
                            case ShaderUniformType::Vec2:
                            {
                                if (ImGui::DragFloat2(uId.c_str(), &uVal.vec2Val[0], 0.05f))
                                    changed = true;
                                break;
                            }
                            case ShaderUniformType::Color3:
                            {
                                if (ImGui::ColorEdit3(uId.c_str(), &uVal.vec3Val[0]))
                                    changed = true;
                                break;
                            }
                            case ShaderUniformType::Vec3:
                            {
                                if (ImGui::DragFloat3(uId.c_str(), &uVal.vec3Val[0], 0.05f))
                                    changed = true;
                                break;
                            }
                            case ShaderUniformType::Color4:
                            {
                                if (ImGui::ColorEdit4(uId.c_str(), &uVal.vec4Val[0]))
                                    changed = true;
                                break;
                            }
                            case ShaderUniformType::Vec4:
                            {
                                if (ImGui::DragFloat4(uId.c_str(), &uVal.vec4Val[0], 0.05f))
                                    changed = true;
                                break;
                            }
                            case ShaderUniformType::Texture2D:
                            {
                                std::string texLabel = "Texture";
                                if (uVal.textureUUID.IsValid())
                                {
                                    std::string fn = EditorUtils::GetAssetFileName(uVal.textureUUID);
                                    if (!fn.empty()) texLabel = fn;
                                }
                                ImGui::Button((texLabel + uId).c_str(), ImVec2(-FLT_MIN, 22.0f));
                                if (ImGui::BeginDragDropTarget())
                                {
                                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                                    {
                                        std::string pathStr((const char*)payload->Data);
                                        static constexpr std::array exts = { ".png", ".jpg", ".jpeg", ".tga", ".hdr", ".rtex" };
                                        bool isImage = std::any_of(exts.begin(), exts.end(), [&](const char* e) { return pathStr.ends_with(e); });
                                        if (isImage)
                                        {
                                            AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                                            if (uuid.IsValid())
                                            {
                                                uVal.textureUUID = uuid;
                                                changed = true;
                                            }
                                        }
                                    }
                                    ImGui::EndDragDropTarget();
                                }
                                break;
                            }
                            }

                            if (changed)
                            {
                                asset->customUniforms[uDef.name] = uVal;
                                MarkDirty(engine);
                            }
                        }
                        UI::EndPropertyTable();
                    }
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Transparency##transphdr"))
    {
        if (UI::BeginPropertyTable("TransparencyTable"))
        {
            UI::PropertyLabel("Mode");
            const char* transModes[] = { "Opaque", "Alpha", "Alpha Scissor", "Depth Prepass" };
            int currentTransMode = static_cast<int>(asset->transparencyMode);
            if (ImGui::Combo("##tmode", &currentTransMode, transModes, 4))
            {
                asset->transparencyMode = static_cast<TransparencyMode>(currentTransMode);
                MarkDirty(engine);
            }

            if (asset->transparencyMode == TransparencyMode::AlphaScissor || asset->transparencyMode == TransparencyMode::DepthPrepass)
            {
                UI::PropertyLabel("Alpha Cutoff");
                if (ImGui::SliderFloat("##acutoff", &asset->alphaCutoff, 0.0f, 1.0f, "%.2f"))
                {
                    MarkDirty(engine);
                }
            }

            if (asset->transparencyMode == TransparencyMode::Alpha || asset->transparencyMode == TransparencyMode::DepthPrepass)
            {
                UI::PropertyLabel("Blend Mode");
                const char* blendModes[] = { "Mix", "Add", "Subtract", "Multiply" };
                int currentBlendMode = static_cast<int>(asset->blendMode);
                if (ImGui::Combo("##bmode", &currentBlendMode, blendModes, 4))
                {
                    asset->blendMode = static_cast<BlendMode>(currentBlendMode);
                    MarkDirty(engine);
                }
            }

            UI::PropertyLabel("Cull Mode");
            const char* cullModes[] = { "Back", "Front", "Disabled" };
            int currentCullMode = static_cast<int>(asset->cullMode);
            if (ImGui::Combo("##cmode", &currentCullMode, cullModes, 3))
            {
                asset->cullMode = static_cast<CullMode>(currentCullMode);
                MarkDirty(engine);
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("Shading##shadinghdr"))
    {
        if (UI::BeginPropertyTable("ShadingTable"))
        {
            UI::PropertyLabel("Shading Mode");
            const char* shadingModes[] = { "Lit", "Unshaded" };
            int currentShadingMode = static_cast<int>(asset->shadingMode);
            if (ImGui::Combo("##smode", &currentShadingMode, shadingModes, 2))
            {
                asset->shadingMode = static_cast<ShadingMode>(currentShadingMode);
                MarkDirty(engine);
            }

            if (asset->shadingMode == ShadingMode::Lit)
            {
                UI::PropertyLabel("Receive Shadows");
                if (ImGui::Checkbox("##rshadows", &asset->receiveShadows))
                {
                    MarkDirty(engine);
                }
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("Albedo"))
    {
        if (UI::BeginPropertyTable("AlbedoTable"))
        {
            DrawTextureSlot("Texture", "alb", asset->albedoTextureUUID, asset->useAlbedoMap);

            UI::PropertyLabel("Color");
            float col[4] = { asset->albedoColor.r, asset->albedoColor.g, asset->albedoColor.b, asset->albedoColor.a };
            if (ImGui::ColorEdit4("##albedocol", col))
            {
                asset->albedoColor = glm::vec4(col[0], col[1], col[2], col[3]);
                MarkDirty(engine);
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("Normal"))
    {
        if (UI::BeginPropertyTable("NormalTable"))
        {
            DrawTextureSlot("Texture", "nrm", asset->normalTextureUUID, asset->useNormalMap);

            UI::PropertyLabel("Scale");
            if (ImGui::SliderFloat("##nrmscl", &asset->normalScale, -10.0f, 10.0f, "%.2f"))
            {
                MarkDirty(engine);
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("Roughness##rhdr"))
    {
        if (UI::BeginPropertyTable("RoughnessTable"))
        {
            DrawTextureSlot("Texture", "rgh", asset->roughnessTextureUUID, asset->useRoughnessMap);

            UI::PropertyLabel("Roughness");
            if (ImGui::SliderFloat("##rghval", &asset->roughness, 0.0f, 1.0f, "%.2f"))
            {
                MarkDirty(engine);
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("Metallic##mhdr"))
    {
        if (UI::BeginPropertyTable("MetallicTable"))
        {
            DrawTextureSlot("Texture", "mtl", asset->metallicTextureUUID, asset->useMetallicMap);

            UI::PropertyLabel("Metallic");
            if (ImGui::SliderFloat("##mtlval", &asset->metallic, 0.0f, 1.0f, "%.2f"))
            {
                MarkDirty(engine);
            }

            UI::PropertyLabel("Specular");
            if (ImGui::SliderFloat("##spcval", &asset->specular, 0.0f, 1.0f, "%.2f"))
            {
                MarkDirty(engine);
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("Displacement"))
    {
        if (UI::BeginPropertyTable("HeightTable"))
        {
            DrawTextureSlot("Texture", "hgt", asset->heightTextureUUID, asset->useHeightMap);

            UI::PropertyLabel("Scale");
            if (ImGui::SliderFloat("##hgtscl", &asset->heightScale, 0.0f, 0.5f, "%.3f"))
            {
                MarkDirty(engine);
            }

            UI::PropertyLabel("Invert Height");
            if (ImGui::Checkbox("##invhgt", &asset->invertHeight))
            {
                MarkDirty(engine);
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("AO##aohdr"))
    {
        if (UI::BeginPropertyTable("AOTable"))
        {
            DrawTextureSlot("Texture", "ao", asset->aoTextureUUID, asset->useAOMap);

            UI::PropertyLabel("AO");
            if (ImGui::SliderFloat("##aoval", &asset->ao, 0.0f, 1.0f, "%.2f"))
            {
                MarkDirty(engine);
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("ARM##armhdr"))
    {
        if (UI::BeginPropertyTable("ARMTable"))
        {
            UI::PropertyLabel("Use ARM Map");
            if (ImGui::Checkbox("##usearm", &asset->useARMMap))
            {
                MarkDirty(engine);
            }

            DrawTextureSlot("ARM Map", "arm", asset->armTextureUUID, asset->useARMMap);

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("Emissive"))
    {
        if (UI::BeginPropertyTable("EmissiveTable"))
        {
            UI::PropertyLabel("Color");
            float col[3] = { asset->emissiveColor.r, asset->emissiveColor.g, asset->emissiveColor.b };
            if (ImGui::ColorEdit3("##emcol", col))
            {
                asset->emissiveColor = glm::vec3(col[0], col[1], col[2]);
                MarkDirty(engine);
            }

            UI::PropertyLabel("Intensity");
            if (ImGui::SliderFloat("##emint", &asset->emissiveIntensity, 0.0f, 20.0f, "%.1f"))
            {
                MarkDirty(engine);
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("Sampler"))
    {
        if (UI::BeginPropertyTable("SamplerTable"))
        {
            UI::PropertyLabel("Filter");
            const char* filterOptions[] = {
                "Default",
                "Nearest",
                "Linear",
                "Nearest Mipmap",
                "Linear Mipmap"
            };
            int currentFilter = static_cast<int>(asset->samplerDesc.filter);
            if (ImGui::Combo("##TextureFilter", &currentFilter, filterOptions, IM_ARRAYSIZE(filterOptions)))
            {
                asset->samplerDesc.filter = static_cast<TextureFilter>(currentFilter);
                MarkDirty(engine);
            }

            UI::PropertyLabel("Repeat");
            const char* wrapOptions[] = {
                "Default",
                "Repeat",
                "Mirror Repeat",
                "Clamp to Edge",
                "Clamp to Border",
                "Mirror Clamp to Edge"
            };
            int currentWrap = static_cast<int>(asset->samplerDesc.repeat);
            if (ImGui::Combo("##TextureWrap", &currentWrap, wrapOptions, IM_ARRAYSIZE(wrapOptions)))
            {
                asset->samplerDesc.repeat = static_cast<TextureWrap>(currentWrap);
                MarkDirty(engine);
            }

            UI::PropertyLabel("Anisotropy");
            const char* anisoOptions[] = {
                "Default",
                "Disabled",
                "2x",
                "4x",
                "8x",
                "16x"
            };
            int currentAniso = static_cast<int>(asset->samplerDesc.aniso);
            if (ImGui::Combo("##TextureAniso", &currentAniso, anisoOptions, IM_ARRAYSIZE(anisoOptions)))
            {
                asset->samplerDesc.aniso = static_cast<TextureAnisotropy>(currentAniso);
                MarkDirty(engine);
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("Billboard##billboardhdr"))
    {
        if (UI::BeginPropertyTable("BillboardTable"))
        {
            UI::PropertyLabel("Mode");
            const char* billboardModes[] = { "Disabled", "Spherical", "Cylindrical" };
            int currentBillboardMode = static_cast<int>(asset->billboardMode);
            if (ImGui::Combo("##bbmode", &currentBillboardMode, billboardModes, 3))
            {
                asset->billboardMode = static_cast<BillboardMode>(currentBillboardMode);
                MarkDirty(engine);
            }

            UI::EndPropertyTable();
        }
    }

    if (ImGui::CollapsingHeader("UV##uvhdr"))
    {
        if (UI::BeginPropertyTable("UVTable"))
        {
            UI::PropertyLabel("UV Offset");
            if (ImGui::DragFloat2("##uvoff", &asset->UVOffset[0], 0.05f))
            {
                MarkDirty(engine);
            }

            UI::PropertyLabel("UV Scale");
            if (ImGui::DragFloat2("##uvscl", &asset->UVScale[0], 0.05f))
            {
                MarkDirty(engine);
            }

            UI::PropertyLabel("Triplanar");
            if (ImGui::Checkbox("##triplanar", &asset->useTriplanar))
            {
                MarkDirty(engine);
            }

            if (asset->useTriplanar)
            {
                UI::PropertyLabel("World Triplanar");
                if (ImGui::Checkbox("##worldtriplanar", &asset->useWorldTriplanar))
                {
                    MarkDirty(engine);
                }

                UI::PropertyLabel("Triplanar Sharpness");
                if (ImGui::SliderFloat("##triplanarsharpness", &asset->triplanarSharpness, 1.0f, 16.0f, "%.2f"))
                {
                    MarkDirty(engine);
                }
            }

            UI::EndPropertyTable();
        }
    }

    ImGui::End();
}

}
