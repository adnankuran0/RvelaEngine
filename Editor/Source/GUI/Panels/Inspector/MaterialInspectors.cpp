#include "MaterialInspectors.h"
#include "InspectorUIUtils.h"
#include "InspectorContext.h"
#include "ImGui/imgui.h"
#include "EditorUtils.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetRegistry.h"
#include "Asset/Types/ShaderAsset.h"
#include "AssetImporters/MaterialSerializer.h"
#include "Scene/Entity.h"
#include <array>
#include <algorithm>
#include <string>

namespace rv {

void MaterialInspectors::Draw(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	UI::DrawComponent<MaterialComponent>("Material", registry, selectedEntity, [selectedEntity](MaterialComponent& material)
	{
		const MaterialInstance& inst = material.GetInstance();

		std::string matBtnLabel = "Default Material";
		if (inst.GetSourceAsset())
		{
			auto path = AssetManager::Get().GetRegistry().GetPath(inst.GetSourceUUID());
			if (!path.empty()) matBtnLabel = path.filename().string();
			else matBtnLabel = "Material Asset";
		}

		bool hasSource = (inst.GetSourceAsset() != nullptr);
		float saveBtnWidth = hasSource ? 60.0f : 0.0f;
		float clearBtnWidth = hasSource ? 24.0f : 0.0f;
		constexpr float spacing = 4.0f;
		float extraWidth = hasSource ? (saveBtnWidth + spacing + clearBtnWidth + spacing) : 0.0f;
		float slotWidth = ImGui::GetContentRegionAvail().x - extraWidth;

		if (ImGui::Button(matBtnLabel.c_str(), ImVec2(slotWidth, 24.0f)))
		{
			ImGui::OpenPopup("SelectMaterialPopup");
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
			{
				std::string pathStr((const char*)payload->Data);
				if (pathStr.ends_with(".rmat") || pathStr.find('#') != std::string::npos)
				{
					AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
					if (uuid.IsValid())
					{
						InspectorContext::ApplyEntityChange(selectedEntity, "Assign Material", [&]() {
							material.SetMaterial(uuid);
						});
					}
					else LOG_ERROR("Could not find UUID for material: {}", pathStr);
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopup("SelectMaterialPopup"))
		{
			static char filterText[128] = "";
			ImGui::TextDisabled("Select Material");
			ImGui::Separator();
			if (ImGui::IsWindowAppearing())
			{
				ImGui::SetKeyboardFocusHere();
				filterText[0] = '\0';
			}
			ImGui::SetNextItemWidth(300.0f);
			ImGui::InputTextWithHint("##MatSearchInput", "Search material...", filterText, sizeof(filterText));
			ImGui::Separator();

			auto matList = EditorUtils::GetAvailableMaterialAssets();
			std::string filterStr = filterText;
			std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

			ImGui::BeginChild("MatListScroll", ImVec2(300.0f, 220.0f), true);
			for (const auto& item : matList)
			{
				std::string itemLower = item.name + " " + item.path;
				std::transform(itemLower.begin(), itemLower.end(), itemLower.begin(), ::tolower);
				if (!filterStr.empty() && itemLower.find(filterStr) == std::string::npos)
					continue;

				bool isSelected = (inst.GetSourceUUID() == item.uuid);
				std::string displayLabel = item.name + " (" + item.path + ")";
				if (ImGui::Selectable(displayLabel.c_str(), isSelected))
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Select Material", [&]() {
						material.SetMaterial(item.uuid);
					});
					ImGui::CloseCurrentPopup();
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndChild();

			if (ImGui::Button("Cancel", ImVec2(-FLT_MIN, 22.0f)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		if (hasSource)
		{
			ImGui::SameLine(0, spacing);
			if (ImGui::Button("Save##matsave", ImVec2(saveBtnWidth, 24.0f)))
			{
				auto asset = inst.GetSourceAsset();
				if (inst.IsOverridden(MatField::AlbedoColor)) asset->albedoColor = material.GetAlbedoColor();
				if (inst.IsOverridden(MatField::EmissiveColor)) asset->emissiveColor = material.GetEmissiveColor();
				if (inst.IsOverridden(MatField::EmissiveIntensity)) asset->emissiveIntensity = material.GetEmissiveIntensity();
				if (inst.IsOverridden(MatField::Metallic)) asset->metallic = material.GetMetallic();
				if (inst.IsOverridden(MatField::Specular)) asset->specular = material.GetSpecular();
				if (inst.IsOverridden(MatField::Roughness)) asset->roughness = material.GetRoughness();
				if (inst.IsOverridden(MatField::AO)) asset->ao = material.GetAO();
				if (inst.IsOverridden(MatField::NormalScale)) asset->normalScale = material.GetNormalScale();
				if (inst.IsOverridden(MatField::HeightScale)) asset->heightScale = material.GetHeightScale();
				if (inst.IsOverridden(MatField::InvertHeight)) asset->invertHeight = material.GetInvertHeight();
				if (inst.IsOverridden(MatField::UVScale)) asset->UVScale = material.GetUVScale();
				if (inst.IsOverridden(MatField::UVOffset)) asset->UVOffset = material.GetUVOffset();
				if (inst.IsOverridden(MatField::UseTriplanar)) asset->useTriplanar = material.GetUseTriplanar();
				if (inst.IsOverridden(MatField::UseWorldTriplanar)) asset->useWorldTriplanar = material.GetUseWorldTriplanar();
				if (inst.IsOverridden(MatField::TriplanarSharpness)) asset->triplanarSharpness = material.GetTriplanarSharpness();
				if (inst.IsOverridden(MatField::TransparencyMode)) asset->transparencyMode = material.GetTransparencyMode();
				if (inst.IsOverridden(MatField::BlendMode)) asset->blendMode = material.GetBlendMode();
				if (inst.IsOverridden(MatField::CullMode)) asset->cullMode = material.GetCullMode();
				if (inst.IsOverridden(MatField::AlphaCutoff)) asset->alphaCutoff = material.GetAlphaCutoff();

				if (inst.IsOverridden(MatField::AlbedoTex)) { asset->albedoTextureUUID = material.GetAlbedoTexture() ? material.GetAlbedoTexture()->GetUUID() : AssetUUID::Invalid(); asset->useAlbedoMap = material.IsUsingAlbedoMap(); }
				if (inst.IsOverridden(MatField::NormalTex)) { asset->normalTextureUUID = material.GetNormalTexture() ? material.GetNormalTexture()->GetUUID() : AssetUUID::Invalid(); asset->useNormalMap = material.IsUsingNormalMap(); }
				if (inst.IsOverridden(MatField::MetallicTex)) { asset->metallicTextureUUID = material.GetMetallicTexture() ? material.GetMetallicTexture()->GetUUID() : AssetUUID::Invalid(); asset->useMetallicMap = material.IsUsingMetallicMap(); }
				if (inst.IsOverridden(MatField::RoughnessTex)) { asset->roughnessTextureUUID = material.GetRoughnessTexture() ? material.GetRoughnessTexture()->GetUUID() : AssetUUID::Invalid(); asset->useRoughnessMap = material.IsUsingRoughnessMap(); }
				if (inst.IsOverridden(MatField::AOTex)) { asset->aoTextureUUID = material.GetAOTexture() ? material.GetAOTexture()->GetUUID() : AssetUUID::Invalid(); asset->useAOMap = material.IsUsingAOMap(); }
				if (inst.IsOverridden(MatField::HeightTex)) { asset->heightTextureUUID = material.GetHeightTexture() ? material.GetHeightTexture()->GetUUID() : AssetUUID::Invalid(); asset->useHeightMap = material.IsUsingHeightMap(); }
				if (inst.IsOverridden(MatField::ARMTex)) { asset->armTextureUUID = material.GetARMTexture() ? material.GetARMTexture()->GetUUID() : AssetUUID::Invalid(); asset->useARMMap = material.IsUsingARMMap(); }
				if (inst.IsOverridden(MatField::UseARMMap)) { asset->useARMMap = material.GetUseARMMap(); }

				if (inst.IsOverridden(MatField::CustomShader)) asset->customShaderUUID = material.GetCustomShaderUUID();
				if (inst.IsOverridden(MatField::TextureFilterMode)) asset->samplerDesc.filter = material.GetTextureFilter();
				if (inst.IsOverridden(MatField::TextureWrapMode)) asset->samplerDesc.repeat = material.GetTextureWrap();
				if (inst.IsOverridden(MatField::TextureAnisotropyMode)) asset->samplerDesc.aniso = material.GetTextureAnisotropy();
				for (const auto& [name, val] : inst.GetCustomUniformOverrides())
				{
					asset->customUniforms[name] = val;
				}

				auto path = AssetManager::Get().GetRegistry().GetPath(asset->GetUUID());
				MaterialSerializer::Save(asset, path);
				material.GetInstance().ClearAllOverrides();
				material.GetInstance().RebuildCache();
			}
			ImGui::SameLine(0, spacing);
			if (ImGui::Button("X##matclear", ImVec2(clearBtnWidth, 24.0f)))
			{
				InspectorContext::ApplyEntityChange(selectedEntity, "Clear Material", [&]() {
					material.SetMaterial(AssetUUID::Invalid());
				});
			}
		}

		ImGui::Spacing();

		auto RevertButton = [&](const char* id, auto clearFn) {
			ImGui::SameLine();
			if (ImGui::Button(id, ImVec2(22, 22)))
			{
				InspectorContext::ApplyEntityChange(selectedEntity, "Revert Material Texture", [&]() {
					clearFn();
				});
			}
		};

		auto TextureSlot = [&](const char* label, const char* idSuffix, bool overridden, Ref<TextureAsset> currentTex, auto setTexFn, auto clearTexFn) {
			UI::PropertyLabel(label);
			float btnWidth = overridden ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
			std::string btnText = "Texture";
			if (currentTex)
			{
				std::string fn = EditorUtils::GetAssetFileName(currentTex->GetUUID());
				if (!fn.empty())
					btnText = fn;
			}
			ImGui::Button(btnText.c_str(), ImVec2(btnWidth, 22));
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
							InspectorContext::ApplyEntityChange(selectedEntity, "Assign Texture", [&]() {
								setTexFn(uuid);
							});
						}
						else LOG_ERROR("Could not find UUID for texture: {}", pathStr);
					}
				}
				ImGui::EndDragDropTarget();
			}
			if (overridden) {
				RevertButton((std::string("R##") + idSuffix).c_str(), clearTexFn);
			}
		};

		if (UI::BeginPropertyTable("CustomShaderTable"))
		{
			UI::PropertyLabel("Shader");
			std::string shaderBtnLabel = "Default (Built-in PBR)";
			AssetUUID curShaderUUID = material.GetCustomShaderUUID();
			Ref<ShaderAsset> shaderAsset = material.GetCustomShaderAsset();
			if (curShaderUUID.IsValid())
			{
				auto sPath = AssetManager::Get().GetRegistry().GetPath(curShaderUUID);
				if (!sPath.empty()) shaderBtnLabel = sPath.filename().string();
				else shaderBtnLabel = "Shader Asset";
			}

			float sClearWidth = curShaderUUID.IsValid() ? 26.0f : 0.0f;
			float sSlotWidth = curShaderUUID.IsValid() ? ImGui::GetContentRegionAvail().x - sClearWidth : -FLT_MIN;

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
							InspectorContext::ApplyEntityChange(selectedEntity, "Assign Shader", [&]() {
								material.SetCustomShader(uuid);
							});
						}
						else LOG_ERROR("Could not find UUID for shader: {}", pathStr);
					}
				}
				ImGui::EndDragDropTarget();
			}
			if (curShaderUUID.IsValid())
			{
				ImGui::SameLine();
				if (ImGui::Button("X##customshaderclear", ImVec2(sClearWidth - 4.0f, 22.0f)))
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Clear Shader", [&]() {
						material.ClearCustomShader();
					});
				}
			}
			UI::EndPropertyTable();
		}

		if (material.GetCustomShaderUUID().IsValid())
		{
			Ref<ShaderAsset> shaderAsset = material.GetCustomShaderAsset();
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
				if (ImGui::SmallButton("Recompile##shaderrecompile"))
				{
					AssetUUID sUUID = material.GetCustomShaderUUID();
					auto sPath = sUUID.IsValid() ? AssetManager::Get().GetRegistry().GetPath(sUUID) : std::filesystem::path{};
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
					if (UI::BeginPropertyTable("CustomUniformsTable"))
					{
						for (const auto& uDef : shaderAsset->uniforms)
						{
							if (uDef.engineTexture != EngineTextureType::None)
								continue;

							UI::PropertyLabel(uDef.displayName.c_str());
							bool isOver = inst.IsCustomUniformOverridden(uDef.name);
							CustomUniformValue uVal = material.GetCustomUniform(uDef.name);
							bool changed = false;

							float ctrlWidth = isOver ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
							ImGui::SetNextItemWidth(ctrlWidth);

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
								ImGui::Button(texLabel.c_str(), ImVec2(ctrlWidth, 22.0f));
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
								InspectorContext::ApplyEntityChange(selectedEntity, "Change Custom Uniform: " + uDef.name, [&]() {
									material.SetCustomUniform(uDef.name, uVal);
								});
							}

							if (isOver)
							{
								RevertButton(("R##revert_" + uDef.name).c_str(), [&]() {
									InspectorContext::ApplyEntityChange(selectedEntity, "Revert Custom Uniform: " + uDef.name, [&]() {
										material.ClearCustomUniform(uDef.name);
									});
								});
							}
						}
						UI::EndPropertyTable();
					}
				}
			}
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
		}

		if (ImGui::CollapsingHeader("Transparency##transphdr"))
		{
			if (UI::BeginPropertyTable("TransparencyTable"))
			{
				UI::PropertyLabel("Mode");
				const char* transModes[] = { "Opaque", "Alpha", "Alpha Scissor", "Depth Prepass" };
				int currentTransMode = static_cast<int>(material.GetTransparencyMode());
				float itemW = inst.IsOverridden(MatField::TransparencyMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(itemW);
				if (ImGui::Combo("##tmode", &currentTransMode, transModes, 4))
					material.SetTransparencyMode(static_cast<TransparencyMode>(currentTransMode));
				if (inst.IsOverridden(MatField::TransparencyMode)) RevertButton("R##tmode", [&]() { material.ClearTransparencyMode(); });

				if (material.GetTransparencyMode() == TransparencyMode::AlphaScissor || material.GetTransparencyMode() == TransparencyMode::DepthPrepass)
				{
					UI::PropertyLabel("Alpha Cutoff");
					float cutoff = material.GetAlphaCutoff();
					float cutW = inst.IsOverridden(MatField::AlphaCutoff) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
					ImGui::SetNextItemWidth(cutW);
					if (ImGui::SliderFloat("##acutoff", &cutoff, 0.0f, 1.0f, "%.2f")) material.SetAlphaCutoff(cutoff);
					if (inst.IsOverridden(MatField::AlphaCutoff)) RevertButton("R##acutoff", [&]() { material.ClearAlphaCutoff(); });
				}

				if (material.GetTransparencyMode() == TransparencyMode::Alpha || material.GetTransparencyMode() == TransparencyMode::DepthPrepass)
				{
					UI::PropertyLabel("Blend Mode");
					const char* blendModes[] = { "Mix", "Add", "Subtract", "Multiply" };
					int currentBlendMode = static_cast<int>(material.GetBlendMode());
					float blendW = inst.IsOverridden(MatField::BlendMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
					ImGui::SetNextItemWidth(blendW);
					if (ImGui::Combo("##bmode", &currentBlendMode, blendModes, 4))
						material.SetBlendMode(static_cast<BlendMode>(currentBlendMode));
					if (inst.IsOverridden(MatField::BlendMode)) RevertButton("R##bmode", [&]() { material.ClearBlendMode(); });
				}

				UI::PropertyLabel("Cull Mode");
				const char* cullModes[] = { "Back", "Front", "Disabled" };
				int currentCullMode = static_cast<int>(material.GetCullMode());
				float cullW = inst.IsOverridden(MatField::CullMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(cullW);
				if (ImGui::Combo("##cmode", &currentCullMode, cullModes, 3))
					material.SetCullMode(static_cast<CullMode>(currentCullMode));
				if (inst.IsOverridden(MatField::CullMode)) RevertButton("R##cmode", [&]() { material.ClearCullMode(); });

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Shading##shadinghdr"))
		{
			if (UI::BeginPropertyTable("ShadingTable"))
			{
				UI::PropertyLabel("Shading Mode");
				const char* shadingModes[] = { "Lit", "Unshaded" };
				int currentShadingMode = static_cast<int>(material.GetShadingMode());
				float shdW = inst.IsOverridden(MatField::ShadingMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(shdW);
				if (ImGui::Combo("##smode", &currentShadingMode, shadingModes, 2))
					material.SetShadingMode(static_cast<ShadingMode>(currentShadingMode));
				if (inst.IsOverridden(MatField::ShadingMode)) RevertButton("R##smode", [&]() { material.ClearShadingMode(); });

				if (material.GetShadingMode() == ShadingMode::Lit)
				{
					UI::PropertyLabel("Receive Shadows");
					bool receiveShadows = material.GetReceiveShadows();
					if (ImGui::Checkbox("##rshadows", &receiveShadows)) material.SetReceiveShadows(receiveShadows);
					if (inst.IsOverridden(MatField::ReceiveShadows)) RevertButton("R##rshadows", [&]() { material.ClearReceiveShadows(); });
				}

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Albedo"))
		{
			if (UI::BeginPropertyTable("AlbedoTable"))
			{
				TextureSlot("Texture", "alb", inst.IsOverridden(MatField::AlbedoTex), material.GetAlbedoTexture(), [&](AssetUUID u) { material.SetAlbedoTexture(u); }, [&]() { material.ClearAlbedoTexture(); });

				UI::PropertyLabel("Color");
				glm::vec4 c = material.GetAlbedoColor();
				float col[4] = { c.r, c.g, c.b, c.a };
				float colW = inst.IsOverridden(MatField::AlbedoColor) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(colW);
				if (ImGui::ColorEdit4("##albedocol", col)) material.SetAlbedoColor({ col[0], col[1], col[2], col[3] });
				if (inst.IsOverridden(MatField::AlbedoColor)) RevertButton("R##albcol", [&]() { material.ClearAlbedoColor(); });

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Normal"))
		{
			if (UI::BeginPropertyTable("NormalTable"))
			{
				TextureSlot("Texture", "nrm", inst.IsOverridden(MatField::NormalTex), material.GetNormalTexture(), [&](AssetUUID u) { material.SetNormalTexture(u); }, [&]() { material.ClearNormalTexture(); });

				UI::PropertyLabel("Scale");
				float normalScale = material.GetNormalScale();
				float scaleW = inst.IsOverridden(MatField::NormalScale) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(scaleW);
				if (ImGui::SliderFloat("##nrmscl", &normalScale, -10.0f, 10.0f, "%.2f")) material.SetNormalScale(normalScale);
				if (inst.IsOverridden(MatField::NormalScale)) RevertButton("R##nrmscl", [&]() { material.ClearNormalScale(); });

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Roughness##rhdr"))
		{
			if (UI::BeginPropertyTable("RoughnessTable"))
			{
				TextureSlot("Texture", "rgh", inst.IsOverridden(MatField::RoughnessTex), material.GetRoughnessTexture(), [&](AssetUUID u) { material.SetRoughnessTexture(u); }, [&]() { material.ClearRoughnessTexture(); });

				UI::PropertyLabel("Roughness");
				float roughness = material.GetRoughness();
				float rghW = inst.IsOverridden(MatField::Roughness) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(rghW);
				if (ImGui::SliderFloat("##rghval", &roughness, 0.0f, 1.0f, "%.2f")) material.SetRoughness(roughness);
				if (inst.IsOverridden(MatField::Roughness)) RevertButton("R##rghval", [&]() { material.ClearRoughness(); });

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Metallic##mhdr"))
		{
			if (UI::BeginPropertyTable("MetallicTable"))
			{
				TextureSlot("Texture", "mtl", inst.IsOverridden(MatField::MetallicTex), material.GetMetallicTexture(), [&](AssetUUID u) { material.SetMetallicTexture(u); }, [&]() { material.ClearMetallicTexture(); });

				UI::PropertyLabel("Metallic");
				float metallicVal = material.GetMetallic();
				float mtlW = inst.IsOverridden(MatField::Metallic) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(mtlW);
				if (ImGui::SliderFloat("##mtlval", &metallicVal, 0.0f, 1.0f, "%.2f")) material.SetMetallic(metallicVal);
				if (inst.IsOverridden(MatField::Metallic)) RevertButton("R##mtlval", [&]() { material.ClearMetallic(); });

				UI::PropertyLabel("Specular");
				float specular = material.GetSpecular();
				float spcW = inst.IsOverridden(MatField::Specular) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(spcW);
				if (ImGui::SliderFloat("##spcval", &specular, 0.0f, 1.0f, "%.2f")) material.SetSpecular(specular);
				if (inst.IsOverridden(MatField::Specular)) RevertButton("R##spcval", [&]() { material.ClearSpecular(); });

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Displacement"))
		{
			if (UI::BeginPropertyTable("HeightTable"))
			{
				TextureSlot("Texture", "hgt", inst.IsOverridden(MatField::HeightTex), material.GetHeightTexture(), [&](AssetUUID u) { material.SetHeightTexture(u); }, [&]() { material.ClearHeightTexture(); });

				UI::PropertyLabel("Scale");
				float heightScale = material.GetHeightScale();
				float hgtW = inst.IsOverridden(MatField::HeightScale) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(hgtW);
				if (ImGui::SliderFloat("##hgtscl", &heightScale, 0.0f, 0.5f, "%.3f")) material.SetHeightScale(heightScale);
				if (inst.IsOverridden(MatField::HeightScale)) RevertButton("R##hgtscl", [&]() { material.ClearHeightScale(); });

				UI::PropertyLabel("Invert Height");
				bool invert = material.GetInvertHeight();
				float invW = inst.IsOverridden(MatField::InvertHeight) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(invW);
				if (ImGui::Checkbox("##invhgt", &invert)) material.SetInvertHeight(invert);
				if (inst.IsOverridden(MatField::InvertHeight)) RevertButton("R##invhgt", [&]() { material.ClearInvertHeight(); });

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("AO##aohdr"))
		{
			if (UI::BeginPropertyTable("AOTable"))
			{
				TextureSlot("Texture", "ao", inst.IsOverridden(MatField::AOTex), material.GetAOTexture(), [&](AssetUUID u) { material.SetAOTexture(u); }, [&]() { material.ClearAOTexture(); });

				UI::PropertyLabel("AO");
				float ao = material.GetAO();
				float aoW = inst.IsOverridden(MatField::AO) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(aoW);
				if (ImGui::SliderFloat("##aoval", &ao, 0.0f, 1.0f, "%.2f")) material.SetAO(ao);
				if (inst.IsOverridden(MatField::AO)) RevertButton("R##aoval", [&]() { material.ClearAO(); });

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("ARM##armhdr"))
		{
			if (UI::BeginPropertyTable("ARMTable"))
			{
				UI::PropertyLabel("Use ARM Map");
				bool useARM = material.GetUseARMMap();
				float armCheckW = inst.IsOverridden(MatField::UseARMMap) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(armCheckW);
				if (ImGui::Checkbox("##usearm", &useARM)) material.SetUseARMMap(useARM);
				if (inst.IsOverridden(MatField::UseARMMap)) RevertButton("R##usearm", [&]() { material.ClearUseARMMap(); });

				TextureSlot("ARM Map", "arm", inst.IsOverridden(MatField::ARMTex), material.GetARMTexture(), [&](AssetUUID u) { material.SetARMTexture(u); }, [&]() { material.ClearARMTexture(); });

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Emissive"))
		{
			if (UI::BeginPropertyTable("EmissiveTable"))
			{
				UI::PropertyLabel("Color");
				glm::vec3 ec = material.GetEmissiveColor();
				float col[3] = { ec.r, ec.g, ec.b };
				float colW = inst.IsOverridden(MatField::EmissiveColor) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(colW);
				if (ImGui::ColorEdit3("##emcol", col)) material.SetEmissiveColor({ col[0], col[1], col[2] });
				if (inst.IsOverridden(MatField::EmissiveColor)) RevertButton("R##emcol", [&]() { material.ClearEmissiveColor(); });

				UI::PropertyLabel("Intensity");
				float intensity = material.GetEmissiveIntensity();
				float intW = inst.IsOverridden(MatField::EmissiveIntensity) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(intW);
				if (ImGui::SliderFloat("##emint", &intensity, 0.0f, 20.0f, "%.1f")) material.SetEmissiveIntensity(intensity);
				if (inst.IsOverridden(MatField::EmissiveIntensity)) RevertButton("R##emint", [&]() { material.ClearEmissiveIntensity(); });

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
				int currentFilter = static_cast<int>(material.GetTextureFilter());
				float filterW = inst.IsOverridden(MatField::TextureFilterMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(filterW);
				if (ImGui::Combo("##TextureFilter", &currentFilter, filterOptions, IM_ARRAYSIZE(filterOptions)))
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Change Texture Filter", [&]() {
						material.SetTextureFilter(static_cast<TextureFilter>(currentFilter));
					});
				}
				if (inst.IsOverridden(MatField::TextureFilterMode)) RevertButton("R##texfilt", [&]() { material.ClearTextureFilter(); });

				UI::PropertyLabel("Repeat / Wrap");
				const char* wrapOptions[] = {
					"Default",
					"Repeat",
					"Mirror Repeat",
					"Clamp to Edge",
					"Clamp to Border",
					"Mirror Clamp to Edge"
				};
				int currentWrap = static_cast<int>(material.GetTextureWrap());
				float wrapW = inst.IsOverridden(MatField::TextureWrapMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(wrapW);
				if (ImGui::Combo("##TextureWrap", &currentWrap, wrapOptions, IM_ARRAYSIZE(wrapOptions)))
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Change Texture Wrap", [&]() {
						material.SetTextureWrap(static_cast<TextureWrap>(currentWrap));
					});
				}
				if (inst.IsOverridden(MatField::TextureWrapMode)) RevertButton("R##texwrap", [&]() { material.ClearTextureWrap(); });

				UI::PropertyLabel("Anisotropy");
				const char* anisoOptions[] = {
					"Default",
					"Disabled",
					"2x",
					"4x",
					"8x",
					"16x"
				};
				int currentAniso = static_cast<int>(material.GetTextureAnisotropy());
				float anisoW = inst.IsOverridden(MatField::TextureAnisotropyMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(anisoW);
				if (ImGui::Combo("##TextureAniso", &currentAniso, anisoOptions, IM_ARRAYSIZE(anisoOptions)))
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Change Texture Anisotropy", [&]() {
						material.SetTextureAnisotropy(static_cast<TextureAnisotropy>(currentAniso));
					});
				}
				if (inst.IsOverridden(MatField::TextureAnisotropyMode)) RevertButton("R##texaniso", [&]() { material.ClearTextureAnisotropy(); });

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("Billboard##billboardhdr"))
		{
			if (UI::BeginPropertyTable("BillboardTable"))
			{
				UI::PropertyLabel("Mode");
				const char* billboardModes[] = { "Disabled", "Spherical", "Cylindrical" };
				int currentBillboardMode = static_cast<int>(material.GetBillboardMode());
				float billW = inst.IsOverridden(MatField::BillboardMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(billW);
				if (ImGui::Combo("##bmode", &currentBillboardMode, billboardModes, 3))
					material.SetBillboardMode(static_cast<BillboardMode>(currentBillboardMode));
				if (inst.IsOverridden(MatField::BillboardMode)) RevertButton("R##bmode", [&]() { material.ClearBillboardMode(); });

				UI::EndPropertyTable();
			}
		}

		if (ImGui::CollapsingHeader("UV##uvhdr"))
		{
			if (UI::BeginPropertyTable("UVTable"))
			{
				UI::PropertyLabel("UV Offset");
				glm::vec2 uvOffset = material.GetUVOffset();
				float offW = inst.IsOverridden(MatField::UVOffset) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(offW);
				if (ImGui::DragFloat2("##uvoff", &uvOffset[0], 0.05f)) material.SetUVOffset(uvOffset);
				if (inst.IsOverridden(MatField::UVOffset)) RevertButton("R##uvoff", [&]() { material.ClearUVOffset(); });

				UI::PropertyLabel("UV Scale");
				glm::vec2 uvScale = material.GetUVScale();
				float sclW = inst.IsOverridden(MatField::UVScale) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
				ImGui::SetNextItemWidth(sclW);
				if (ImGui::DragFloat2("##uvscl", &uvScale[0], 0.05f)) material.SetUVScale(uvScale);
				if (inst.IsOverridden(MatField::UVScale)) RevertButton("R##uvscl", [&]() { material.ClearUVScale(); });

				UI::PropertyLabel("Triplanar");
				bool useTriplanar = material.GetUseTriplanar();
				if (ImGui::Checkbox("##triplanar", &useTriplanar)) material.SetUseTriplanar(useTriplanar);
				if (inst.IsOverridden(MatField::UseTriplanar)) RevertButton("R##triplanar", [&]() { material.ClearUseTriplanar(); });

				if (material.GetUseTriplanar())
				{
					UI::PropertyLabel("World Triplanar");
					bool useWorldTriplanar = material.GetUseWorldTriplanar();
					if (ImGui::Checkbox("##worldtriplanar", &useWorldTriplanar)) material.SetUseWorldTriplanar(useWorldTriplanar);
					if (inst.IsOverridden(MatField::UseWorldTriplanar)) RevertButton("R##worldtriplanar", [&]() { material.ClearUseWorldTriplanar(); });

					UI::PropertyLabel("Triplanar Sharpness");
					float sharpness = material.GetTriplanarSharpness();
					float sharpW = inst.IsOverridden(MatField::TriplanarSharpness) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
					ImGui::SetNextItemWidth(sharpW);
					if (ImGui::SliderFloat("##triplanarsharpness", &sharpness, 1.0f, 16.0f, "%.2f")) material.SetTriplanarSharpness(sharpness);
					if (inst.IsOverridden(MatField::TriplanarSharpness)) RevertButton("R##triplanarsharpness", [&]() { material.ClearTriplanarSharpness(); });
				}

				UI::EndPropertyTable();
			}
		}
	});
}

} // namespace rv
