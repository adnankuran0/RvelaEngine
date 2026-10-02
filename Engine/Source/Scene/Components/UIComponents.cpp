#include "rvelapch.h"
#include "UIComponents.h"

namespace rv {

glm::vec4 RectTransformComponent::CalculateScreenRect(const glm::vec2& parentPos, const glm::vec2& parentSize) const
{
    float anchorLeft = parentPos.x + anchorMin.x * parentSize.x;
    float anchorRight = parentPos.x + anchorMax.x * parentSize.x;
    float anchorBottom = parentPos.y + anchorMin.y * parentSize.y;
    float anchorTop = parentPos.y + anchorMax.y * parentSize.y;

    float width = (anchorMin.x == anchorMax.x) ? size.x * scale.x : (anchorRight - anchorLeft) + size.x * scale.x;
    float height = (anchorMin.y == anchorMax.y) ? size.y * scale.y : (anchorTop - anchorBottom) + size.y * scale.y;

    float pivotX = anchorLeft + position.x - (pivot.x * width);
    float pivotY = anchorBottom + position.y - (pivot.y * height);

    return glm::vec4(pivotX, pivotY, pivotX + width, pivotY + height);
}

json UICanvasComponent::Serialize() const
{
    json j;
    j["mode"] = static_cast<int>(mode);
    j["referenceResolution"] = { referenceResolution.x, referenceResolution.y };
    j["matchWidthOrHeight"] = matchWidthOrHeight;
    j["sortOrder"] = sortOrder;
    j["depthTest"] = depthTest;
    j["billboardMode"] = static_cast<int>(billboardMode);
    j["constantScreenSize"] = constantScreenSize;
    j["constantScaleFactor"] = constantScaleFactor;
    return j;
}

void UICanvasComponent::Deserialize(const json& j)
{
    if (j.contains("mode")) mode = static_cast<CanvasMode>(j["mode"].get<int>());
    if (j.contains("referenceResolution")) {
        referenceResolution.x = j["referenceResolution"][0];
        referenceResolution.y = j["referenceResolution"][1];
    }
    if (j.contains("matchWidthOrHeight")) matchWidthOrHeight = j["matchWidthOrHeight"];
    if (j.contains("sortOrder")) sortOrder = j["sortOrder"];
    if (j.contains("depthTest")) depthTest = j["depthTest"];
    if (j.contains("billboardMode")) billboardMode = static_cast<BillboardMode>(j["billboardMode"].get<int>());
    if (j.contains("constantScreenSize")) constantScreenSize = j["constantScreenSize"];
    if (j.contains("constantScaleFactor")) constantScaleFactor = j["constantScaleFactor"];
}

json RectTransformComponent::Serialize() const
{
    json j;
    j["position"] = { position.x, position.y };
    j["size"] = { size.x, size.y };
    j["anchorMin"] = { anchorMin.x, anchorMin.y };
    j["anchorMax"] = { anchorMax.x, anchorMax.y };
    j["pivot"] = { pivot.x, pivot.y };
    j["rotation"] = rotation;
    j["scale"] = { scale.x, scale.y };
    j["raycastTarget"] = raycastTarget;
    return j;
}

void RectTransformComponent::Deserialize(const json& j)
{
    if (j.contains("position")) { position.x = j["position"][0]; position.y = j["position"][1]; }
    if (j.contains("size")) { size.x = j["size"][0]; size.y = j["size"][1]; }
    if (j.contains("anchorMin")) { anchorMin.x = j["anchorMin"][0]; anchorMin.y = j["anchorMin"][1]; }
    if (j.contains("anchorMax")) { anchorMax.x = j["anchorMax"][0]; anchorMax.y = j["anchorMax"][1]; }
    if (j.contains("pivot")) { pivot.x = j["pivot"][0]; pivot.y = j["pivot"][1]; }
    if (j.contains("rotation")) rotation = j["rotation"];
    if (j.contains("scale")) { scale.x = j["scale"][0]; scale.y = j["scale"][1]; }
    if (j.contains("raycastTarget")) raycastTarget = j["raycastTarget"];
}

json UIImageComponent::Serialize() const
{
    json j;
    j["color"] = { color.r, color.g, color.b, color.a };
    j["textureUUID"] = textureUUID.ToString();
    j["texturePath"] = texturePath;
    j["imageType"] = static_cast<int>(imageType);
    j["cornerRadius"] = cornerRadius;
    return j;
}

void UIImageComponent::Deserialize(const json& j)
{
    if (j.contains("color")) {
        color.r = j["color"][0]; color.g = j["color"][1]; color.b = j["color"][2]; color.a = j["color"][3];
    }
    if (j.contains("textureUUID")) textureUUID = AssetUUID::FromString(j["textureUUID"].get<std::string>());
    if (j.contains("texturePath")) texturePath = j["texturePath"].get<std::string>();
    if (j.contains("imageType")) imageType = static_cast<ImageType>(j["imageType"].get<int>());
    if (j.contains("cornerRadius")) cornerRadius = j["cornerRadius"];
}

json UITextComponent::Serialize() const
{
    json j;
    j["text"] = text;
    j["color"] = { color.r, color.g, color.b, color.a };
    j["fontSize"] = fontSize;
    j["alignment"] = static_cast<int>(alignment);
    j["fontPath"] = fontPath;
    j["fontUUID"] = fontUUID.ToString();
    j["isBold"] = isBold;
    j["isOutline"] = isOutline;
    j["outlineSize"] = outlineSize;
    j["outlineColor"] = { outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a };
    j["wordWrap"] = wordWrap;
    j["lineSpacing"] = lineSpacing;
    j["characterSpacing"] = characterSpacing;
    return j;
}

void UITextComponent::Deserialize(const json& j)
{
    if (j.contains("text")) text = j["text"].get<std::string>();
    if (j.contains("color")) {
        color.r = j["color"][0]; color.g = j["color"][1]; color.b = j["color"][2]; color.a = j["color"][3];
    }
    if (j.contains("fontSize")) fontSize = j["fontSize"];
    if (j.contains("alignment")) alignment = static_cast<TextAlignment>(j["alignment"].get<int>());
    if (j.contains("fontPath")) fontPath = j["fontPath"].get<std::string>();
    if (j.contains("fontUUID")) fontUUID = AssetUUID::FromString(j["fontUUID"].get<std::string>());
    if (j.contains("isBold")) isBold = j["isBold"];
    if (j.contains("isOutline")) isOutline = j["isOutline"];
    if (j.contains("outlineSize")) outlineSize = j["outlineSize"];
    if (j.contains("outlineColor")) { outlineColor.r = j["outlineColor"][0]; outlineColor.g = j["outlineColor"][1]; outlineColor.b = j["outlineColor"][2]; outlineColor.a = j["outlineColor"][3]; }
    if (j.contains("wordWrap")) wordWrap = j["wordWrap"];
    if (j.contains("lineSpacing")) lineSpacing = j["lineSpacing"];
    if (j.contains("characterSpacing")) characterSpacing = j["characterSpacing"];
}

json UIButtonComponent::Serialize() const
{
    json j;
    j["normalColor"] = { normalColor.r, normalColor.g, normalColor.b, normalColor.a };
    j["hoverColor"] = { hoverColor.r, hoverColor.g, hoverColor.b, hoverColor.a };
    j["pressedColor"] = { pressedColor.r, pressedColor.g, pressedColor.b, pressedColor.a };
    j["disabledColor"] = { disabledColor.r, disabledColor.g, disabledColor.b, disabledColor.a };
    j["cornerRadius"] = cornerRadius;
    j["interactable"] = interactable;
    j["text"] = text;
    j["textColor"] = { textColor.r, textColor.g, textColor.b, textColor.a };
    j["fontSize"] = fontSize;
    j["fontPath"] = fontPath;
    j["isBold"] = isBold;
    j["isOutline"] = isOutline;
    j["outlineSize"] = outlineSize;
    j["outlineColor"] = { outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a };
    j["luaCallback"] = luaCallback;
    return j;
}

void UIButtonComponent::Deserialize(const json& j)
{
    if (j.contains("normalColor")) { normalColor.r = j["normalColor"][0]; normalColor.g = j["normalColor"][1]; normalColor.b = j["normalColor"][2]; normalColor.a = j["normalColor"][3]; }
    if (j.contains("hoverColor")) { hoverColor.r = j["hoverColor"][0]; hoverColor.g = j["hoverColor"][1]; hoverColor.b = j["hoverColor"][2]; hoverColor.a = j["hoverColor"][3]; }
    if (j.contains("pressedColor")) { pressedColor.r = j["pressedColor"][0]; pressedColor.g = j["pressedColor"][1]; pressedColor.b = j["pressedColor"][2]; pressedColor.a = j["pressedColor"][3]; }
    if (j.contains("disabledColor")) { disabledColor.r = j["disabledColor"][0]; disabledColor.g = j["disabledColor"][1]; disabledColor.b = j["disabledColor"][2]; disabledColor.a = j["disabledColor"][3]; }
    if (j.contains("cornerRadius")) cornerRadius = j["cornerRadius"];
    if (j.contains("interactable")) interactable = j["interactable"];
    if (j.contains("text")) text = j["text"].get<std::string>();
    if (j.contains("textColor")) { textColor.r = j["textColor"][0]; textColor.g = j["textColor"][1]; textColor.b = j["textColor"][2]; textColor.a = j["textColor"][3]; }
    if (j.contains("fontSize")) fontSize = j["fontSize"];
    if (j.contains("fontPath")) fontPath = j["fontPath"].get<std::string>();
    if (j.contains("isBold")) isBold = j["isBold"];
    if (j.contains("isOutline")) isOutline = j["isOutline"];
    if (j.contains("outlineSize")) outlineSize = j["outlineSize"];
    if (j.contains("outlineColor")) { outlineColor.r = j["outlineColor"][0]; outlineColor.g = j["outlineColor"][1]; outlineColor.b = j["outlineColor"][2]; outlineColor.a = j["outlineColor"][3]; }
    if (j.contains("luaCallback")) luaCallback = j["luaCallback"].get<std::string>();
}

json UISliderComponent::Serialize() const
{
    json j;
    j["minValue"] = minValue;
    j["maxValue"] = maxValue;
    j["value"] = value;
    j["interactable"] = interactable;
    j["backgroundColor"] = { backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a };
    j["fillColor"] = { fillColor.r, fillColor.g, fillColor.b, fillColor.a };
    j["handleColor"] = { handleColor.r, handleColor.g, handleColor.b, handleColor.a };
    j["luaCallback"] = luaCallback;
    return j;
}

void UISliderComponent::Deserialize(const json& j)
{
    if (j.contains("minValue")) minValue = j["minValue"];
    if (j.contains("maxValue")) maxValue = j["maxValue"];
    if (j.contains("value")) value = j["value"];
    if (j.contains("interactable")) interactable = j["interactable"];
    if (j.contains("backgroundColor")) { backgroundColor.r = j["backgroundColor"][0]; backgroundColor.g = j["backgroundColor"][1]; backgroundColor.b = j["backgroundColor"][2]; backgroundColor.a = j["backgroundColor"][3]; }
    if (j.contains("fillColor")) { fillColor.r = j["fillColor"][0]; fillColor.g = j["fillColor"][1]; fillColor.b = j["fillColor"][2]; fillColor.a = j["fillColor"][3]; }
    if (j.contains("handleColor")) { handleColor.r = j["handleColor"][0]; handleColor.g = j["handleColor"][1]; handleColor.b = j["handleColor"][2]; handleColor.a = j["handleColor"][3]; }
    if (j.contains("luaCallback")) luaCallback = j["luaCallback"].get<std::string>();
}

json UIProgressBarComponent::Serialize() const
{
    json j;
    j["value"] = value;
    j["backgroundColor"] = { backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a };
    j["fillColor"] = { fillColor.r, fillColor.g, fillColor.b, fillColor.a };
    return j;
}

void UIProgressBarComponent::Deserialize(const json& j)
{
    if (j.contains("value")) value = j["value"];
    if (j.contains("backgroundColor")) { backgroundColor.r = j["backgroundColor"][0]; backgroundColor.g = j["backgroundColor"][1]; backgroundColor.b = j["backgroundColor"][2]; backgroundColor.a = j["backgroundColor"][3]; }
    if (j.contains("fillColor")) { fillColor.r = j["fillColor"][0]; fillColor.g = j["fillColor"][1]; fillColor.b = j["fillColor"][2]; fillColor.a = j["fillColor"][3]; }
}

json UICheckboxComponent::Serialize() const
{
    json j;
    j["isChecked"] = isChecked;
    j["interactable"] = interactable;
    j["label"] = label;
    j["fontSize"] = fontSize;
    j["boxSize"] = boxSize;
    j["boxColor"] = { boxColor.r, boxColor.g, boxColor.b, boxColor.a };
    j["checkmarkColor"] = { checkmarkColor.r, checkmarkColor.g, checkmarkColor.b, checkmarkColor.a };
    j["textColor"] = { textColor.r, textColor.g, textColor.b, textColor.a };
    j["luaCallback"] = luaCallback;
    return j;
}

void UICheckboxComponent::Deserialize(const json& j)
{
    if (j.contains("isChecked")) isChecked = j["isChecked"];
    if (j.contains("interactable")) interactable = j["interactable"];
    if (j.contains("label")) label = j["label"].get<std::string>();
    if (j.contains("fontSize")) fontSize = j["fontSize"];
    if (j.contains("boxSize")) boxSize = j["boxSize"];
    if (j.contains("boxColor")) { boxColor.r = j["boxColor"][0]; boxColor.g = j["boxColor"][1]; boxColor.b = j["boxColor"][2]; boxColor.a = j["boxColor"][3]; }
    if (j.contains("checkmarkColor")) { checkmarkColor.r = j["checkmarkColor"][0]; checkmarkColor.g = j["checkmarkColor"][1]; checkmarkColor.b = j["checkmarkColor"][2]; checkmarkColor.a = j["checkmarkColor"][3]; }
    if (j.contains("textColor")) { textColor.r = j["textColor"][0]; textColor.g = j["textColor"][1]; textColor.b = j["textColor"][2]; textColor.a = j["textColor"][3]; }
    if (j.contains("luaCallback")) luaCallback = j["luaCallback"].get<std::string>();
}

}
