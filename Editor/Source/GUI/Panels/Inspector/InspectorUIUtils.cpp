#include "InspectorUIUtils.h"

namespace rv::UI {

bool BeginPropertyTable(const char* id, float labelRatio)
{
	if (ImGui::BeginTable(id, 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoSavedSettings))
	{
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, labelRatio);
		ImGui::TableSetupColumn("Widget", ImGuiTableColumnFlags_WidthStretch, 1.0f - labelRatio);
		return true;
	}
	return false;
}

void EndPropertyTable()
{
	ImGui::EndTable();
}

void PropertyLabel(const char* label)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 24.0f);
	ImGui::TableSetColumnIndex(0);
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::TableSetColumnIndex(1);
	ImGui::SetNextItemWidth(-FLT_MIN);
}

bool DrawTransformRow(const char* label, glm::vec3& values, float resetValue, float speed, float min, float max, bool isScale, bool* lockRatio)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 26.0f);
	ImGui::TableSetColumnIndex(0);
	ImGui::AlignTextToFramePadding();

	ImGui::PushID(label);

	// Reset button
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.50f, 0.72f, 0.35f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.68f, 0.62f, 0.88f, 0.50f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	float btnH = ImGui::GetFrameHeight();
	bool changed = false;

	if (ImGui::Button("R", ImVec2(btnH, btnH)))
	{
		values = glm::vec3(resetValue);
		changed = true;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reset %s to (%.2f, %.2f, %.2f)", label, resetValue, resetValue, resetValue);
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3);

	ImGui::SameLine(0, 4.0f);
	ImGui::TextUnformatted(label);

	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("RowContextMenu");
	}

	// Lock
	if (isScale && lockRatio)
	{
		float availCol0 = ImGui::GetContentRegionAvail().x;
		float lockSize = btnH;
		if (availCol0 > lockSize + 4.0f)
			ImGui::SameLine(ImGui::GetCursorPosX() + availCol0 - lockSize);
		else
			ImGui::SameLine(0, 6.0f);

		bool locked = *lockRatio;
		ImTextureID lockTex = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(locked ? EditorIcon::LockClosed : EditorIcon::LockOpen).GetID();

		ImVec4 lockBtnCol = locked ? ImVec4(0.24f, 0.45f, 0.78f, 0.65f) : ImVec4(0, 0, 0, 0);
		ImVec4 lockBtnHov = locked ? ImVec4(0.32f, 0.55f, 0.88f, 0.85f) : ImVec4(0.55f, 0.50f, 0.72f, 0.30f);
		ImGui::PushStyleColor(ImGuiCol_Button, lockBtnCol);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, lockBtnHov);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.40f, 0.62f, 0.95f, 0.95f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 3.0f));

		const ImVec2 iconSize(lockSize - 6.0f, lockSize - 6.0f);
		const ImVec4 tintCol = locked ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.70f, 0.70f, 0.75f, 0.85f);

		if (ImGui::ImageButton("##lockRatioBtn", lockTex, iconSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tintCol))
		{
			*lockRatio = !locked;
		}
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(locked ? "Scale proportions locked (Uniform scaling)\nClick to unlock"
									 : "Scale proportions unlocked\nClick to lock proportions");
		}
	}

	auto& clipboard = InspectorContext::GetTransformClipboard();

	if (ImGui::BeginPopup("RowContextMenu"))
	{
		if (ImGui::MenuItem(("Reset " + std::string(label)).c_str()))
		{
			values = glm::vec3(resetValue);
			changed = true;
		}
		ImGui::Separator();
		if (ImGui::MenuItem(("Copy " + std::string(label)).c_str()))
		{
			clipboard.vec3Value = values;
			clipboard.hasVec3 = true;
		}
		if (ImGui::MenuItem(("Paste " + std::string(label)).c_str(), nullptr, false, clipboard.hasVec3))
		{
			values = clipboard.vec3Value;
			changed = true;
		}
		if (isScale && lockRatio)
		{
			ImGui::Separator();
			if (ImGui::MenuItem("Lock Scale Proportions", nullptr, lockRatio))
			{
			}
		}
		ImGui::EndPopup();
	}

	// Values
	ImGui::TableSetColumnIndex(1);

	ImGuiStyle& style = ImGui::GetStyle();
	float buttonWidth = ImGui::GetFrameHeight();
	float totalWidth = ImGui::GetContentRegionAvail().x;
	float itemSpacing = style.ItemSpacing.x;
	float inputWidth = std::max((totalWidth - (buttonWidth * 3.0f) - (itemSpacing * 5.0f)) / 3.0f, 1.0f);

	glm::vec3 oldVals = values;
	bool chX = false, chY = false, chZ = false;

	auto DrawAxis = [&](const char* axisLabel, float& val, bool& axisChanged,
		const ImVec4& colNormal, const ImVec4& colHover, const ImVec4& colActive)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, colNormal);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colActive);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		if (ImGui::Button(axisLabel, ImVec2(buttonWidth, buttonWidth)))
		{
			if (isScale && lockRatio && *lockRatio)
				values = glm::vec3(resetValue);
			else
				val = resetValue;
			axisChanged = true;
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);

		ImGui::SameLine(0, 2.0f);
		ImGui::SetNextItemWidth(inputWidth);
		std::string dragLabel = std::string("##") + axisLabel;
		if (ImGui::DragFloat(dragLabel.c_str(), &val, speed, min, max, "%.2f"))
		{
			axisChanged = true;
		}
	};

	DrawAxis("X", values.x, chX, ImVec4(0.78f, 0.22f, 0.22f, 1.0f), ImVec4(0.90f, 0.32f, 0.32f, 1.0f), ImVec4(0.68f, 0.16f, 0.16f, 1.0f));
	ImGui::SameLine(0, itemSpacing);
	DrawAxis("Y", values.y, chY, ImVec4(0.22f, 0.70f, 0.22f, 1.0f), ImVec4(0.32f, 0.82f, 0.32f, 1.0f), ImVec4(0.16f, 0.60f, 0.16f, 1.0f));
	ImGui::SameLine(0, itemSpacing);
	DrawAxis("Z", values.z, chZ, ImVec4(0.22f, 0.45f, 0.88f, 1.0f), ImVec4(0.32f, 0.58f, 0.98f, 1.0f), ImVec4(0.16f, 0.36f, 0.78f, 1.0f));

	if (isScale && lockRatio && *lockRatio)
	{
		if (chX)
		{
			float ratio = (oldVals.x != 0.0f) ? (values.x / oldVals.x) : 1.0f;
			if (std::abs(oldVals.x) < 0.0001f)
			{
				values.y = values.x;
				values.z = values.x;
			}
			else
			{
				values.y = std::clamp(oldVals.y * ratio, min, max);
				values.z = std::clamp(oldVals.z * ratio, min, max);
			}
		}
		else if (chY)
		{
			float ratio = (oldVals.y != 0.0f) ? (values.y / oldVals.y) : 1.0f;
			if (std::abs(oldVals.y) < 0.0001f)
			{
				values.x = values.y;
				values.z = values.y;
			}
			else
			{
				values.x = std::clamp(oldVals.x * ratio, min, max);
				values.z = std::clamp(oldVals.z * ratio, min, max);
			}
		}
		else if (chZ)
		{
			float ratio = (oldVals.z != 0.0f) ? (values.z / oldVals.z) : 1.0f;
			if (std::abs(oldVals.z) < 0.0001f)
			{
				values.x = values.z;
				values.y = values.z;
			}
			else
			{
				values.x = std::clamp(oldVals.x * ratio, min, max);
				values.y = std::clamp(oldVals.y * ratio, min, max);
			}
		}
	}

	ImGui::PopID();
	return changed || chX || chY || chZ || (values != oldVals);
}

