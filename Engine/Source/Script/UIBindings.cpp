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
using UIPanelHandle = ComponentHandle<UIPanelComponent>;
using UILayoutGroupHandle = ComponentHandle<UILayoutGroupComponent>;
using UITextInputHandle = ComponentHandle<UITextInputComponent>;
using UIScrollRectHandle = ComponentHandle<UIScrollRectComponent>;
using UITabContainerHandle = ComponentHandle<UITabContainerComponent>;

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
        ),
        "textureUUID", sol::property(
            [](UIImageHandle& h) { return h.Get() ? h.Get()->textureUUID : AssetUUID::Invalid(); },
            [](UIImageHandle& h, const AssetUUID& u) { if (auto* c = h.Get()) c->textureUUID = u; }
        ),
        "SetTexture", [](UIImageHandle& h, const AssetUUID& u) {
            if (auto* c = h.Get()) c->SetTexture(u);
        },
        "ClearTexture", [](UIImageHandle& h) {
            if (auto* c = h.Get()) c->ClearTexture();
        }
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
        "alignment", sol::property(
            [](UITextHandle& h) { return h.Get() ? static_cast<int>(h.Get()->alignment) : 0; },
            [](UITextHandle& h, int a) { if (auto* c = h.Get()) c->alignment = static_cast<TextAlignment>(a); }
        ),
        "verticalAlignment", sol::property(
            [](UITextHandle& h) { return h.Get() ? static_cast<int>(h.Get()->verticalAlignment) : 1; },
            [](UITextHandle& h, int va) { if (auto* c = h.Get()) c->verticalAlignment = static_cast<TextVerticalAlignment>(va); }
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
        "fontSize", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->fontSize : 24.0f; },
            [](UIButtonHandle& h, float sz) { if (auto* c = h.Get()) c->fontSize = sz; }
        ),
        "isBold", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->isBold : false; },
            [](UIButtonHandle& h, bool b) { if (auto* c = h.Get()) c->isBold = b; }
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
        "disabledColor", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->disabledColor : glm::vec4(1.0f); },
            [](UIButtonHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->disabledColor = col; }
        ),
        "normalTextureUUID", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->normalTextureUUID : AssetUUID::Invalid(); },
            [](UIButtonHandle& h, const AssetUUID& u) { if (auto* c = h.Get()) c->normalTextureUUID = u; }
        ),
        "hoverTextureUUID", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->hoverTextureUUID : AssetUUID::Invalid(); },
            [](UIButtonHandle& h, const AssetUUID& u) { if (auto* c = h.Get()) c->hoverTextureUUID = u; }
        ),
        "pressedTextureUUID", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->pressedTextureUUID : AssetUUID::Invalid(); },
            [](UIButtonHandle& h, const AssetUUID& u) { if (auto* c = h.Get()) c->pressedTextureUUID = u; }
        ),
        "disabledTextureUUID", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->disabledTextureUUID : AssetUUID::Invalid(); },
            [](UIButtonHandle& h, const AssetUUID& u) { if (auto* c = h.Get()) c->disabledTextureUUID = u; }
        ),
        "cornerRadius", sol::property(
            [](UIButtonHandle& h) { return h.Get() ? h.Get()->cornerRadius : 0.0f; },
            [](UIButtonHandle& h, float r) { if (auto* c = h.Get()) c->cornerRadius = r; }
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
        "backgroundColor", sol::property(
            [](UIProgressBarHandle& h) { return h.Get() ? h.Get()->backgroundColor : glm::vec4(1.0f); },
            [](UIProgressBarHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->backgroundColor = col; }
        ),
        "fillColor", sol::property(
            [](UIProgressBarHandle& h) { return h.Get() ? h.Get()->fillColor : glm::vec4(1.0f); },
            [](UIProgressBarHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->fillColor = col; }
        ),
        "backgroundTextureUUID", sol::property(
            [](UIProgressBarHandle& h) { return h.Get() ? h.Get()->backgroundTextureUUID : AssetUUID::Invalid(); },
            [](UIProgressBarHandle& h, const AssetUUID& u) { if (auto* c = h.Get()) c->backgroundTextureUUID = u; }
        ),
        "fillTextureUUID", sol::property(
            [](UIProgressBarHandle& h) { return h.Get() ? h.Get()->fillTextureUUID : AssetUUID::Invalid(); },
            [](UIProgressBarHandle& h, const AssetUUID& u) { if (auto* c = h.Get()) c->fillTextureUUID = u; }
        ),
        "showPercentageText", sol::property(
            [](UIProgressBarHandle& h) { return h.Get() ? h.Get()->showPercentageText : false; },
            [](UIProgressBarHandle& h, bool b) { if (auto* c = h.Get()) c->showPercentageText = b; }
        ),
        "direction", sol::property(
            [](UIProgressBarHandle& h) { return h.Get() ? static_cast<int>(h.Get()->direction) : 0; },
            [](UIProgressBarHandle& h, int d) { if (auto* c = h.Get()) c->direction = static_cast<UIProgressBarDirection>(d); }
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

    lua.new_usertype<UIPanelHandle>("UIPanelComponent",
        "IsValid", &UIPanelHandle::IsValid,
        "backgroundColor", sol::property(
            [](UIPanelHandle& h) { return h.Get() ? h.Get()->backgroundColor : glm::vec4(1.0f); },
            [](UIPanelHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->backgroundColor = col; }
        ),
        "borderColor", sol::property(
            [](UIPanelHandle& h) { return h.Get() ? h.Get()->borderColor : glm::vec4(1.0f); },
            [](UIPanelHandle& h, const glm::vec4& col) { if (auto* c = h.Get()) c->borderColor = col; }
        ),
        "borderThickness", sol::property(
            [](UIPanelHandle& h) { return h.Get() ? h.Get()->borderThickness : 0.0f; },
            [](UIPanelHandle& h, float f) { if (auto* c = h.Get()) c->borderThickness = f; }
        ),
        "cornerRadius", sol::property(
            [](UIPanelHandle& h) { return h.Get() ? h.Get()->cornerRadius : 0.0f; },
            [](UIPanelHandle& h, float r) { if (auto* c = h.Get()) c->cornerRadius = r; }
        ),
        "title", sol::property(
            [](UIPanelHandle& h) { return h.Get() ? h.Get()->title : std::string(); },
            [](UIPanelHandle& h, const std::string& t) { if (auto* c = h.Get()) c->title = t; }
        ),
        "isDraggable", sol::property(
            [](UIPanelHandle& h) { return h.Get() ? h.Get()->isDraggable : false; },
            [](UIPanelHandle& h, bool b) { if (auto* c = h.Get()) c->isDraggable = b; }
        ),
        "clipChildren", sol::property(
            [](UIPanelHandle& h) { return h.Get() ? h.Get()->clipChildren : false; },
            [](UIPanelHandle& h, bool b) { if (auto* c = h.Get()) c->clipChildren = b; }
        )
    );

    lua.new_usertype<UILayoutGroupHandle>("UILayoutGroupComponent",
        "IsValid", &UILayoutGroupHandle::IsValid,
        "type", sol::property(
            [](UILayoutGroupHandle& h) { return h.Get() ? static_cast<int>(h.Get()->type) : 0; },
            [](UILayoutGroupHandle& h, int t) { if (auto* c = h.Get()) c->type = static_cast<UILayoutType>(t); }
        ),
        "spacing", sol::property(
            [](UILayoutGroupHandle& h) { return h.Get() ? h.Get()->spacing : 0.0f; },
            [](UILayoutGroupHandle& h, float s) { if (auto* c = h.Get()) c->spacing = s; }
        ),
        "fitChildren", sol::property(
            [](UILayoutGroupHandle& h) { return h.Get() ? h.Get()->fitChildren : false; },
            [](UILayoutGroupHandle& h, bool b) { if (auto* c = h.Get()) c->fitChildren = b; }
        ),
        "controlChildWidth", sol::property(
            [](UILayoutGroupHandle& h) { return h.Get() ? h.Get()->controlChildWidth : false; },
            [](UILayoutGroupHandle& h, bool b) { if (auto* c = h.Get()) c->controlChildWidth = b; }
        ),
        "controlChildHeight", sol::property(
            [](UILayoutGroupHandle& h) { return h.Get() ? h.Get()->controlChildHeight : false; },
            [](UILayoutGroupHandle& h, bool b) { if (auto* c = h.Get()) c->controlChildHeight = b; }
        )
    );

    lua.new_usertype<UITextInputHandle>("UITextInputComponent",
        "IsValid", &UITextInputHandle::IsValid,
        "text", sol::property(
            [](UITextInputHandle& h) { return h.Get() ? h.Get()->text : std::string(); },
            [](UITextInputHandle& h, const std::string& t) { if (auto* c = h.Get()) c->text = t; }
        ),
        "placeholder", sol::property(
            [](UITextInputHandle& h) { return h.Get() ? h.Get()->placeholder : std::string(); },
            [](UITextInputHandle& h, const std::string& p) { if (auto* c = h.Get()) c->placeholder = p; }
        ),
        "isPassword", sol::property(
            [](UITextInputHandle& h) { return h.Get() ? h.Get()->isPassword : false; },
            [](UITextInputHandle& h, bool b) { if (auto* c = h.Get()) c->isPassword = b; }
        ),
        "interactable", sol::property(
            [](UITextInputHandle& h) { return h.Get() ? h.Get()->interactable : true; },
            [](UITextInputHandle& h, bool b) { if (auto* c = h.Get()) c->interactable = b; }
        ),
        "isFocused", sol::property(
            [](UITextInputHandle& h) { return h.Get() ? h.Get()->isFocused : false; },
            [](UITextInputHandle& h, bool b) { if (auto* c = h.Get()) c->isFocused = b; }
        ),
        "luaCallback", sol::property(
            [](UITextInputHandle& h) { return h.Get() ? h.Get()->luaCallback : std::string(); },
            [](UITextInputHandle& h, const std::string& cb) { if (auto* c = h.Get()) c->luaCallback = cb; }
        ),
        "luaSubmitCallback", sol::property(
            [](UITextInputHandle& h) { return h.Get() ? h.Get()->luaSubmitCallback : std::string(); },
            [](UITextInputHandle& h, const std::string& cb) { if (auto* c = h.Get()) c->luaSubmitCallback = cb; }
        ),
        "OnValueChanged", [](UITextInputHandle& h, sol::protected_function func) {
            if (auto* c = h.Get()) {
                c->onValueChanged = [func](const std::string& str) {
                    if (func.valid()) {
                        auto result = func(str);
                        if (!result.valid()) {
                            sol::error err = result;
                            LOG_ERROR("Lua TextInput OnValueChanged error: {}", err.what());
                        }
                    }
                };
            }
        },
        "OnSubmit", [](UITextInputHandle& h, sol::protected_function func) {
            if (auto* c = h.Get()) {
                c->onSubmit = [func](const std::string& str) {
                    if (func.valid()) {
                        auto result = func(str);
                        if (!result.valid()) {
                            sol::error err = result;
                            LOG_ERROR("Lua TextInput OnSubmit error: {}", err.what());
                        }
                    }
                };
            }
        }
    );

    lua.new_usertype<UIScrollRectHandle>("UIScrollRectComponent",
        "IsValid", &UIScrollRectHandle::IsValid,
        "horizontal", sol::property(
            [](UIScrollRectHandle& h) { return h.Get() ? h.Get()->horizontal : false; },
            [](UIScrollRectHandle& h, bool b) { if (auto* c = h.Get()) c->horizontal = b; }
        ),
        "vertical", sol::property(
            [](UIScrollRectHandle& h) { return h.Get() ? h.Get()->vertical : true; },
            [](UIScrollRectHandle& h, bool b) { if (auto* c = h.Get()) c->vertical = b; }
        ),
        "scrollPosition", sol::property(
            [](UIScrollRectHandle& h) { return h.Get() ? h.Get()->scrollPosition : glm::vec2(0.0f); },
            [](UIScrollRectHandle& h, const glm::vec2& p) { if (auto* c = h.Get()) c->scrollPosition = p; }
        ),
        "contentSize", sol::property(
            [](UIScrollRectHandle& h) { return h.Get() ? h.Get()->contentSize : glm::vec2(0.0f); },
            [](UIScrollRectHandle& h, const glm::vec2& s) { if (auto* c = h.Get()) c->contentSize = s; }
        ),
        "scrollSpeed", sol::property(
            [](UIScrollRectHandle& h) { return h.Get() ? h.Get()->scrollSpeed : 30.0f; },
            [](UIScrollRectHandle& h, float s) { if (auto* c = h.Get()) c->scrollSpeed = s; }
        ),
        "showScrollbar", sol::property(
            [](UIScrollRectHandle& h) { return h.Get() ? h.Get()->showScrollbar : true; },
            [](UIScrollRectHandle& h, bool b) { if (auto* c = h.Get()) c->showScrollbar = b; }
        ),
        "OnScroll", [](UIScrollRectHandle& h, sol::protected_function func) {
            if (auto* c = h.Get()) {
                c->onScroll = [func](const glm::vec2& pos) {
                    if (func.valid()) {
                        auto result = func(pos);
                        if (!result.valid()) {
                            sol::error err = result;
                            LOG_ERROR("Lua ScrollRect OnScroll error: {}", err.what());
                        }
                    }
                };
            }
        }
    );

    lua.new_usertype<UITabContainerHandle>("UITabContainerComponent",
        "IsValid", &UITabContainerHandle::IsValid,
        "activeTabIndex", sol::property(
            [](UITabContainerHandle& h) { return h.Get() ? h.Get()->activeTabIndex : 0; },
            [](UITabContainerHandle& h, int idx) { if (auto* c = h.Get()) c->activeTabIndex = idx; }
        ),
        "tabHeight", sol::property(
            [](UITabContainerHandle& h) { return h.Get() ? h.Get()->tabHeight : 34.0f; },
            [](UITabContainerHandle& h, float th) { if (auto* c = h.Get()) c->tabHeight = th; }
        ),
        "AddTab", [](UITabContainerHandle& h, const std::string& title) {
            if (auto* c = h.Get()) c->tabTitles.push_back(title);
        },
        "GetTabCount", [](UITabContainerHandle& h) -> int {
            return h.Get() ? static_cast<int>(h.Get()->tabTitles.size()) : 0;
        },
        "OnTabChanged", [](UITabContainerHandle& h, sol::protected_function func) {
            if (auto* c = h.Get()) {
                c->onTabChanged = [func](int idx) {
                    if (func.valid()) {
                        auto result = func(idx);
                        if (!result.valid()) {
                            sol::error err = result;
                            LOG_ERROR("Lua TabContainer OnTabChanged error: {}", err.what());
                        }
                    }
                };
            }
        }
    );

    lua.new_enum<TextAlignment>("TextAlignment", {
        { "Left", TextAlignment::Left },
        { "Center", TextAlignment::Center },
        { "Right", TextAlignment::Right }
    });

    lua.new_enum<TextVerticalAlignment>("TextVerticalAlignment", {
        { "Top", TextVerticalAlignment::Top },
        { "Middle", TextVerticalAlignment::Middle },
        { "Bottom", TextVerticalAlignment::Bottom }
    });

    lua.new_enum<UILayoutType>("UILayoutType", {
        { "Vertical", UILayoutType::Vertical },
        { "Horizontal", UILayoutType::Horizontal }
    });

    lua.new_enum<UIChildAlignment>("UIChildAlignment", {
        { "Start", UIChildAlignment::Start },
        { "Center", UIChildAlignment::Center },
        { "End", UIChildAlignment::End }
    });

    lua.new_enum<UIProgressBarDirection>("UIProgressBarDirection", {
        { "LeftToRight", UIProgressBarDirection::LeftToRight },
        { "RightToLeft", UIProgressBarDirection::RightToLeft },
        { "BottomToTop", UIProgressBarDirection::BottomToTop },
        { "TopToBottom", UIProgressBarDirection::TopToBottom }
    });
}
