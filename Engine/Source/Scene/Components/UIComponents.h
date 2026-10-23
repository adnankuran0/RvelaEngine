#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>
#include "Asset/AssetUUID.h"
#include "Renderer/RenderTypes.h"
#include "json.hpp"

namespace rv {

using json = nlohmann::json;

enum class CanvasMode
{
    ScreenSpaceOverlay,
    WorldSpace
};

enum class ImageType
{
    Simple,
    Sliced
};

enum class TextAlignment
{
    Left,
    Center,
    Right
};

enum class TextVerticalAlignment
{
    Top,
    Middle,
    Bottom
};

enum class UILayoutType
{
    Vertical = 0,
    Horizontal = 1
};

enum class UIChildAlignment
{
    Start = 0,
    Center = 1,
    End = 2
};

enum class UIProgressBarDirection
{
    LeftToRight = 0,
    RightToLeft = 1,
    BottomToTop = 2,
    TopToBottom = 3
};

struct UICanvasComponent
{
    CanvasMode mode = CanvasMode::ScreenSpaceOverlay;
    glm::vec2 referenceResolution = { 1920.0f, 1080.0f };
    bool matchWidthOrHeight = true;
    int sortOrder = 0;
    bool depthTest = true;
    BillboardMode billboardMode = BillboardMode::Disabled;
    bool constantScreenSize = false;
    float constantScaleFactor = 1.0f;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct RectTransformComponent
{
    glm::vec2 position = { 0.0f, 0.0f };
    glm::vec2 size = { 100.0f, 100.0f };
    glm::vec2 anchorMin = { 0.5f, 0.5f };
    glm::vec2 anchorMax = { 0.5f, 0.5f };
    glm::vec2 pivot = { 0.5f, 0.5f };
    float rotation = 0.0f; // degrees
    glm::vec2 scale = { 1.0f, 1.0f };
    bool raycastTarget = true;

    // Computed absolute screen rectangle
    glm::vec4 computedScreenRect = { 0.0f, 0.0f, 0.0f, 0.0f };
    float computedRotation = 0.0f;
    glm::mat4 computedWorldMatrix = glm::mat4(1.0f);

    glm::vec4 CalculateScreenRect(const glm::vec2& parentPosition, const glm::vec2& parentSize) const;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UIImageComponent
{
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    AssetUUID textureUUID = AssetUUID::Invalid();
    ImageType imageType = ImageType::Simple;
    float cornerRadius = 0.0f;

    void SetTexture(const AssetUUID& uuid) { textureUUID = uuid; }
    void ClearTexture() { textureUUID = AssetUUID::Invalid(); }

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UITextComponent
{
    std::string text = "Text";
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float fontSize = 24.0f;
    TextAlignment alignment = TextAlignment::Left;
    TextVerticalAlignment verticalAlignment = TextVerticalAlignment::Middle;
    std::string fontPath;
    AssetUUID fontUUID = AssetUUID::Invalid();

    bool isBold = false;
    bool isOutline = false;
    float outlineSize = 1.0f;
    glm::vec4 outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };

    bool wordWrap = false;
    float lineSpacing = 1.2f;
    float characterSpacing = 0.0f;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UIButtonComponent
{
    glm::vec4 normalColor = { 0.25f, 0.38f, 0.65f, 1.0f };
    glm::vec4 hoverColor = { 0.35f, 0.50f, 0.80f, 1.0f };
    glm::vec4 pressedColor = { 0.15f, 0.25f, 0.50f, 1.0f };
    glm::vec4 disabledColor = { 0.20f, 0.20f, 0.20f, 0.5f };
    float cornerRadius = 6.0f;
    bool interactable = true;

    AssetUUID normalTextureUUID = AssetUUID::Invalid();
    AssetUUID hoverTextureUUID = AssetUUID::Invalid();
    AssetUUID pressedTextureUUID = AssetUUID::Invalid();
    AssetUUID disabledTextureUUID = AssetUUID::Invalid();
    ImageType imageType = ImageType::Simple;
    float borderSize = 8.0f;

    // Integrated Button Text
    std::string text = "Button";
    glm::vec4 textColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float fontSize = 24.0f;
    std::string fontPath;

    bool isBold = false;
    bool isOutline = false;
    float outlineSize = 1.0f;
    glm::vec4 outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };

    // runtime
    bool isHovered = false;
    bool isPressed = false;

    std::string luaCallback;
    std::function<void()> onClick;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UISliderComponent
{
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float value = 0.5f;
    bool interactable = true;

    glm::vec4 backgroundColor = { 0.15f, 0.15f, 0.15f, 1.0f };
    glm::vec4 fillColor = { 0.20f, 0.60f, 0.90f, 1.0f };
    glm::vec4 handleColor = { 0.90f, 0.90f, 0.90f, 1.0f };

    bool isDragging = false;
    std::string luaCallback;
    std::function<void(float)> onValueChanged;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UIProgressBarComponent
{
    float value = 1.0f; // 0.0 to 1.0
    glm::vec4 backgroundColor = { 0.15f, 0.15f, 0.15f, 1.0f };
    glm::vec4 fillColor = { 0.20f, 0.75f, 0.35f, 1.0f };

    AssetUUID backgroundTextureUUID = AssetUUID::Invalid();
    AssetUUID fillTextureUUID = AssetUUID::Invalid();
    glm::vec4 borderColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    float borderThickness = 0.0f;
    float cornerRadius = 4.0f;
    UIProgressBarDirection direction = UIProgressBarDirection::LeftToRight;

    bool showPercentageText = false;
    glm::vec4 percentageTextColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float percentageFontSize = 16.0f;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UICheckboxComponent
{
    bool isChecked = false;
    bool interactable = true;
    bool isHovered = false;

    std::string label = "Checkbox";
    float fontSize = 18.0f;
    float boxSize = 20.0f;

    glm::vec4 boxColor = { 0.20f, 0.20f, 0.20f, 1.0f };
    glm::vec4 checkmarkColor = { 0.20f, 0.60f, 0.90f, 1.0f };
    glm::vec4 textColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    std::string luaCallback;
    std::function<void(bool)> onValueChanged;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UIPanelComponent
{
    glm::vec4 backgroundColor = { 0.12f, 0.14f, 0.18f, 0.95f };
    glm::vec4 borderColor = { 0.25f, 0.30f, 0.40f, 1.0f };
    float borderThickness = 1.0f;
    float cornerRadius = 8.0f;
    AssetUUID backgroundTextureUUID = AssetUUID::Invalid();
    ImageType imageType = ImageType::Simple;

    bool isDraggable = false;
    float headerHeight = 32.0f;
    std::string title = "";
    glm::vec4 headerColor = { 0.18f, 0.22f, 0.28f, 1.0f };
    glm::vec4 titleColor = { 0.95f, 0.95f, 0.95f, 1.0f };
    float titleFontSize = 18.0f;

    bool clipChildren = false;

    // runtime
    bool isDragging = false;
    glm::vec2 dragOffset = { 0.0f, 0.0f };

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UILayoutGroupComponent
{
    UILayoutType type = UILayoutType::Vertical;
    float spacing = 8.0f;
    glm::vec4 padding = { 8.0f, 8.0f, 8.0f, 8.0f }; // left, top, right, bottom
    UIChildAlignment alignment = UIChildAlignment::Start;
    bool fitChildren = false;
    bool controlChildWidth = true;
    bool controlChildHeight = false;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UITextInputComponent
{
    std::string text = "";
    std::string placeholder = "Enter text...";
    uint32_t maxLength = 128;
    bool isPassword = false;
    bool interactable = true;

    glm::vec4 backgroundColor = { 0.10f, 0.12f, 0.16f, 1.0f };
    glm::vec4 borderColor = { 0.25f, 0.30f, 0.40f, 1.0f };
    glm::vec4 focusedBorderColor = { 0.25f, 0.55f, 0.90f, 1.0f };
    float borderThickness = 1.5f;
    float cornerRadius = 4.0f;

    glm::vec4 textColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec4 placeholderColor = { 0.55f, 0.55f, 0.55f, 1.0f };
    glm::vec4 cursorColor = { 0.9f, 0.9f, 0.9f, 1.0f };
    float fontSize = 18.0f;
    glm::vec2 textPadding = { 8.0f, 0.0f };

    // runtime
    bool isFocused = false;
    bool isHovered = false;
    int cursorPosition = 0;
    float cursorBlinkTimer = 0.0f;
    bool cursorVisible = true;

    std::string luaCallback;
    std::string luaSubmitCallback;
    std::function<void(const std::string&)> onValueChanged;
    std::function<void(const std::string&)> onSubmit;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UIScrollRectComponent
{
    bool horizontal = false;
    bool vertical = true;
    glm::vec2 scrollPosition = { 0.0f, 0.0f }; // 0.0 to 1.0
    glm::vec2 contentSize = { 200.0f, 600.0f };
    float scrollSpeed = 30.0f;

    bool showScrollbar = true;
    float scrollbarWidth = 8.0f;
    glm::vec4 trackColor = { 0.15f, 0.15f, 0.15f, 0.5f };
    glm::vec4 thumbColor = { 0.40f, 0.45f, 0.55f, 0.8f };
    glm::vec4 thumbHoverColor = { 0.55f, 0.65f, 0.80f, 1.0f };
    float cornerRadius = 4.0f;

    // runtime
    bool isDraggingVertical = false;
    bool isDraggingHorizontal = false;
    float dragStartMouseY = 0.0f;
    float dragStartScrollY = 0.0f;
    bool isThumbHovered = false;

    std::string luaCallback;
    std::function<void(const glm::vec2&)> onScroll;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UITabContainerComponent
{
    std::vector<std::string> tabTitles = { "Tab 1", "Tab 2", "Tab 3" };
    int activeTabIndex = 0;
    float tabHeight = 34.0f;
    float tabSpacing = 4.0f;

    glm::vec4 tabNormalColor = { 0.18f, 0.20f, 0.25f, 1.0f };
    glm::vec4 tabHoverColor = { 0.28f, 0.32f, 0.40f, 1.0f };
    glm::vec4 tabActiveColor = { 0.25f, 0.45f, 0.80f, 1.0f };
    glm::vec4 tabTextColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float tabFontSize = 16.0f;
    float tabCornerRadius = 6.0f;

    glm::vec4 contentBackgroundColor = { 0.12f, 0.14f, 0.18f, 0.95f };
    float contentCornerRadius = 6.0f;

    int hoveredTabIndex = -1;

    std::string luaCallback;
    std::function<void(int)> onTabChanged;

    json Serialize() const;
    void Deserialize(const json& j);
};

}