void DrawVec3Control(const char* label, glm::vec3& values, float resetValue, float speed, float min, float max)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 24.0f);
	ImGui::TableSetColumnIndex(0);
	ImGui::AlignTextToFramePadding();

	ImGui::PushID(label);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.50f, 0.72f, 0.35f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.68f, 0.62f, 0.88f, 0.50f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	if (ImGui::Button("R", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
	{
		values = glm::vec3(resetValue);
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3);

	ImGui::SameLine(0, 4.0f);
	ImGui::TextUnformatted(label);

	ImGui::TableSetColumnIndex(1);

	ImGuiStyle& style = ImGui::GetStyle();
	float buttonWidth = ImGui::GetFrameHeight();
	float totalWidth = ImGui::GetContentRegionAvail().x;
	float itemSpacing = style.ItemSpacing.x;
	float inputWidth = std::max((totalWidth - (buttonWidth * 3.0f) - (itemSpacing * 5.0f)) / 3.0f, 1.0f);

	auto DrawAxis = [&](const char* axisLabel, float& val, const ImVec4& colNormal, const ImVec4& colHover, const ImVec4& colActive)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, colNormal);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colActive);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		if (ImGui::Button(axisLabel, ImVec2(buttonWidth, buttonWidth)))
			val = resetValue;
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);

		ImGui::SameLine(0, 2.0f);
		ImGui::SetNextItemWidth(inputWidth);
		std::string dragLabel = std::string("##") + axisLabel;
		ImGui::DragFloat(dragLabel.c_str(), &val, speed, min, max, "%.2f");
	};

	DrawAxis("X", values.x, ImVec4(0.75f, 0.20f, 0.20f, 1.0f), ImVec4(0.85f, 0.30f, 0.30f, 1.0f), ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
	ImGui::SameLine(0, itemSpacing);
	DrawAxis("Y", values.y, ImVec4(0.20f, 0.70f, 0.20f, 1.0f), ImVec4(0.30f, 0.80f, 0.30f, 1.0f), ImVec4(0.15f, 0.60f, 0.15f, 1.0f));
	ImGui::SameLine(0, itemSpacing);
	DrawAxis("Z", values.z, ImVec4(0.20f, 0.45f, 0.85f, 1.0f), ImVec4(0.30f, 0.55f, 0.95f, 1.0f), ImVec4(0.15f, 0.35f, 0.75f, 1.0f));

	ImGui::PopID();
}

