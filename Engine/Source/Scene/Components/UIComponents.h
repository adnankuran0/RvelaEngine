#pragma once

#include <glm/glm.hpp>
#include <string>
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
    std::string texturePath;
    ImageType imageType = ImageType::Simple;
    float cornerRadius = 0.0f;

    json Serialize() const;
    void Deserialize(const json& j);
};

struct UITextComponent
{
    std::string text = "Text";
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float fontSize = 24.0f;
    TextAlignment alignment = TextAlignment::Left;
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

    // Integrated Button Text
    std::string text = "Button";
    glm::vec4 textColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float fontSize = 24.0f;
    std::string fontPath;

    bool isBold = false;
    bool isOutline = false;
    float outlineSize = 1.0f;
    glm::vec4 outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Runtime state
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

}
