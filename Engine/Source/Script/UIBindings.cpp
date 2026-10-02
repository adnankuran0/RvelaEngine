#include "rvelapch.h"
#include "UIBindings.h"
#include "ComponentHandle.h"
#include "Scene/Components/UIComponents.h"

using namespace rv;

using RectTransformHandle = ComponentHandle<RectTransformComponent>;
using UICanvasHandle = ComponentHandle<UICanvasComponent>;
using UIImageHandle = ComponentHandle<UIImageComponent>;
using UITextHandle = ComponentHandle<UITextComponent>;
using UIButtonHandle = ComponentHandle<UIButtonComponent>;
using UISliderHandle = ComponentHandle<UISliderComponent>;
using UIProgressBarHandle = ComponentHandle<UIProgressBarComponent>;
using UICheckboxHandle = ComponentHandle<UICheckboxComponent>;

void LuaBindings::RegisterUIComponents(sol::state& lua)
{
    lua.new_usertype<RectTransformHandle>("RectTransformComponent",
        "IsValid", &RectTransformHandle::IsValid,
        "position", sol::property(
            [](RectTransformHandle& h) { return h.Get() ? h.Get()->position : glm::vec2(0.0f); },
            [](RectTransformHandle& h, const glm::vec2& v) { if (auto* c = h.Get()) c->position = v; }
        ),
        "size", sol::property(
            [](RectTransformHandle& h) { return h.Get() ? h.Get()->size : glm::vec2(0.0f); },
            [](RectTransformHandle& h, const glm::vec2& v) { if (auto* c = h.Get()) c->size = v; }
        ),
        "anchorMin", sol::property(
            [](RectTransformHandle& h) { return h.Get() ? h.Get()->anchorMin : glm::vec2(0.0f); },
            [](RectTransformHandle& h, const glm::vec2& v) { if (auto* c = h.Get()) c->anchorMin = v; }
        ),
        "anchorMax", sol::property(
            [](RectTransformHandle& h) { return h.Get() ? h.Get()->anchorMax : glm::vec2(0.0f); },
            [](RectTransformHandle& h, const glm::vec2& v) { if (auto* c = h.Get()) c->anchorMax = v; }
        ),
        "pivot", sol::property(
            [](RectTransformHandle& h) { return h.Get() ? h.Get()->pivot : glm::vec2(0.0f); },
            [](RectTransformHandle& h, const glm::vec2& v) { if (auto* c = h.Get()) c->pivot = v; }
        ),
        "rotation", sol::property(
            [](RectTransformHandle& h) { return h.Get() ? h.Get()->rotation : 0.0f; },
            [](RectTransformHandle& h, float r) { if (auto* c = h.Get()) c->rotation = r; }
        ),
        "scale", sol::property(
            [](RectTransformHandle& h) { return h.Get() ? h.Get()->scale : glm::vec2(1.0f); },
            [](RectTransformHandle& h, const glm::vec2& v) { if (auto* c = h.Get()) c->scale = v; }
        ),
        "raycastTarget", sol::property(
            [](RectTransformHandle& h) { return h.Get() ? h.Get()->raycastTarget : true; },
            [](RectTransformHandle& h, bool b) { if (auto* c = h.Get()) c->raycastTarget = b; }
        )
    );

    lua.new_usertype<UICanvasHandle>("UICanvasComponent",
        "IsValid", &UICanvasHandle::IsValid,
        "sortOrder", sol::property(
            [](UICanvasHandle& h) { return h.Get() ? h.Get()->sortOrder : 0; },
            [](UICanvasHandle& h, int order) { if (auto* c = h.Get()) c->sortOrder = order; }
        ),
        "billboardMode", sol::property(
            [](UICanvasHandle& h) { return h.Get() ? static_cast<int>(h.Get()->billboardMode) : 0; },
            [](UICanvasHandle& h, int mode) { if (auto* c = h.Get()) c->billboardMode = static_cast<BillboardMode>(mode); }
        ),
        "isBillboard", sol::property(
            [](UICanvasHandle& h) { return h.Get() ? h.Get()->billboardMode != BillboardMode::Disabled : false; },
            [](UICanvasHandle& h, bool b) { if (auto* c = h.Get()) c->billboardMode = b ? BillboardMode::Spherical : BillboardMode::Disabled; }
        ),
        "constantScreenSize", sol::property(
            [](UICanvasHandle& h) { return h.Get() ? h.Get()->constantScreenSize : false; },
            [](UICanvasHandle& h, bool b) { if (auto* c = h.Get()) c->constantScreenSize = b; }
        ),
        "constantScaleFactor", sol::property(
            [](UICanvasHandle& h) { return h.Get() ? h.Get()->constantScaleFactor : 1.0f; },
            [](UICanvasHandle& h, float f) { if (auto* c = h.Get()) c->constantScaleFactor = f; }
        )
    );

    lua.new_usertype<UIImageHandle>("UIImageComponent",
        "IsValid", &UIImageHandle::IsValid,
        "color", sol::property(
            [](UIImageHandle& h) { return h.Get() ? h.Get()->color : glm::vec4(1.0f); },
            [](UIImageHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->color = col; }
        )
    );

    lua.new_usertype<UITextHandle>("UITextComponent",
        "IsValid", &UITextHandle::IsValid,
        "text", sol::property(
            [](UITextHandle& h) { return h.Get() ? h.Get()->text : std::string(); },
            [](UITextHandle& h, const std::string& txt) { if (auto* c = h.Get()) c->text = txt; }
        ),
        "color", sol::property(
            [](UITextHandle& h) { return h.Get() ? h.Get()->color : glm::vec4(1.0f); },
            [](UITextHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->color = col; }
        ),
        "fontSize", sol::property(
            [](UITextHandle& h) { return h.Get() ? h.Get()->fontSize : 18.0f; },
            [](UITextHandle& h, float size) { if (auto* c = h.Get()) c->fontSize = size; }
        ),
        "isBold", sol::property(
            [](UITextHandle& h) { return h.Get() ? h.Get()->isBold : false; },
            [](UITextHandle& h, bool b) { if (auto* c = h.Get()) c->isBold = b; }
        ),
        "isOutline", sol::property(
            [](UITextHandle& h) { return h.Get() ? h.Get()->isOutline : false; },
            [](UITextHandle& h, bool b) { if (auto* c = h.Get()) c->isOutline = b; }
        ),
        "outlineSize", sol::property(
            [](UITextHandle& h) { return h.Get() ? h.Get()->outlineSize : 1.0f; },
            [](UITextHandle& h, float size) { if (auto* c = h.Get()) c->outlineSize = size; }
        ),
        "outlineColor", sol::property(
            [](UITextHandle& h) { return h.Get() ? h.Get()->outlineColor : glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); },
            [](UITextHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->outlineColor = col; }
        ),
        "wordWrap", sol::property(
            [](UITextHandle& h) { return h.Get() ? h.Get()->wordWrap : false; },
            [](UITextHandle& h, bool b) { if (auto* c = h.Get()) c->wordWrap = b; }
        ),
        "lineSpacing", sol::property(
            [](UITextHandle& h) { return h.Get() ? h.Get()->lineSpacing : 1.2f; },
            [](UITextHandle& h, float f) { if (auto* c = h.Get()) c->lineSpacing = f; }
        ),
        "characterSpacing", sol::property(
            [](UITextHandle& h) { return h.Get() ? h.Get()->characterSpacing : 0.0f; },
            [](UITextHandle& h, float f) { if (auto* c = h.Get()) c->characterSpacing = f; }
        )
    );

    lua.new_usertype<UIButtonHandle>("UIButtonComponent",
        "IsValid", &UIButtonHandle::IsValid,
        "interactable", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->interactable : false; },
            [](UIButtonHandle& h, bool b) { if (auto* c = h.Get()) c->interactable = b; }
        ),
        "text", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->text : std::string(); },
            [](UIButtonHandle& h, const std::string& txt) { if (auto* c = h.Get()) c->text = txt; }
        ),
        "textColor", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->textColor : glm::vec4(1.0f); },
            [](UIButtonHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->textColor = col; }
        ),
        "isBold", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->isBold : false; },
            [](UIButtonHandle& h, bool b) { if (auto* c = h.Get()) c->isBold = b; }
        ),
        "isOutline", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->isOutline : false; },
            [](UIButtonHandle& h, bool b) { if (auto* c = h.Get()) c->isOutline = b; }
        ),
        "outlineSize", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->outlineSize : 1.0f; },
            [](UIButtonHandle& h, float size) { if (auto* c = h.Get()) c->outlineSize = size; }
        ),
        "outlineColor", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->outlineColor : glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); },
            [](UIButtonHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->outlineColor = col; }
        ),
        "normalColor", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->normalColor : glm::vec4(1.0f); },
            [](UIButtonHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->normalColor = col; }
        ),
        "hoverColor", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->hoverColor : glm::vec4(1.0f); },
            [](UIButtonHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->hoverColor = col; }
        ),
        "pressedColor", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->pressedColor : glm::vec4(1.0f); },
            [](UIButtonHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->pressedColor = col; }
        ),
        "luaCallback", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->luaCallback : std::string(); },
            [](UIButtonHandle& h, const std::string& cb) { if (auto* c = h.Get()) c->luaCallback = cb; }
        ),
        "OnClick", [](UIButtonHandle& h, sol::protected_function func) {
            if (auto* c = h.Get()) {
                c->onClick = [func]() {
                    if (func.valid()) {
                        auto result = func();
                        if (!result.valid()) {
                            sol::error err = result;
                            LOG_ERROR("Lua OnClick error: {}", err.what());
                        }
                    }
                };
            }
        }
    );

    lua.new_usertype<UISliderHandle>("UISliderComponent",
        "IsValid", &UISliderHandle::IsValid,
        "value", sol::property(
            [](UISliderHandle& h) { return h.Get() ? h.Get()->value : 0.0f; },
            [](UISliderHandle& h, float val) { if (auto* c = h.Get()) c->value = val; }
        ),
        "minValue", sol::property(
            [](UISliderHandle& h) { return h.Get() ? h.Get()->minValue : 0.0f; },
            [](UISliderHandle& h, float val) { if (auto* c = h.Get()) c->minValue = val; }
        ),
        "maxValue", sol::property(
            [](UISliderHandle& h) { return h.Get() ? h.Get()->maxValue : 1.0f; },
            [](UISliderHandle& h, float val) { if (auto* c = h.Get()) c->maxValue = val; }
        ),
        "luaCallback", sol::property(
            [](UISliderHandle& h) { return h.Get() ? h.Get()->luaCallback : std::string(); },
            [](UISliderHandle& h, const std::string& cb) { if (auto* c = h.Get()) c->luaCallback = cb; }
        ),
        "OnValueChanged", [](UISliderHandle& h, sol::protected_function func) {
            if (auto* c = h.Get()) {
                c->onValueChanged = [func](float val) {
                    if (func.valid()) {
                        auto result = func(val);
                        if (!result.valid()) {
                            sol::error err = result;
                            LOG_ERROR("Lua OnValueChanged error: {}", err.what());
                        }
                    }
                };
            }
        }
    );

    lua.new_usertype<UIProgressBarHandle>("UIProgressBarComponent",
        "IsValid", &UIProgressBarHandle::IsValid,
        "value", sol::property(
            [](UIProgressBarHandle& h) { return h.Get() ? h.Get()->value : 0.0f; },
            [](UIProgressBarHandle& h, float val) { if (auto* c = h.Get()) c->value = val; }
        ),
        "fillColor", sol::property(
            [](UIProgressBarHandle& h) { return h.Get() ? h.Get()->fillColor : glm::vec4(1.0f); },
            [](UIProgressBarHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->fillColor = col; }
        )
    );

    lua.new_usertype<UICheckboxHandle>("UICheckboxComponent",
        "IsValid", &UICheckboxHandle::IsValid,
        "isChecked", sol::property(
            [](UICheckboxHandle& h) { return h.Get() ? h.Get()->isChecked : false; },
            [](UICheckboxHandle& h, bool b) { if (auto* c = h.Get()) c->isChecked = b; }
        ),
        "interactable", sol::property(
            [](UICheckboxHandle& h) { return h.Get() ? h.Get()->interactable : true; },
            [](UICheckboxHandle& h, bool b) { if (auto* c = h.Get()) c->interactable = b; }
        ),
        "label", sol::property(
            [](UICheckboxHandle& h) { return h.Get() ? h.Get()->label : std::string(); },
            [](UICheckboxHandle& h, const std::string& txt) { if (auto* c = h.Get()) c->label = txt; }
        ),
        "fontSize", sol::property(
            [](UICheckboxHandle& h) { return h.Get() ? h.Get()->fontSize : 18.0f; },
            [](UICheckboxHandle& h, float sz) { if (auto* c = h.Get()) c->fontSize = sz; }
        ),
        "luaCallback", sol::property(
            [](UICheckboxHandle& h) { return h.Get() ? h.Get()->luaCallback : std::string(); },
            [](UICheckboxHandle& h, const std::string& cb) { if (auto* c = h.Get()) c->luaCallback = cb; }
        ),
        "OnValueChanged", [](UICheckboxHandle& h, sol::protected_function func) {
            if (auto* c = h.Get()) {
                c->onValueChanged = [func](bool checked) {
                    if (func.valid()) {
                        auto result = func(checked);
                        if (!result.valid()) {
                            sol::error err = result;
                            LOG_ERROR("Lua Checkbox OnValueChanged error: {}", err.what());
                        }
                    }
                };
            }
        }
    );
}
