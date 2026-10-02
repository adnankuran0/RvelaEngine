#include "rvelapch.h"
#include "UIInteraction.h"
#include "UILayout.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Scene/Components/UIComponents.h"
#include "Script/ScriptEngine.h"
#include "Input/Input.h"
#include "Core/Log.h"
#include <algorithm>
#include <cmath>

namespace rv {

void UIInteraction::UpdateInteractions(Scene* scene, float dt, const glm::vec2& viewportSize, const glm::vec2& mousePos, bool mousePressed, bool mouseHeld, bool mouseReleased)
{
    if (!scene) return;

    // Convert to ortho UI canvas space
    glm::vec2 uiMousePos;
    uiMousePos.x = mousePos.x;
    uiMousePos.y = viewportSize.y - mousePos.y;

    auto buttonView = scene->GetRegistry().view<RectTransformComponent, UIButtonComponent>();
    for (auto entity : buttonView)
    {
        auto& rect = buttonView.get<RectTransformComponent>(entity);
        auto& button = buttonView.get<UIButtonComponent>(entity);

        if (!button.interactable)
        {
            button.isHovered = false;
            button.isPressed = false;
            continue;
        }

        bool hovered = UILayout::IsPointInRect(uiMousePos, rect.computedScreenRect, rect.computedRotation);
        button.isHovered = hovered;

        if (hovered && mousePressed)
        {
            button.isPressed = true;
            if (button.onClick) button.onClick();
            if (!button.luaCallback.empty())
            {
                DispatchLuaCallback(scene, entity, button.luaCallback);
            }
        }
        else if (mouseReleased)
        {
            button.isPressed = false;
        }
        else if (!mouseHeld)
        {
            button.isPressed = false;
        }
    }

    auto sliderView = scene->GetRegistry().view<RectTransformComponent, UISliderComponent>();
    for (auto entity : sliderView)
    {
        auto& rect = sliderView.get<RectTransformComponent>(entity);
        auto& slider = sliderView.get<UISliderComponent>(entity);

        if (!slider.interactable) continue;

        bool hovered = UILayout::IsPointInRect(uiMousePos, rect.computedScreenRect, rect.computedRotation);

        if (hovered && mousePressed)
        {
            slider.isDragging = true;
        }

        if (mouseReleased)
        {
            slider.isDragging = false;
        }

        if (slider.isDragging && mouseHeld)
        {
            glm::vec2 rectCenter = { (rect.computedScreenRect.x + rect.computedScreenRect.z) * 0.5f, (rect.computedScreenRect.y + rect.computedScreenRect.w) * 0.5f };
            float localX = uiMousePos.x;
            if (rect.computedRotation != 0.0f) {
                float rad = glm::radians(-rect.computedRotation);
                float cosR = std::cos(rad);
                float sinR = std::sin(rad);
                float dx = uiMousePos.x - rectCenter.x;
                float dy = uiMousePos.y - rectCenter.y;
                localX = rectCenter.x + dx * cosR - dy * sinR;
            }

            float width = rect.computedScreenRect.z - rect.computedScreenRect.x;
            if (width > 0.0f)
            {
                float t = std::clamp((localX - rect.computedScreenRect.x) / width, 0.0f, 1.0f);
                float newValue = slider.minValue + t * (slider.maxValue - slider.minValue);
                if (newValue != slider.value)
                {
                    slider.value = newValue;
                    if (slider.onValueChanged) slider.onValueChanged(slider.value);
                    if (!slider.luaCallback.empty())
                    {
                        DispatchLuaCallback(scene, entity, slider.luaCallback, slider.value);
                    }
                }
            }
        }
    }

    auto checkboxView = scene->GetRegistry().view<RectTransformComponent, UICheckboxComponent>();
    for (auto entity : checkboxView)
    {
        auto& rect = checkboxView.get<RectTransformComponent>(entity);
        auto& cb = checkboxView.get<UICheckboxComponent>(entity);

        if (!cb.interactable)
        {
            cb.isHovered = false;
            continue;
        }

        bool hovered = UILayout::IsPointInRect(uiMousePos, rect.computedScreenRect, rect.computedRotation);
        cb.isHovered = hovered;

        if (hovered && mousePressed)
        {
            cb.isChecked = !cb.isChecked;
            if (cb.onValueChanged) cb.onValueChanged(cb.isChecked);
            if (!cb.luaCallback.empty())
            {
                DispatchLuaCallback(scene, entity, cb.luaCallback, cb.isChecked ? 1.0f : 0.0f);
            }
        }
    }

    // Raycast check
    bool hoveredUI = false;
    auto rectView = scene->GetRegistry().view<RectTransformComponent>();
    for (auto entity : rectView)
    {
        const auto& rect = rectView.get<RectTransformComponent>(entity);
        if (!rect.raycastTarget) continue;

        bool hasUI = scene->HasComponent<UIButtonComponent>(entity) ||
                     scene->HasComponent<UIImageComponent>(entity) ||
                     scene->HasComponent<UITextComponent>(entity) ||
                     scene->HasComponent<UISliderComponent>(entity) ||
                     scene->HasComponent<UIProgressBarComponent>(entity) ||
                     scene->HasComponent<UICheckboxComponent>(entity);

        if (hasUI && UILayout::IsPointInRect(uiMousePos, rect.computedScreenRect, rect.computedRotation))
        {
            hoveredUI = true;
            break;
        }
    }
    Input::SetMouseOverUI(hoveredUI);
}

void UIInteraction::DispatchLuaCallback(Scene* scene, entt::entity entity, const std::string& callbackName, float param)
{
    if (callbackName.empty() || !scene) return;

    bool isSlider = scene->HasComponent<UISliderComponent>(entity);
    bool isCheckbox = scene->HasComponent<UICheckboxComponent>(entity);

    auto TryInvoke = [&](ScriptComponent& script, entt::entity targetScriptEntity) -> bool {
        if (!script.luaInstance.valid()) return false;
        sol::protected_function func = script.luaInstance[callbackName];
        if (func.valid()) {
            sol::protected_function_result result;
            if (isSlider) {
                result = func(script.luaInstance, param, Entity(entity, scene));
            } else if (isCheckbox) {
                result = func(script.luaInstance, param > 0.5f, Entity(entity, scene));
            } else {
                result = func(script.luaInstance, Entity(entity, scene));
            }

            if (!result.valid()) {
                sol::error err = result;
                LOG_ERROR("Lua UI callback error [{}] on entity {}: {}", callbackName, (uint32_t)entity, err.what());
            }
            return true;
        }
        return false;
    };

    if (scene->HasComponent<ScriptComponent>(entity)) {
        if (TryInvoke(scene->GetComponent<ScriptComponent>(entity), entity)) return;
    }

    entt::entity parent = scene->GetParent(entity);
    while (parent != entt::null) {
        if (scene->HasComponent<ScriptComponent>(parent)) {
            if (TryInvoke(scene->GetComponent<ScriptComponent>(parent), parent)) return;
        }
        parent = scene->GetParent(parent);
    }

    auto view = scene->GetRegistry().view<ScriptComponent>();
    for (auto scriptEntity : view) {
        if (scriptEntity != entity) {
            auto& script = view.get<ScriptComponent>(scriptEntity);
            if (TryInvoke(script, scriptEntity)) return;
        }
    }

    LOG_WARN("UI Callback '{}' was triggered on entity {}, but no script function was found!", callbackName, (uint32_t)entity);
}

}
