#include "UIInspectors.h"
#include "InspectorUIUtils.h"
#include "InspectorContext.h"
#include "ImGui/imgui.h"
#include "EditorUtils.h"
#include "Scene/Entity.h"
#include <cstring>
#include <string>
#include <vector>

namespace rv {

static void DrawTextureSlot(const char* label, const char* id, AssetUUID& textureUUID, entt::entity selectedEntity)
{
	UI::PropertyLabel(label);
	std::string texName = "None (Texture)";
	if (textureUUID.IsValid())
	{
		std::string fn = EditorUtils::GetAssetFileName(textureUUID);
		texName = fn.empty() ? "Texture Asset" : fn;
	}

	float clearBtnWidth = textureUUID.IsValid() ? 24.0f : 0.0f;
	float slotWidth = textureUUID.IsValid() ? (ImGui::GetContentRegionAvail().x - clearBtnWidth - 4.0f) : -FLT_MIN;

	std::string btnLabel = texName + "##" + id;
	ImGui::Button(btnLabel.c_str(), ImVec2(slotWidth, 22.0f));
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
		{
			std::string pathStr((const char*)payload->Data);
			AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
			if (uuid.IsValid())
			{
				InspectorContext::ApplyEntityChange(selectedEntity, "Assign UI Texture", [&]() {
					textureUUID = uuid;
				});
			}
		}
		ImGui::EndDragDropTarget();
	}

	if (textureUUID.IsValid())
	{
		ImGui::SameLine();
		std::string clearBtnId = "X##Clear" + std::string(id);
		if (ImGui::Button(clearBtnId.c_str(), ImVec2(clearBtnWidth, 22.0f)))
		{
			InspectorContext::ApplyEntityChange(selectedEntity, "Clear UI Texture", [&]() {
				textureUUID = AssetUUID::Invalid();
			});
		}
	}
}

