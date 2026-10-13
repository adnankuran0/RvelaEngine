#include "UIInspectors.h"
#include "InspectorUIUtils.h"
#include "InspectorContext.h"
#include "ImGui/imgui.h"
#include "EditorUtils.h"
#include "Scene/Entity.h"
#include <cstring>
#include <string>

namespace rv {

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

			UI::PropertyLabel("Texture");
			std::string texName = "None (Texture)";
			if (img.textureUUID.IsValid())
			{
				std::string fn = EditorUtils::GetAssetFileName(img.textureUUID);
				texName = fn.empty() ? "Texture Asset" : fn;
			}

			float clearBtnWidth = img.textureUUID.IsValid() ? 24.0f : 0.0f;
			float slotWidth = img.textureUUID.IsValid() ? (ImGui::GetContentRegionAvail().x - clearBtnWidth - 4.0f) : -FLT_MIN;

			ImGui::Button(texName.c_str(), ImVec2(slotWidth, 22.0f));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
				{
					std::string pathStr((const char*)payload->Data);
					AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
					if (uuid.IsValid())
					{
						InspectorContext::ApplyEntityChange(selectedEntity, "Assign UI Image Texture", [&]() {
							img.textureUUID = uuid;
						});
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (img.textureUUID.IsValid())
			{
				ImGui::SameLine();
				if (ImGui::Button("X##ClearUIImageTex", ImVec2(clearBtnWidth, 22.0f)))
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Clear UI Image Texture", [&]() {
						img.textureUUID = AssetUUID::Invalid();
					});
				}
			}

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

			UI::PropertyLabel("Background");
			ImGui::ColorEdit4("##BgCol", &pb.backgroundColor.r);

			UI::PropertyLabel("Fill Color");
			ImGui::ColorEdit4("##FillCol", &pb.fillColor.r);

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
}

} // namespace rv
