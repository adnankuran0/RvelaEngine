#include "rvelapch.h"
#include "UIComponents.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetRegistry.h"

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
    j["imageType"] = static_cast<int>(imageType);
    j["cornerRadius"] = cornerRadius;
    return j;
}

void UIImageComponent::Deserialize(const json& j)
{
    if (j.contains("color")) {
        color.r = j["color"][0]; color.g = j["color"][1]; color.b = j["color"][2]; color.a = j["color"][3];
    }
    if (j.contains("textureUUID")) {
        textureUUID = AssetUUID::FromString(j["textureUUID"].get<std::string>());
    } else if (j.contains("texturePath")) {
        std::string tp = j["texturePath"].get<std::string>();
        if (!tp.empty()) textureUUID = AssetManager::Get().GetRegistry().GetUUID(tp);
    }
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
    j["verticalAlignment"] = static_cast<int>(verticalAlignment);
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
    if (j.contains("verticalAlignment")) verticalAlignment = static_cast<TextVerticalAlignment>(j["verticalAlignment"].get<int>());
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

    j["normalTextureUUID"] = normalTextureUUID.ToString();
    j["hoverTextureUUID"] = hoverTextureUUID.ToString();
    j["pressedTextureUUID"] = pressedTextureUUID.ToString();
    j["disabledTextureUUID"] = disabledTextureUUID.ToString();
    j["imageType"] = static_cast<int>(imageType);
    j["borderSize"] = borderSize;

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

    if (j.contains("normalTextureUUID")) normalTextureUUID = AssetUUID::FromString(j["normalTextureUUID"].get<std::string>());
    if (j.contains("hoverTextureUUID")) hoverTextureUUID = AssetUUID::FromString(j["hoverTextureUUID"].get<std::string>());
    if (j.contains("pressedTextureUUID")) pressedTextureUUID = AssetUUID::FromString(j["pressedTextureUUID"].get<std::string>());
    if (j.contains("disabledTextureUUID")) disabledTextureUUID = AssetUUID::FromString(j["disabledTextureUUID"].get<std::string>());
    if (j.contains("imageType")) imageType = static_cast<ImageType>(j["imageType"].get<int>());
    if (j.contains("borderSize")) borderSize = j["borderSize"];

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

    j["backgroundTextureUUID"] = backgroundTextureUUID.ToString();
    j["fillTextureUUID"] = fillTextureUUID.ToString();
    j["borderColor"] = { borderColor.r, borderColor.g, borderColor.b, borderColor.a };
    j["borderThickness"] = borderThickness;
    j["cornerRadius"] = cornerRadius;
    j["direction"] = static_cast<int>(direction);

    j["showPercentageText"] = showPercentageText;
    j["percentageTextColor"] = { percentageTextColor.r, percentageTextColor.g, percentageTextColor.b, percentageTextColor.a };
    j["percentageFontSize"] = percentageFontSize;
    return j;
}