void DrawVec2Control(const char* label, glm::vec2& values, float resetValue, float speed, float min, float max)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 24.0f);
	ImGui::TableSetColumnIndex(0);
	ImGui::AlignTextToFramePadding();

	ImGui::PushID(label);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.50f, 0.72f, 0.35f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.68f, 0.62f, 0.88f, 0.50f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	if (ImGui::Button("R", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
	{
		values = glm::vec2(resetValue);
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3);

	ImGui::SameLine(0, 4.0f);
	ImGui::TextUnformatted(label);

	ImGui::TableSetColumnIndex(1);

	ImGuiStyle& style = ImGui::GetStyle();
	float buttonWidth = ImGui::GetFrameHeight();
	float totalWidth = ImGui::GetContentRegionAvail().x;
	float itemSpacing = style.ItemSpacing.x;
	float inputWidth = std::max((totalWidth - (buttonWidth * 2.0f) - (itemSpacing * 3.0f)) / 2.0f, 1.0f);

	auto DrawAxis = [&](const char* axisLabel, float& val, const ImVec4& colNormal, const ImVec4& colHover, const ImVec4& colActive)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, colNormal);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colHover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colActive);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		if (ImGui::Button(axisLabel, ImVec2(buttonWidth, buttonWidth)))
			val = resetValue;
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);

		ImGui::SameLine(0, 2.0f);
		ImGui::SetNextItemWidth(inputWidth);
		std::string dragLabel = std::string("##") + axisLabel;
		ImGui::DragFloat(dragLabel.c_str(), &val, speed, min, max, "%.2f");
	};

	DrawAxis("X", values.x, ImVec4(0.75f, 0.20f, 0.20f, 1.0f), ImVec4(0.85f, 0.30f, 0.30f, 1.0f), ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
	ImGui::SameLine(0, itemSpacing);
	DrawAxis("Y", values.y, ImVec4(0.20f, 0.70f, 0.20f, 1.0f), ImVec4(0.30f, 0.80f, 0.30f, 1.0f), ImVec4(0.15f, 0.60f, 0.15f, 1.0f));

	ImGui::PopID();
}

} // namespace rv::UI
