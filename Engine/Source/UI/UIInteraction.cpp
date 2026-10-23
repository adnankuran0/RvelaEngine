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

    if (Input::IsMouseCaptured())
    {
        auto buttonView = scene->GetRegistry().view<UIButtonComponent>();
        for (auto entity : buttonView)
        {
            auto& button = buttonView.get<UIButtonComponent>(entity);
            button.isHovered = false;
            button.isPressed = false;
        }

        auto sliderView = scene->GetRegistry().view<UISliderComponent>();
        for (auto entity : sliderView)
        {
            auto& slider = sliderView.get<UISliderComponent>(entity);
            slider.isDragging = false;
        }

        auto panelView = scene->GetRegistry().view<UIPanelComponent>();
        for (auto entity : panelView)
        {
            auto& panel = panelView.get<UIPanelComponent>(entity);
            panel.isDragging = false;
        }

        auto scrollView = scene->GetRegistry().view<UIScrollRectComponent>();
        for (auto entity : scrollView)
        {
            auto& scroll = scrollView.get<UIScrollRectComponent>(entity);
            scroll.isDraggingVertical = false;
            scroll.isDraggingHorizontal = false;
            scroll.isThumbHovered = false;
        }

        auto inputView = scene->GetRegistry().view<UITextInputComponent>();
        for (auto entity : inputView)
        {
            auto& input = inputView.get<UITextInputComponent>(entity);
            input.isHovered = false;
        }

        return;
    }

    // Convert to ortho UI canvas space
    glm::vec2 uiMousePos;
    uiMousePos.x = mousePos.x;
    uiMousePos.y = viewportSize.y - mousePos.y;

    auto buttonView = scene->GetRegistry().view<RectTransformComponent, UIButtonComponent>();
    for (auto entity : buttonView)
    {
        if (!scene->IsEntityActive(entity))
        {
            auto& button = buttonView.get<UIButtonComponent>(entity);
            button.isHovered = false;
            button.isPressed = false;
            continue;
        }

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
        if (!scene->IsEntityActive(entity))
        {
            auto& slider = sliderView.get<UISliderComponent>(entity);
            slider.isDragging = false;
            continue;
        }

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
        if (!scene->IsEntityActive(entity))
        {
            auto& cb = checkboxView.get<UICheckboxComponent>(entity);
            cb.isHovered = false;
            continue;
        }

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

    auto panelView = scene->GetRegistry().view<RectTransformComponent, UIPanelComponent>();
    for (auto entity : panelView)
    {
        if (!scene->IsEntityActive(entity)) continue;
        auto& rect = panelView.get<RectTransformComponent>(entity);
        auto& panel = panelView.get<UIPanelComponent>(entity);
        if (!panel.isDraggable) continue;

        glm::vec4 headerRect = glm::vec4(
            rect.computedScreenRect.x,
            rect.computedScreenRect.w - panel.headerHeight,
            rect.computedScreenRect.z,
            rect.computedScreenRect.w
        );

        bool headerHovered = UILayout::IsPointInRect(uiMousePos, headerRect, rect.computedRotation);
        if (headerHovered && mousePressed)
        {
            panel.isDragging = true;
            panel.dragOffset = glm::vec2(uiMousePos.x - rect.position.x, uiMousePos.y - rect.position.y);
        }
        if (mouseReleased || !mouseHeld)
        {
            panel.isDragging = false;
        }
        if (panel.isDragging && mouseHeld)
        {
            rect.position = glm::vec2(uiMousePos.x - panel.dragOffset.x, uiMousePos.y - panel.dragOffset.y);
        }
    }

    // Text Inputs
    const auto& typedChars = Input::GetTypedCharacters();
    bool backspacePressed = Input::IsKeyJustPressed(KeyCode::Backspace);
    bool deletePressed = Input::IsKeyJustPressed(KeyCode::Delete);
    bool leftPressed = Input::IsKeyJustPressed(KeyCode::Left);
    bool rightPressed = Input::IsKeyJustPressed(KeyCode::Right);
    bool enterPressed = Input::IsKeyJustPressed(KeyCode::Enter);

    auto inputView = scene->GetRegistry().view<RectTransformComponent, UITextInputComponent>();
    for (auto entity : inputView)
    {
        if (!scene->IsEntityActive(entity))
        {
            auto& input = inputView.get<UITextInputComponent>(entity);
            input.isFocused = false;
            input.isHovered = false;
            continue;
        }

        auto& rect = inputView.get<RectTransformComponent>(entity);
        auto& input = inputView.get<UITextInputComponent>(entity);

        if (!input.interactable)
        {
            input.isFocused = false;
            input.isHovered = false;
            continue;
        }

        bool hovered = UILayout::IsPointInRect(uiMousePos, rect.computedScreenRect, rect.computedRotation);
        input.isHovered = hovered;

        if (mousePressed)
        {
            if (hovered)
            {
                input.isFocused = true;
                input.cursorPosition = static_cast<int>(input.text.length());
                input.cursorBlinkTimer = 0.0f;
                input.cursorVisible = true;
            }
            else
            {
                input.isFocused = false;
            }
        }

        if (input.isFocused)
        {
            input.cursorBlinkTimer += dt;
            if (input.cursorBlinkTimer >= 0.5f)
            {
                input.cursorBlinkTimer = 0.0f;
                input.cursorVisible = !input.cursorVisible;
            }

            bool textChanged = false;

            for (uint32_t cp : typedChars)
            {
                if (cp >= 32 && cp <= 126 && input.text.length() < input.maxLength)
                {
                    char ch = static_cast<char>(cp);
                    input.cursorPosition = std::clamp(input.cursorPosition, 0, (int)input.text.length());
                    input.text.insert(input.text.begin() + input.cursorPosition, ch);
                    input.cursorPosition++;
                    input.cursorVisible = true;
                    input.cursorBlinkTimer = 0.0f;
                    textChanged = true;
                }
            }

            if (backspacePressed && input.cursorPosition > 0 && !input.text.empty())
            {
                input.cursorPosition = std::clamp(input.cursorPosition, 0, (int)input.text.length());
                input.text.erase(input.cursorPosition - 1, 1);
                input.cursorPosition--;
                input.cursorVisible = true;
                input.cursorBlinkTimer = 0.0f;
                textChanged = true;
            }

            if (deletePressed && input.cursorPosition < (int)input.text.length())
            {
                input.text.erase(input.cursorPosition, 1);
                input.cursorVisible = true;
                input.cursorBlinkTimer = 0.0f;
                textChanged = true;
            }

            if (leftPressed && input.cursorPosition > 0)
            {
                input.cursorPosition--;
                input.cursorVisible = true;
                input.cursorBlinkTimer = 0.0f;
            }

            if (rightPressed && input.cursorPosition < (int)input.text.length())
            {
                input.cursorPosition++;
                input.cursorVisible = true;
                input.cursorBlinkTimer = 0.0f;
            }

            if (textChanged)
            {
                if (input.onValueChanged) input.onValueChanged(input.text);
                if (!input.luaCallback.empty())
                {
                    DispatchLuaCallbackString(scene, entity, input.luaCallback, input.text);
                }
            }

            if (enterPressed)
            {
                if (input.onSubmit) input.onSubmit(input.text);
                if (!input.luaSubmitCallback.empty())
                {
                    DispatchLuaCallbackString(scene, entity, input.luaSubmitCallback, input.text);
                }
            }
        }
    }

    auto scrollView = scene->GetRegistry().view<RectTransformComponent, UIScrollRectComponent>();
    for (auto entity : scrollView)
    {
        if (!scene->IsEntityActive(entity)) continue;
        auto& rect = scrollView.get<RectTransformComponent>(entity);
        auto& scroll = scrollView.get<UIScrollRectComponent>(entity);

        bool hovered = UILayout::IsPointInRect(uiMousePos, rect.computedScreenRect, rect.computedRotation);

        float viewH = rect.computedScreenRect.w - rect.computedScreenRect.y;

        glm::vec4 thumbRect = glm::vec4(0.0f);
        if (scroll.showScrollbar && scroll.vertical && scroll.contentSize.y > viewH)
        {
            float trackH = viewH;
            float thumbH = std::max(20.0f, trackH * (viewH / scroll.contentSize.y));
            float maxTravel = trackH - thumbH;
            float thumbY = rect.computedScreenRect.w - thumbH - (scroll.scrollPosition.y * maxTravel);
            thumbRect = glm::vec4(
                rect.computedScreenRect.z - scroll.scrollbarWidth,
                thumbY,
                rect.computedScreenRect.z,
                thumbY + thumbH
            );
        }

        scroll.isThumbHovered = UILayout::IsPointInRect(uiMousePos, thumbRect, rect.computedRotation);

        if (hovered && scroll.vertical)
        {
            float mouseScroll = Input::GetMouseScrollY();
            if (std::abs(mouseScroll) > 0.001f)
            {
                float maxScrollY = std::max(1.0f, scroll.contentSize.y - viewH);
                scroll.scrollPosition.y -= (mouseScroll * scroll.scrollSpeed) / maxScrollY;
                scroll.scrollPosition.y = std::clamp(scroll.scrollPosition.y, 0.0f, 1.0f);
                if (scroll.onScroll) scroll.onScroll(scroll.scrollPosition);
                if (!scroll.luaCallback.empty())
                {
                    DispatchLuaCallback(scene, entity, scroll.luaCallback, scroll.scrollPosition.y);
                }
            }
        }

        if (scroll.isThumbHovered && mousePressed)
        {
            scroll.isDraggingVertical = true;
            scroll.dragStartMouseY = uiMousePos.y;
            scroll.dragStartScrollY = scroll.scrollPosition.y;
        }

        if (mouseReleased || !mouseHeld)
        {
            scroll.isDraggingVertical = false;
        }

        if (scroll.isDraggingVertical && mouseHeld)
        {
            float thumbH = std::max(20.0f, viewH * (viewH / scroll.contentSize.y));
            float maxTravel = viewH - thumbH;
            if (maxTravel > 0.0f)
            {
                float deltaY = uiMousePos.y - scroll.dragStartMouseY;
                scroll.scrollPosition.y = std::clamp(scroll.dragStartScrollY - (deltaY / maxTravel), 0.0f, 1.0f);
                if (scroll.onScroll) scroll.onScroll(scroll.scrollPosition);
                if (!scroll.luaCallback.empty())
                {
                    DispatchLuaCallback(scene, entity, scroll.luaCallback, scroll.scrollPosition.y);
                }
            }
        }
    }

    auto tabView = scene->GetRegistry().view<RectTransformComponent, UITabContainerComponent>();
    for (auto entity : tabView)
    {
        if (!scene->IsEntityActive(entity)) continue;
        auto& rect = tabView.get<RectTransformComponent>(entity);
        auto& tabs = tabView.get<UITabContainerComponent>(entity);

        tabs.hoveredTabIndex = -1;
        float viewW = rect.computedScreenRect.z - rect.computedScreenRect.x;
        int numTabs = static_cast<int>(tabs.tabTitles.size());
        if (numTabs <= 0) continue;

        float tabW = (viewW - (tabs.tabSpacing * (numTabs - 1))) / static_cast<float>(numTabs);
        float tabTop = rect.computedScreenRect.w;
        float tabBottom = tabTop - tabs.tabHeight;

        for (int i = 0; i < numTabs; i++)
        {
            float tabLeft = rect.computedScreenRect.x + i * (tabW + tabs.tabSpacing);
            glm::vec4 tabRect = glm::vec4(tabLeft, tabBottom, tabLeft + tabW, tabTop);

            if (UILayout::IsPointInRect(uiMousePos, tabRect, rect.computedRotation))
            {
                tabs.hoveredTabIndex = i;
                if (mousePressed && tabs.activeTabIndex != i)
                {
                    tabs.activeTabIndex = i;
                    if (tabs.onTabChanged) tabs.onTabChanged(i);
                    if (!tabs.luaCallback.empty())
                    {
                        DispatchLuaCallback(scene, entity, tabs.luaCallback, static_cast<float>(i));
                    }
                }
                break;
            }
        }
    }

    // Raycast check
    bool hoveredUI = false;
    auto rectView = scene->GetRegistry().view<RectTransformComponent>();
    for (auto entity : rectView)
    {
        if (!scene->IsEntityActive(entity))
            continue;

        const auto& rect = rectView.get<RectTransformComponent>(entity);
        if (!rect.raycastTarget) continue;

        bool hasUI = scene->HasComponent<UIButtonComponent>(entity) ||
                     scene->HasComponent<UIImageComponent>(entity) ||
                     scene->HasComponent<UITextComponent>(entity) ||
                     scene->HasComponent<UISliderComponent>(entity) ||
                     scene->HasComponent<UIProgressBarComponent>(entity) ||
                     scene->HasComponent<UICheckboxComponent>(entity) ||
                     scene->HasComponent<UIPanelComponent>(entity) ||
                     scene->HasComponent<UITextInputComponent>(entity) ||
                     scene->HasComponent<UIScrollRectComponent>(entity) ||
                     scene->HasComponent<UITabContainerComponent>(entity);

        if (hasUI && UILayout::IsPointInRect(uiMousePos, rect.computedScreenRect, rect.computedRotation))
        {
            hoveredUI = true;
            break;
        }
    }
    Input::SetMouseOverUI(hoveredUI);
}

void UIInteraction::DispatchLuaCallbackString(Scene* scene, entt::entity entity, const std::string& callbackName, const std::string& param)
{
    if (callbackName.empty() || !scene) return;

    auto TryInvoke = [&](ScriptComponent& script, entt::entity targetScriptEntity) -> bool {
        if (!script.luaInstance.valid()) return false;
        sol::protected_function func = script.luaInstance[callbackName];
        if (func.valid()) {
            sol::protected_function_result result = func(script.luaInstance, param, Entity(entity, scene));
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