void UIInspectors::Draw(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	UI::DrawComponent<UICanvasComponent>("UI Canvas", registry, selectedEntity, [&](UICanvasComponent& canvas)
	{
		if (UI::BeginPropertyTable("UICanvasTable"))
		{
			UI::PropertyLabel("Canvas Mode");
			const char* modes[] = { "Screen Space Overlay", "World Space" };
			int currentMode = static_cast<int>(canvas.mode);
			if (ImGui::Combo("##CanvasMode", &currentMode, modes, 2))
				canvas.mode = static_cast<CanvasMode>(currentMode);

			if (canvas.mode == CanvasMode::WorldSpace)
			{
				UI::PropertyLabel("Depth Test");
				ImGui::Checkbox("##DepthTest", &canvas.depthTest);

				UI::PropertyLabel("Billboard Mode");
				const char* bbModes[] = { "Disabled", "Spherical", "Cylindrical" };
				int currentBB = static_cast<int>(canvas.billboardMode);
				if (ImGui::Combo("##BillboardMode", &currentBB, bbModes, 3))
					canvas.billboardMode = static_cast<BillboardMode>(currentBB);

				UI::PropertyLabel("Constant Screen Size");
				ImGui::Checkbox("##ConstantScreenSize", &canvas.constantScreenSize);

				if (canvas.constantScreenSize)
				{
					UI::PropertyLabel("Scale Factor");
					ImGui::DragFloat("##ConstantScaleFactor", &canvas.constantScaleFactor, 0.05f, 0.05f, 50.0f);
				}
			}

			UI::PropertyLabel("Ref Resolution");
			ImGui::DragFloat2("##RefRes", &canvas.referenceResolution.x, 1.0f, 100.0f, 8192.0f);

			UI::PropertyLabel("Sort Order");
			ImGui::DragInt("##SortOrder", &canvas.sortOrder, 1, -100, 100);

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<RectTransformComponent>("Rect Transform", registry, selectedEntity, [&](RectTransformComponent& rect)
	{
		if (UI::BeginPropertyTable("RectTransformTable"))
		{
			UI::PropertyLabel("Position");
			ImGui::DragFloat2("##Pos", &rect.position.x, 1.0f);

			UI::PropertyLabel("Size");
			ImGui::DragFloat2("##Size", &rect.size.x, 1.0f, 0.0f, 8192.0f);

			UI::PropertyLabel("Anchor Min");
			ImGui::DragFloat2("##AnchorMin", &rect.anchorMin.x, 0.01f, 0.0f, 1.0f);

			UI::PropertyLabel("Anchor Max");
			ImGui::DragFloat2("##AnchorMax", &rect.anchorMax.x, 0.01f, 0.0f, 1.0f);

			UI::PropertyLabel("Pivot");
			ImGui::DragFloat2("##Pivot", &rect.pivot.x, 0.01f, 0.0f, 1.0f);

			UI::PropertyLabel("Rotation");
			ImGui::DragFloat("##Rotation", &rect.rotation, 0.5f, -360.0f, 360.0f);

			UI::PropertyLabel("Scale");
			ImGui::DragFloat2("##Scale", &rect.scale.x, 0.05f, 0.0f, 100.0f);

			UI::PropertyLabel("Raycast Target");
			ImGui::Checkbox("##RaycastTarget", &rect.raycastTarget);

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UIImageComponent>("UI Image", registry, selectedEntity, [&](UIImageComponent& img)
	{
		if (UI::BeginPropertyTable("UIImageTable"))
		{
			UI::PropertyLabel("Color");
			ImGui::ColorEdit4("##Color", &img.color.r);

			DrawTextureSlot("Texture", "UIImageTex", img.textureUUID, selectedEntity);

			UI::PropertyLabel("Corner Radius");
			ImGui::DragFloat("##CornerRadius", &img.cornerRadius, 0.5f, 0.0f, 100.0f);

			UI::PropertyLabel("Image Type");
			const char* types[] = { "Simple", "Sliced" };
			int currentType = static_cast<int>(img.imageType);
			if (ImGui::Combo("##ImageType", &currentType, types, 2))
				img.imageType = static_cast<ImageType>(currentType);

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UITextComponent>("UI Text", registry, selectedEntity, [&](UITextComponent& textComp)
	{
		if (UI::BeginPropertyTable("UITextTable"))
		{
			UI::PropertyLabel("Text");
			char buf[1024];
			strncpy(buf, textComp.text.c_str(), sizeof(buf));
			if (ImGui::InputTextMultiline("##Text", buf, sizeof(buf), ImVec2(-FLT_MIN, 60.0f)))
				textComp.text = buf;

			UI::PropertyLabel("Color");
			ImGui::ColorEdit4("##Color", &textComp.color.r);

			UI::PropertyLabel("Font Size");
			ImGui::DragFloat("##FontSize", &textComp.fontSize, 1.0f, 6.0f, 128.0f);

			UI::PropertyLabel("Alignment");
			const char* aligns[] = { "Left", "Center", "Right" };
			int currentAlign = static_cast<int>(textComp.alignment);
			if (ImGui::Combo("##Align", &currentAlign, aligns, 3))
				textComp.alignment = static_cast<TextAlignment>(currentAlign);

			UI::PropertyLabel("Vertical Alignment");
			const char* vAligns[] = { "Top", "Middle", "Bottom" };
			int currentVAlign = static_cast<int>(textComp.verticalAlignment);
			if (ImGui::Combo("##VAlign", &currentVAlign, vAligns, 3))
				textComp.verticalAlignment = static_cast<TextVerticalAlignment>(currentVAlign);

			UI::PropertyLabel("Word Wrap");
			ImGui::Checkbox("##WordWrap", &textComp.wordWrap);

			UI::PropertyLabel("Line Spacing");
			ImGui::DragFloat("##LineSpacing", &textComp.lineSpacing, 0.05f, 0.5f, 3.0f, "%.2f");

			UI::PropertyLabel("Char Spacing");
			ImGui::DragFloat("##CharSpacing", &textComp.characterSpacing, 0.5f, -10.0f, 50.0f, "%.1f px");

			UI::PropertyLabel("Bold");
			ImGui::Checkbox("##IsBold", &textComp.isBold);

			UI::PropertyLabel("Outline");
			ImGui::Checkbox("##IsOutline", &textComp.isOutline);

			if (textComp.isOutline)
			{
				UI::PropertyLabel("Outline Size");
				ImGui::DragFloat("##OutlineSize", &textComp.outlineSize, 0.1f, 0.5f, 10.0f, "%.1f px");

				UI::PropertyLabel("Outline Color");
				ImGui::ColorEdit4("##OutlineColor", &textComp.outlineColor.r);
			}

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UIButtonComponent>("UI Button", registry, selectedEntity, [&](UIButtonComponent& btn)
	{
		if (UI::BeginPropertyTable("UIButtonTable"))
		{
			UI::PropertyLabel("Interactable");
			ImGui::Checkbox("##Interactable", &btn.interactable);

			UI::PropertyLabel("Text");
			char bufText[1024];
			strncpy(bufText, btn.text.c_str(), sizeof(bufText));
			if (ImGui::InputTextMultiline("##BtnText", bufText, sizeof(bufText), ImVec2(-FLT_MIN, 45.0f)))
				btn.text = bufText;

			UI::PropertyLabel("Text Color");
			ImGui::ColorEdit4("##TextColor", &btn.textColor.r);

			UI::PropertyLabel("Font Size");
			ImGui::DragFloat("##FontSize", &btn.fontSize, 1.0f, 6.0f, 128.0f);

			UI::PropertyLabel("Bold Text");
			ImGui::Checkbox("##IsBoldText", &btn.isBold);

			UI::PropertyLabel("Outline Text");
			ImGui::Checkbox("##IsOutlineText", &btn.isOutline);

			if (btn.isOutline)
			{
				UI::PropertyLabel("Outline Size");
				ImGui::DragFloat("##OutlineSizeText", &btn.outlineSize, 0.1f, 0.5f, 10.0f, "%.1f px");

				UI::PropertyLabel("Outline Color");
				ImGui::ColorEdit4("##OutlineColorText", &btn.outlineColor.r);
			}

			UI::PropertyLabel("Corner Radius");
			ImGui::DragFloat("##CornerRadius", &btn.cornerRadius, 0.5f, 0.0f, 100.0f);

			UI::PropertyLabel("Normal Color");
			ImGui::ColorEdit4("##NormalCol", &btn.normalColor.r);

			UI::PropertyLabel("Hover Color");
			ImGui::ColorEdit4("##HoverCol", &btn.hoverColor.r);

			UI::PropertyLabel("Pressed Color");
			ImGui::ColorEdit4("##PressedCol", &btn.pressedColor.r);

			UI::PropertyLabel("Disabled Color");
			ImGui::ColorEdit4("##DisabledCol", &btn.disabledColor.r);

			DrawTextureSlot("Normal Tex", "BtnNormTex", btn.normalTextureUUID, selectedEntity);
			DrawTextureSlot("Hover Tex", "BtnHovTex", btn.hoverTextureUUID, selectedEntity);
			DrawTextureSlot("Pressed Tex", "BtnPressTex", btn.pressedTextureUUID, selectedEntity);
			DrawTextureSlot("Disabled Tex", "BtnDisTex", btn.disabledTextureUUID, selectedEntity);

			UI::PropertyLabel("Image Type");
			const char* types[] = { "Simple", "Sliced" };
			int currentType = static_cast<int>(btn.imageType);
			if (ImGui::Combo("##BtnImageType", &currentType, types, 2))
				btn.imageType = static_cast<ImageType>(currentType);

			if (btn.imageType == ImageType::Sliced)
			{
				UI::PropertyLabel("Border Size");
				ImGui::DragFloat("##BtnBorderSize", &btn.borderSize, 0.5f, 1.0f, 64.0f);
			}

			UI::PropertyLabel("Lua Callback");
			char buf[128];
			strncpy(buf, btn.luaCallback.c_str(), sizeof(buf));
			if (ImGui::InputText("##LuaCB", buf, sizeof(buf)))
				btn.luaCallback = buf;

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UISliderComponent>("UI Slider", registry, selectedEntity, [&](UISliderComponent& slider)
	{
		if (UI::BeginPropertyTable("UISliderTable"))
		{
			UI::PropertyLabel("Value");
			ImGui::SliderFloat("##Val", &slider.value, slider.minValue, slider.maxValue);

			UI::PropertyLabel("Min Value");
			ImGui::DragFloat("##MinVal", &slider.minValue, 0.1f);

			UI::PropertyLabel("Max Value");
			ImGui::DragFloat("##MaxVal", &slider.maxValue, 0.1f);

			UI::PropertyLabel("Background");
			ImGui::ColorEdit4("##BgCol", &slider.backgroundColor.r);

			UI::PropertyLabel("Fill Color");
			ImGui::ColorEdit4("##FillCol", &slider.fillColor.r);

			UI::PropertyLabel("Handle Color");
			ImGui::ColorEdit4("##HandleCol", &slider.handleColor.r);

			UI::PropertyLabel("Lua Callback");
			char buf[128];
			strncpy(buf, slider.luaCallback.c_str(), sizeof(buf));
			if (ImGui::InputText("##LuaCB", buf, sizeof(buf)))
				slider.luaCallback = buf;

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UIProgressBarComponent>("UI Progress Bar", registry, selectedEntity, [&](UIProgressBarComponent& pb)
	{
		if (UI::BeginPropertyTable("UIProgressBarTable"))
		{
			UI::PropertyLabel("Value");
			ImGui::SliderFloat("##Val", &pb.value, 0.0f, 1.0f);

			UI::PropertyLabel("Direction");
			const char* dirs[] = { "Left to Right", "Right to Left", "Bottom to Top", "Top to Bottom" };
			int currentDir = static_cast<int>(pb.direction);
			if (ImGui::Combo("##PBarDir", &currentDir, dirs, 4))
				pb.direction = static_cast<UIProgressBarDirection>(currentDir);

			UI::PropertyLabel("Background Color");
			ImGui::ColorEdit4("##BgCol", &pb.backgroundColor.r);

			UI::PropertyLabel("Fill Color");
			ImGui::ColorEdit4("##FillCol", &pb.fillColor.r);

			DrawTextureSlot("Background Tex", "PBarBgTex", pb.backgroundTextureUUID, selectedEntity);
			DrawTextureSlot("Fill Tex", "PBarFillTex", pb.fillTextureUUID, selectedEntity);

			UI::PropertyLabel("Corner Radius");
			ImGui::DragFloat("##PBarRadius", &pb.cornerRadius, 0.5f, 0.0f, 50.0f);

			UI::PropertyLabel("Border Thickness");
			ImGui::DragFloat("##PBarBorderThick", &pb.borderThickness, 0.5f, 0.0f, 20.0f);

			if (pb.borderThickness > 0.0f)
			{
				UI::PropertyLabel("Border Color");
				ImGui::ColorEdit4("##PBarBorderCol", &pb.borderColor.r);
			}

			UI::PropertyLabel("Show Percentage");
			ImGui::Checkbox("##PBarShowPct", &pb.showPercentageText);

			if (pb.showPercentageText)
			{
				UI::PropertyLabel("Percentage Color");
				ImGui::ColorEdit4("##PBarPctCol", &pb.percentageTextColor.r);

				UI::PropertyLabel("Percentage Font Size");
				ImGui::DragFloat("##PBarPctFontSize", &pb.percentageFontSize, 0.5f, 8.0f, 64.0f);
			}

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UICheckboxComponent>("UI Checkbox", registry, selectedEntity, [&](UICheckboxComponent& cb)
	{
		if (UI::BeginPropertyTable("UICheckboxTable"))
		{
			UI::PropertyLabel("Is Checked");
			ImGui::Checkbox("##IsChecked", &cb.isChecked);

			UI::PropertyLabel("Interactable");
			ImGui::Checkbox("##Interactable", &cb.interactable);

			UI::PropertyLabel("Label");
			char labelBuf[256];
			strncpy(labelBuf, cb.label.c_str(), sizeof(labelBuf));
			if (ImGui::InputText("##Label", labelBuf, sizeof(labelBuf)))
				cb.label = labelBuf;

			UI::PropertyLabel("Font Size");
			ImGui::DragFloat("##FontSize", &cb.fontSize, 0.5f, 6.0f, 120.0f);

			UI::PropertyLabel("Box Size");
			ImGui::DragFloat("##BoxSize", &cb.boxSize, 0.5f, 6.0f, 100.0f);

			UI::PropertyLabel("Box Color");
			ImGui::ColorEdit4("##BoxCol", &cb.boxColor.r);

			UI::PropertyLabel("Checkmark Color");
			ImGui::ColorEdit4("##CheckCol", &cb.checkmarkColor.r);

			UI::PropertyLabel("Text Color");
			ImGui::ColorEdit4("##TextCol", &cb.textColor.r);

			UI::PropertyLabel("Lua Callback");
			char buf[128];
			strncpy(buf, cb.luaCallback.c_str(), sizeof(buf));
			if (ImGui::InputText("##LuaCB", buf, sizeof(buf)))
				cb.luaCallback = buf;

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UIPanelComponent>("UI Panel", registry, selectedEntity, [&](UIPanelComponent& panel)
	{
		if (UI::BeginPropertyTable("UIPanelTable"))
		{
			UI::PropertyLabel("Background Color");
			ImGui::ColorEdit4("##PanelBgCol", &panel.backgroundColor.r);

			DrawTextureSlot("Background Tex", "PanelBgTex", panel.backgroundTextureUUID, selectedEntity);

			UI::PropertyLabel("Corner Radius");
			ImGui::DragFloat("##PanelRadius", &panel.cornerRadius, 0.5f, 0.0f, 50.0f);

			UI::PropertyLabel("Border Thickness");
			ImGui::DragFloat("##PanelBorderThick", &panel.borderThickness, 0.5f, 0.0f, 20.0f);

			if (panel.borderThickness > 0.0f)
			{
				UI::PropertyLabel("Border Color");
				ImGui::ColorEdit4("##PanelBorderCol", &panel.borderColor.r);
			}

			UI::PropertyLabel("Clip Children");
			ImGui::Checkbox("##PanelClip", &panel.clipChildren);

			UI::PropertyLabel("Is Draggable");
			ImGui::Checkbox("##PanelDrag", &panel.isDraggable);

			UI::PropertyLabel("Title");
			char titleBuf[256];
			strncpy(titleBuf, panel.title.c_str(), sizeof(titleBuf));
			if (ImGui::InputText("##PanelTitle", titleBuf, sizeof(titleBuf)))
				panel.title = titleBuf;

			if (panel.isDraggable || !panel.title.empty())
			{
				UI::PropertyLabel("Header Height");
				ImGui::DragFloat("##PanelHeaderH", &panel.headerHeight, 1.0f, 10.0f, 100.0f);

				UI::PropertyLabel("Header Color");
				ImGui::ColorEdit4("##PanelHeaderCol", &panel.headerColor.r);

				UI::PropertyLabel("Title Color");
				ImGui::ColorEdit4("##PanelTitleCol", &panel.titleColor.r);

				UI::PropertyLabel("Title Font Size");
				ImGui::DragFloat("##PanelTitleFontSz", &panel.titleFontSize, 0.5f, 8.0f, 64.0f);
			}

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UILayoutGroupComponent>("UI Layout Group", registry, selectedEntity, [&](UILayoutGroupComponent& lg)
	{
		if (UI::BeginPropertyTable("UILayoutGroupTable"))
		{
			UI::PropertyLabel("Type");
			const char* types[] = { "Vertical", "Horizontal" };
			int currentType = static_cast<int>(lg.type);
			if (ImGui::Combo("##LGType", &currentType, types, 2))
				lg.type = static_cast<UILayoutType>(currentType);

			UI::PropertyLabel("Spacing");
			ImGui::DragFloat("##LGSpacing", &lg.spacing, 0.5f, 0.0f, 100.0f);

			UI::PropertyLabel("Padding Left");
			ImGui::DragFloat("##LGPaddingL", &lg.padding.x, 0.5f, 0.0f, 100.0f);
			UI::PropertyLabel("Padding Top");
			ImGui::DragFloat("##LGPaddingT", &lg.padding.y, 0.5f, 0.0f, 100.0f);
			UI::PropertyLabel("Padding Right");
			ImGui::DragFloat("##LGPaddingR", &lg.padding.z, 0.5f, 0.0f, 100.0f);
			UI::PropertyLabel("Padding Bottom");
			ImGui::DragFloat("##LGPaddingB", &lg.padding.w, 0.5f, 0.0f, 100.0f);

			UI::PropertyLabel("Child Alignment");
			const char* aligns[] = { "Start", "Center", "End" };
			int currentAlign = static_cast<int>(lg.alignment);
			if (ImGui::Combo("##LGAlign", &currentAlign, aligns, 3))
				lg.alignment = static_cast<UIChildAlignment>(currentAlign);

			UI::PropertyLabel("Control Child Width");
			ImGui::Checkbox("##LGControlW", &lg.controlChildWidth);

			UI::PropertyLabel("Control Child Height");
			ImGui::Checkbox("##LGControlH", &lg.controlChildHeight);

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UITextInputComponent>("UI Text Input", registry, selectedEntity, [&](UITextInputComponent& input)
	{
		if (UI::BeginPropertyTable("UITextInputTable"))
		{
			UI::PropertyLabel("Text");
			char textBuf[1024];
			strncpy(textBuf, input.text.c_str(), sizeof(textBuf));
			if (ImGui::InputText("##InputText", textBuf, sizeof(textBuf)))
				input.text = textBuf;

			UI::PropertyLabel("Placeholder");
			char phBuf[256];
			strncpy(phBuf, input.placeholder.c_str(), sizeof(phBuf));
			if (ImGui::InputText("##InputPH", phBuf, sizeof(phBuf)))
				input.placeholder = phBuf;

			UI::PropertyLabel("Max Length");
			int maxL = static_cast<int>(input.maxLength);
			if (ImGui::DragInt("##InputMaxL", &maxL, 1, 1, 1024))
				input.maxLength = static_cast<uint32_t>(maxL);

			UI::PropertyLabel("Is Password");
			ImGui::Checkbox("##InputIsPass", &input.isPassword);

			UI::PropertyLabel("Interactable");
			ImGui::Checkbox("##InputInteract", &input.interactable);

			UI::PropertyLabel("Background Color");
			ImGui::ColorEdit4("##InputBgCol", &input.backgroundColor.r);

			UI::PropertyLabel("Border Color");
			ImGui::ColorEdit4("##InputBorderCol", &input.borderColor.r);

			UI::PropertyLabel("Focused Border Color");
			ImGui::ColorEdit4("##InputFocBorderCol", &input.focusedBorderColor.r);

			UI::PropertyLabel("Border Thickness");
			ImGui::DragFloat("##InputBorderThick", &input.borderThickness, 0.5f, 0.0f, 10.0f);

			UI::PropertyLabel("Corner Radius");
			ImGui::DragFloat("##InputRadius", &input.cornerRadius, 0.5f, 0.0f, 50.0f);

			UI::PropertyLabel("Text Color");
			ImGui::ColorEdit4("##InputTextCol", &input.textColor.r);

			UI::PropertyLabel("Placeholder Color");
			ImGui::ColorEdit4("##InputPHCol", &input.placeholderColor.r);

			UI::PropertyLabel("Cursor Color");
			ImGui::ColorEdit4("##InputCursorCol", &input.cursorColor.r);

			UI::PropertyLabel("Font Size");
			ImGui::DragFloat("##InputFontSz", &input.fontSize, 0.5f, 8.0f, 64.0f);

			UI::PropertyLabel("Lua Change Callback");
			char buf[128];
			strncpy(buf, input.luaCallback.c_str(), sizeof(buf));
			if (ImGui::InputText("##InputLuaCB", buf, sizeof(buf)))
				input.luaCallback = buf;

			UI::PropertyLabel("Lua Submit Callback");
			char bufSub[128];
			strncpy(bufSub, input.luaSubmitCallback.c_str(), sizeof(bufSub));
			if (ImGui::InputText("##InputLuaSubCB", bufSub, sizeof(bufSub)))
				input.luaSubmitCallback = bufSub;

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UIScrollRectComponent>("UI Scroll Rect", registry, selectedEntity, [&](UIScrollRectComponent& scroll)
	{
		if (UI::BeginPropertyTable("UIScrollRectTable"))
		{
			UI::PropertyLabel("Vertical");
			ImGui::Checkbox("##ScrollVert", &scroll.vertical);

			UI::PropertyLabel("Horizontal");
			ImGui::Checkbox("##ScrollHoriz", &scroll.horizontal);

			UI::PropertyLabel("Scroll Position");
			ImGui::SliderFloat2("##ScrollPos", &scroll.scrollPosition.x, 0.0f, 1.0f);

			UI::PropertyLabel("Content Size");
			ImGui::DragFloat2("##ScrollContentSz", &scroll.contentSize.x, 1.0f, 10.0f, 8192.0f);

			UI::PropertyLabel("Scroll Speed");
			ImGui::DragFloat("##ScrollSpeed", &scroll.scrollSpeed, 1.0f, 1.0f, 200.0f);

			UI::PropertyLabel("Show Scrollbar");
			ImGui::Checkbox("##ScrollShowBar", &scroll.showScrollbar);

			if (scroll.showScrollbar)
			{
				UI::PropertyLabel("Scrollbar Width");
				ImGui::DragFloat("##ScrollBarW", &scroll.scrollbarWidth, 0.5f, 2.0f, 30.0f);

				UI::PropertyLabel("Track Color");
				ImGui::ColorEdit4("##ScrollTrackCol", &scroll.trackColor.r);

				UI::PropertyLabel("Thumb Color");
				ImGui::ColorEdit4("##ScrollThumbCol", &scroll.thumbColor.r);

				UI::PropertyLabel("Thumb Hover Color");
				ImGui::ColorEdit4("##ScrollThumbHovCol", &scroll.thumbHoverColor.r);

				UI::PropertyLabel("Corner Radius");
				ImGui::DragFloat("##ScrollBarRadius", &scroll.cornerRadius, 0.5f, 0.0f, 20.0f);
			}

			UI::PropertyLabel("Lua Callback");
			char buf[128];
			strncpy(buf, scroll.luaCallback.c_str(), sizeof(buf));
			if (ImGui::InputText("##ScrollLuaCB", buf, sizeof(buf)))
				scroll.luaCallback = buf;

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<UITabContainerComponent>("UI Tab Container", registry, selectedEntity, [&](UITabContainerComponent& tabs)
	{
		if (UI::BeginPropertyTable("UITabContainerTable"))
		{
			UI::PropertyLabel("Active Tab Index");
			int maxIdx = std::max(0, static_cast<int>(tabs.tabTitles.size()) - 1);
			ImGui::SliderInt("##TabsActiveIdx", &tabs.activeTabIndex, 0, maxIdx);

			UI::PropertyLabel("Tab Height");
			ImGui::DragFloat("##TabsHeight", &tabs.tabHeight, 0.5f, 16.0f, 80.0f);

			UI::PropertyLabel("Tab Spacing");
			ImGui::DragFloat("##TabsSpacing", &tabs.tabSpacing, 0.5f, 0.0f, 30.0f);

			UI::PropertyLabel("Tab Normal Color");
			ImGui::ColorEdit4("##TabsNormCol", &tabs.tabNormalColor.r);

			UI::PropertyLabel("Tab Hover Color");
			ImGui::ColorEdit4("##TabsHovCol", &tabs.tabHoverColor.r);

			UI::PropertyLabel("Tab Active Color");
			ImGui::ColorEdit4("##TabsActCol", &tabs.tabActiveColor.r);

			UI::PropertyLabel("Tab Text Color");
			ImGui::ColorEdit4("##TabsTextCol", &tabs.tabTextColor.r);

			UI::PropertyLabel("Tab Font Size");
			ImGui::DragFloat("##TabsFontSz", &tabs.tabFontSize, 0.5f, 8.0f, 64.0f);

			UI::PropertyLabel("Tab Corner Radius");
			ImGui::DragFloat("##TabsRadius", &tabs.tabCornerRadius, 0.5f, 0.0f, 30.0f);

			UI::PropertyLabel("Content BG Color");
			ImGui::ColorEdit4("##TabsContentBgCol", &tabs.contentBackgroundColor.r);

			UI::PropertyLabel("Content Corner Radius");
			ImGui::DragFloat("##TabsContentRadius", &tabs.contentCornerRadius, 0.5f, 0.0f, 30.0f);

			UI::PropertyLabel("Tabs List");
			if (ImGui::Button("+ Add Tab##AddTabBtn", ImVec2(-FLT_MIN, 22.0f)))
			{
				tabs.tabTitles.push_back("Tab " + std::to_string(tabs.tabTitles.size() + 1));
			}

			for (size_t i = 0; i < tabs.tabTitles.size(); i++)
			{
				char itemBuf[128];
				strncpy(itemBuf, tabs.tabTitles[i].c_str(), sizeof(itemBuf));
				std::string itemLabel = "Tab " + std::to_string(i + 1);
				UI::PropertyLabel(itemLabel.c_str());

				std::string inputId = "##TabTitle" + std::to_string(i);
				float delBtnW = 24.0f;
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - delBtnW - 4.0f);
				if (ImGui::InputText(inputId.c_str(), itemBuf, sizeof(itemBuf)))
				{
					tabs.tabTitles[i] = itemBuf;
				}

				ImGui::SameLine();
				std::string delId = "X##DelTab" + std::to_string(i);
				if (ImGui::Button(delId.c_str(), ImVec2(delBtnW, 20.0f)))
				{
					tabs.tabTitles.erase(tabs.tabTitles.begin() + i);
					if (tabs.activeTabIndex >= (int)tabs.tabTitles.size())
						tabs.activeTabIndex = std::max(0, (int)tabs.tabTitles.size() - 1);
					break;
				}
			}

			UI::PropertyLabel("Lua Callback");
			char buf[128];
			strncpy(buf, tabs.luaCallback.c_str(), sizeof(buf));
			if (ImGui::InputText("##TabsLuaCB", buf, sizeof(buf)))
				tabs.luaCallback = buf;

			UI::EndPropertyTable();
		}
	});
}

}