void UIProgressBarComponent::Deserialize(const json& j)
{
    if (j.contains("value")) value = j["value"];
    if (j.contains("backgroundColor")) { backgroundColor.r = j["backgroundColor"][0]; backgroundColor.g = j["backgroundColor"][1]; backgroundColor.b = j["backgroundColor"][2]; backgroundColor.a = j["backgroundColor"][3]; }
    if (j.contains("fillColor")) { fillColor.r = j["fillColor"][0]; fillColor.g = j["fillColor"][1]; fillColor.b = j["fillColor"][2]; fillColor.a = j["fillColor"][3]; }

    if (j.contains("backgroundTextureUUID")) backgroundTextureUUID = AssetUUID::FromString(j["backgroundTextureUUID"].get<std::string>());
    if (j.contains("fillTextureUUID")) fillTextureUUID = AssetUUID::FromString(j["fillTextureUUID"].get<std::string>());
    if (j.contains("borderColor")) { borderColor.r = j["borderColor"][0]; borderColor.g = j["borderColor"][1]; borderColor.b = j["borderColor"][2]; borderColor.a = j["borderColor"][3]; }
    if (j.contains("borderThickness")) borderThickness = j["borderThickness"];
    if (j.contains("cornerRadius")) cornerRadius = j["cornerRadius"];
    if (j.contains("direction")) direction = static_cast<UIProgressBarDirection>(j["direction"].get<int>());

    if (j.contains("showPercentageText")) showPercentageText = j["showPercentageText"];
    if (j.contains("percentageTextColor")) { percentageTextColor.r = j["percentageTextColor"][0]; percentageTextColor.g = j["percentageTextColor"][1]; percentageTextColor.b = j["percentageTextColor"][2]; percentageTextColor.a = j["percentageTextColor"][3]; }
    if (j.contains("percentageFontSize")) percentageFontSize = j["percentageFontSize"];
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

json UIPanelComponent::Serialize() const
{
    json j;
    j["backgroundColor"] = { backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a };
    j["borderColor"] = { borderColor.r, borderColor.g, borderColor.b, borderColor.a };
    j["borderThickness"] = borderThickness;
    j["cornerRadius"] = cornerRadius;
    j["backgroundTextureUUID"] = backgroundTextureUUID.ToString();
    j["imageType"] = static_cast<int>(imageType);
    j["clipChildren"] = clipChildren;

    j["isDraggable"] = isDraggable;
    j["headerHeight"] = headerHeight;
    j["title"] = title;
    j["headerColor"] = { headerColor.r, headerColor.g, headerColor.b, headerColor.a };
    j["titleColor"] = { titleColor.r, titleColor.g, titleColor.b, titleColor.a };
    j["titleFontSize"] = titleFontSize;
    return j;
}

void UIPanelComponent::Deserialize(const json& j)
{
    if (j.contains("backgroundColor")) { backgroundColor.r = j["backgroundColor"][0]; backgroundColor.g = j["backgroundColor"][1]; backgroundColor.b = j["backgroundColor"][2]; backgroundColor.a = j["backgroundColor"][3]; }
    if (j.contains("borderColor")) { borderColor.r = j["borderColor"][0]; borderColor.g = j["borderColor"][1]; borderColor.b = j["borderColor"][2]; borderColor.a = j["borderColor"][3]; }
    if (j.contains("borderThickness")) borderThickness = j["borderThickness"];
    if (j.contains("cornerRadius")) cornerRadius = j["cornerRadius"];
    if (j.contains("backgroundTextureUUID")) backgroundTextureUUID = AssetUUID::FromString(j["backgroundTextureUUID"].get<std::string>());
    if (j.contains("imageType")) imageType = static_cast<ImageType>(j["imageType"].get<int>());
    if (j.contains("clipChildren")) clipChildren = j["clipChildren"];

    if (j.contains("isDraggable")) isDraggable = j["isDraggable"];
    if (j.contains("headerHeight")) headerHeight = j["headerHeight"];
    if (j.contains("title")) title = j["title"].get<std::string>();
    if (j.contains("headerColor")) { headerColor.r = j["headerColor"][0]; headerColor.g = j["headerColor"][1]; headerColor.b = j["headerColor"][2]; headerColor.a = j["headerColor"][3]; }
    if (j.contains("titleColor")) { titleColor.r = j["titleColor"][0]; titleColor.g = j["titleColor"][1]; titleColor.b = j["titleColor"][2]; titleColor.a = j["titleColor"][3]; }
    if (j.contains("titleFontSize")) titleFontSize = j["titleFontSize"];
}

json UILayoutGroupComponent::Serialize() const
{
    json j;
    j["type"] = static_cast<int>(type);
    j["spacing"] = spacing;
    j["padding"] = { padding.x, padding.y, padding.z, padding.w };
    j["alignment"] = static_cast<int>(alignment);
    j["fitChildren"] = fitChildren;
    j["controlChildWidth"] = controlChildWidth;
    j["controlChildHeight"] = controlChildHeight;
    return j;
}

void UILayoutGroupComponent::Deserialize(const json& j)
{
    if (j.contains("type")) type = static_cast<UILayoutType>(j["type"].get<int>());
    if (j.contains("spacing")) spacing = j["spacing"];
    if (j.contains("padding")) { padding.x = j["padding"][0]; padding.y = j["padding"][1]; padding.z = j["padding"][2]; padding.w = j["padding"][3]; }
    if (j.contains("alignment")) alignment = static_cast<UIChildAlignment>(j["alignment"].get<int>());
    if (j.contains("fitChildren")) fitChildren = j["fitChildren"];
    if (j.contains("controlChildWidth")) controlChildWidth = j["controlChildWidth"];
    if (j.contains("controlChildHeight")) controlChildHeight = j["controlChildHeight"];
}

json UITextInputComponent::Serialize() const
{
    json j;
    j["text"] = text;
    j["placeholder"] = placeholder;
    j["maxLength"] = maxLength;
    j["isPassword"] = isPassword;
    j["interactable"] = interactable;

    j["backgroundColor"] = { backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a };
    j["borderColor"] = { borderColor.r, borderColor.g, borderColor.b, borderColor.a };
    j["focusedBorderColor"] = { focusedBorderColor.r, focusedBorderColor.g, focusedBorderColor.b, focusedBorderColor.a };
    j["borderThickness"] = borderThickness;
    j["cornerRadius"] = cornerRadius;

    j["textColor"] = { textColor.r, textColor.g, textColor.b, textColor.a };
    j["placeholderColor"] = { placeholderColor.r, placeholderColor.g, placeholderColor.b, placeholderColor.a };
    j["cursorColor"] = { cursorColor.r, cursorColor.g, cursorColor.b, cursorColor.a };
    j["fontSize"] = fontSize;
    j["textPadding"] = { textPadding.x, textPadding.y };

    j["luaCallback"] = luaCallback;
    j["luaSubmitCallback"] = luaSubmitCallback;
    return j;
}

void UITextInputComponent::Deserialize(const json& j)
{
    if (j.contains("text")) text = j["text"].get<std::string>();
    if (j.contains("placeholder")) placeholder = j["placeholder"].get<std::string>();
    if (j.contains("maxLength")) maxLength = j["maxLength"];
    if (j.contains("isPassword")) isPassword = j["isPassword"];
    if (j.contains("interactable")) interactable = j["interactable"];

    if (j.contains("backgroundColor")) { backgroundColor.r = j["backgroundColor"][0]; backgroundColor.g = j["backgroundColor"][1]; backgroundColor.b = j["backgroundColor"][2]; backgroundColor.a = j["backgroundColor"][3]; }
    if (j.contains("borderColor")) { borderColor.r = j["borderColor"][0]; borderColor.g = j["borderColor"][1]; borderColor.b = j["borderColor"][2]; borderColor.a = j["borderColor"][3]; }
    if (j.contains("focusedBorderColor")) { focusedBorderColor.r = j["focusedBorderColor"][0]; focusedBorderColor.g = j["focusedBorderColor"][1]; focusedBorderColor.b = j["focusedBorderColor"][2]; focusedBorderColor.a = j["focusedBorderColor"][3]; }
    if (j.contains("borderThickness")) borderThickness = j["borderThickness"];
    if (j.contains("cornerRadius")) cornerRadius = j["cornerRadius"];

    if (j.contains("textColor")) { textColor.r = j["textColor"][0]; textColor.g = j["textColor"][1]; textColor.b = j["textColor"][2]; textColor.a = j["textColor"][3]; }
    if (j.contains("placeholderColor")) { placeholderColor.r = j["placeholderColor"][0]; placeholderColor.g = j["placeholderColor"][1]; placeholderColor.b = j["placeholderColor"][2]; placeholderColor.a = j["placeholderColor"][3]; }
    if (j.contains("cursorColor")) { cursorColor.r = j["cursorColor"][0]; cursorColor.g = j["cursorColor"][1]; cursorColor.b = j["cursorColor"][2]; cursorColor.a = j["cursorColor"][3]; }
    if (j.contains("fontSize")) fontSize = j["fontSize"];
    if (j.contains("textPadding")) { textPadding.x = j["textPadding"][0]; textPadding.y = j["textPadding"][1]; }

    if (j.contains("luaCallback")) luaCallback = j["luaCallback"].get<std::string>();
    if (j.contains("luaSubmitCallback")) luaSubmitCallback = j["luaSubmitCallback"].get<std::string>();
}

json UIScrollRectComponent::Serialize() const
{
    json j;
    j["horizontal"] = horizontal;
    j["vertical"] = vertical;
    j["scrollPosition"] = { scrollPosition.x, scrollPosition.y };
    j["contentSize"] = { contentSize.x, contentSize.y };
    j["scrollSpeed"] = scrollSpeed;

    j["showScrollbar"] = showScrollbar;
    j["scrollbarWidth"] = scrollbarWidth;
    j["trackColor"] = { trackColor.r, trackColor.g, trackColor.b, trackColor.a };
    j["thumbColor"] = { thumbColor.r, thumbColor.g, thumbColor.b, thumbColor.a };
    j["thumbHoverColor"] = { thumbHoverColor.r, thumbHoverColor.g, thumbHoverColor.b, thumbHoverColor.a };
    j["cornerRadius"] = cornerRadius;

    j["luaCallback"] = luaCallback;
    return j;
}

void UIScrollRectComponent::Deserialize(const json& j)
{
    if (j.contains("horizontal")) horizontal = j["horizontal"];
    if (j.contains("vertical")) vertical = j["vertical"];
    if (j.contains("scrollPosition")) { scrollPosition.x = j["scrollPosition"][0]; scrollPosition.y = j["scrollPosition"][1]; }
    if (j.contains("contentSize")) { contentSize.x = j["contentSize"][0]; contentSize.y = j["contentSize"][1]; }
    if (j.contains("scrollSpeed")) scrollSpeed = j["scrollSpeed"];

    if (j.contains("showScrollbar")) showScrollbar = j["showScrollbar"];
    if (j.contains("scrollbarWidth")) scrollbarWidth = j["scrollbarWidth"];
    if (j.contains("trackColor")) { trackColor.r = j["trackColor"][0]; trackColor.g = j["trackColor"][1]; trackColor.b = j["trackColor"][2]; trackColor.a = j["trackColor"][3]; }
    if (j.contains("thumbColor")) { thumbColor.r = j["thumbColor"][0]; thumbColor.g = j["thumbColor"][1]; thumbColor.b = j["thumbColor"][2]; thumbColor.a = j["thumbColor"][3]; }
    if (j.contains("thumbHoverColor")) { thumbHoverColor.r = j["thumbHoverColor"][0]; thumbHoverColor.g = j["thumbHoverColor"][1]; thumbHoverColor.b = j["thumbHoverColor"][2]; thumbHoverColor.a = j["thumbHoverColor"][3]; }
    if (j.contains("cornerRadius")) cornerRadius = j["cornerRadius"];

    if (j.contains("luaCallback")) luaCallback = j["luaCallback"].get<std::string>();
}

json UITabContainerComponent::Serialize() const
{
    json j;
    j["tabTitles"] = tabTitles;
    j["activeTabIndex"] = activeTabIndex;
    j["tabHeight"] = tabHeight;
    j["tabSpacing"] = tabSpacing;

    j["tabNormalColor"] = { tabNormalColor.r, tabNormalColor.g, tabNormalColor.b, tabNormalColor.a };
    j["tabHoverColor"] = { tabHoverColor.r, tabHoverColor.g, tabHoverColor.b, tabHoverColor.a };
    j["tabActiveColor"] = { tabActiveColor.r, tabActiveColor.g, tabActiveColor.b, tabActiveColor.a };
    j["tabTextColor"] = { tabTextColor.r, tabTextColor.g, tabTextColor.b, tabTextColor.a };
    j["tabFontSize"] = tabFontSize;
    j["tabCornerRadius"] = tabCornerRadius;

    j["contentBackgroundColor"] = { contentBackgroundColor.r, contentBackgroundColor.g, contentBackgroundColor.b, contentBackgroundColor.a };
    j["contentCornerRadius"] = contentCornerRadius;

    j["luaCallback"] = luaCallback;
    return j;
}

void UITabContainerComponent::Deserialize(const json& j)
{
    if (j.contains("tabTitles")) tabTitles = j["tabTitles"].get<std::vector<std::string>>();
    if (j.contains("activeTabIndex")) activeTabIndex = j["activeTabIndex"];
    if (j.contains("tabHeight")) tabHeight = j["tabHeight"];
    if (j.contains("tabSpacing")) tabSpacing = j["tabSpacing"];

    if (j.contains("tabNormalColor")) { tabNormalColor.r = j["tabNormalColor"][0]; tabNormalColor.g = j["tabNormalColor"][1]; tabNormalColor.b = j["tabNormalColor"][2]; tabNormalColor.a = j["tabNormalColor"][3]; }
    if (j.contains("tabHoverColor")) { tabHoverColor.r = j["tabHoverColor"][0]; tabHoverColor.g = j["tabHoverColor"][1]; tabHoverColor.b = j["tabHoverColor"][2]; tabHoverColor.a = j["tabHoverColor"][3]; }
    if (j.contains("tabActiveColor")) { tabActiveColor.r = j["tabActiveColor"][0]; tabActiveColor.g = j["tabActiveColor"][1]; tabActiveColor.b = j["tabActiveColor"][2]; tabActiveColor.a = j["tabActiveColor"][3]; }
    if (j.contains("tabTextColor")) { tabTextColor.r = j["tabTextColor"][0]; tabTextColor.g = j["tabTextColor"][1]; tabTextColor.b = j["tabTextColor"][2]; tabTextColor.a = j["tabTextColor"][3]; }
    if (j.contains("tabFontSize")) tabFontSize = j["tabFontSize"];
    if (j.contains("tabCornerRadius")) tabCornerRadius = j["tabCornerRadius"];

    if (j.contains("contentBackgroundColor")) { contentBackgroundColor.r = j["contentBackgroundColor"][0]; contentBackgroundColor.g = j["contentBackgroundColor"][1]; contentBackgroundColor.b = j["contentBackgroundColor"][2]; contentBackgroundColor.a = j["contentBackgroundColor"][3]; }
    if (j.contains("contentCornerRadius")) contentCornerRadius = j["contentCornerRadius"];

    if (j.contains("luaCallback")) luaCallback = j["luaCallback"].get<std::string>();
}

}
