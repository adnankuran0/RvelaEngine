#include "rvelapch.h"
#include "UIRenderPass.h"
#include "UIRenderer.h"
#include "UILayout.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Scene/Components/UIComponents.h"
#include "Asset/AssetManager.h"
#include "Renderer/TextureCache.h"
#include "Renderer/Shader.h"
#include "Renderer/Camera.h"
#include "Core/Engine.h"
#include <glad/gl.h>
#include <algorithm>

namespace rv {

void UIRenderPass::Render(Scene* scene, float viewportWidth, float viewportHeight)
{
    if (!scene) return;

    // Ensure layout hierarchy is computed for current viewport size
    UILayout::UpdateCanvasLayouts(scene, viewportWidth, viewportHeight);

    GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean depthMaskWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);
    GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);

    Camera* camera = Engine::Get() ? Engine::Get()->GetCamera() : nullptr;

    struct CanvasEntry {
        entt::entity entity;
        int sortOrder;
    };
    std::vector<CanvasEntry> canvases;

    auto canvasView = scene->GetRegistry().view<UICanvasComponent>();
    for (auto canvasEntity : canvasView)
    {
        const auto& canvas = canvasView.get<UICanvasComponent>(canvasEntity);
        canvases.push_back({ canvasEntity, canvas.sortOrder });
    }

    std::sort(canvases.begin(), canvases.end(), [](const CanvasEntry& a, const CanvasEntry& b) {
        return a.sortOrder < b.sortOrder;
    });

    if (canvases.empty())
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        UIRenderer::Begin(viewportWidth, viewportHeight);
        auto rectView = scene->GetRegistry().view<RectTransformComponent>();
        for (auto entity : rectView)
        {
            entt::entity parent = scene->GetParent(entity);
            if (parent == entt::null || !scene->HasComponent<RectTransformComponent>(parent))
            {
                UILayout::UpdateLayoutRecursively(scene, entity, { 0.0f, 0.0f }, { viewportWidth, viewportHeight }, { viewportWidth * 0.5f, viewportHeight * 0.5f }, 0.0f);
                RenderEntityRecursively(scene, entity);
            }
        }
        UIRenderer::End();
    }
    else
    {
        for (const auto& canvasEntry : canvases)
        {
            entt::entity canvasEntity = canvasEntry.entity;
            const auto& canvas = scene->GetComponent<UICanvasComponent>(canvasEntity);

            glm::mat4 customProjMatrix(1.0f);
            bool isWorldSpace = (canvas.mode == CanvasMode::WorldSpace && camera != nullptr);

            if (isWorldSpace)
            {
                glm::mat4 canvasWorldMatrix = UILayout::CalculateWorldSpaceCanvasMatrix(scene, canvasEntity, canvas, camera);

                glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));
                glm::mat4 viewProj = camera->GetProjectionMatrix() * camera->GetViewMatrix();
                customProjMatrix = viewProj * canvasWorldMatrix * scaleMatrix;

                if (canvas.depthTest) {
                    glEnable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                } else {
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                }
            }
            else
            {
                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
            }

            UIRenderer::Begin(viewportWidth, viewportHeight, customProjMatrix);

            RenderEntityRecursively(scene, canvasEntity);

            UIRenderer::End();
        }
    }

    // Restore gl state
    if (depthTestWasEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(depthMaskWasEnabled);
    if (cullFaceWasEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (blendWasEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
}

void UIRenderPass::RenderEntityIDs(Scene* scene, float viewportWidth, float viewportHeight)
{
    if (!scene) return;

    UILayout::UpdateCanvasLayouts(scene, viewportWidth, viewportHeight);

    static std::shared_ptr<Shader> s_UIEntityShader = nullptr;
    if (!s_UIEntityShader) {
        s_UIEntityShader = std::make_shared<Shader>("UI_EntityBuffer", ENGINE_PATH("Shaders\\ui_entitybuffer.glsl"));
    }

    Camera* camera = Engine::Get() ? Engine::Get()->GetCamera() : nullptr;

    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean depthMaskWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);
    GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);

    struct CanvasEntry {
        entt::entity entity;
        int sortOrder;
    };
    std::vector<CanvasEntry> canvases;

    auto canvasView = scene->GetRegistry().view<UICanvasComponent>();
    for (auto canvasEntity : canvasView)
    {
        const auto& canvas = canvasView.get<UICanvasComponent>(canvasEntity);
        canvases.push_back({ canvasEntity, canvas.sortOrder });
    }

    std::sort(canvases.begin(), canvases.end(), [](const CanvasEntry& a, const CanvasEntry& b) {
        return a.sortOrder < b.sortOrder;
    });

    glDisable(GL_BLEND);

    if (canvases.empty())
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        UIRenderer::Begin(viewportWidth, viewportHeight, glm::mat4(1.0f), s_UIEntityShader);
        auto rectView = scene->GetRegistry().view<RectTransformComponent>();
        for (auto entity : rectView)
        {
            entt::entity parent = scene->GetParent(entity);
            if (parent == entt::null || !scene->HasComponent<RectTransformComponent>(parent))
            {
                RenderEntityIDRecursively(scene, entity, s_UIEntityShader);
            }
        }
        UIRenderer::End();
    }
    else
    {
        for (const auto& canvasEntry : canvases)
        {
            entt::entity canvasEntity = canvasEntry.entity;
            const auto& canvas = scene->GetComponent<UICanvasComponent>(canvasEntity);

            glm::mat4 customProjMatrix(1.0f);
            bool isWorldSpace = (canvas.mode == CanvasMode::WorldSpace && camera != nullptr);

            if (isWorldSpace)
            {
                glm::mat4 canvasWorldMatrix = UILayout::CalculateWorldSpaceCanvasMatrix(scene, canvasEntity, canvas, camera);

                glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));
                glm::mat4 viewProj = camera->GetProjectionMatrix() * camera->GetViewMatrix();
                customProjMatrix = viewProj * canvasWorldMatrix * scaleMatrix;

                if (canvas.depthTest) {
                    glEnable(GL_DEPTH_TEST);
                    glDepthMask(GL_TRUE);
                } else {
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                }
            }
            else
            {
                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
                customProjMatrix = glm::ortho(0.0f, viewportWidth, 0.0f, viewportHeight, -100.0f, 100.0f);
            }

            UIRenderer::Begin(viewportWidth, viewportHeight, customProjMatrix, s_UIEntityShader);
            RenderEntityIDRecursively(scene, canvasEntity, s_UIEntityShader);
            UIRenderer::End();
        }
    }

    // Restore GL states
    if (blendWasEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (depthTestWasEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(depthMaskWasEnabled);
    if (cullFaceWasEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
}

void UIRenderPass::RenderSelectionMask(Scene* scene, entt::entity selectedEntity, float viewportWidth, float viewportHeight)
{
    if (!scene || !scene->GetRegistry().valid(selectedEntity)) return;

    UILayout::UpdateCanvasLayouts(scene, viewportWidth, viewportHeight);

    if (!scene->HasComponent<RectTransformComponent>(selectedEntity)) return;

    Camera* camera = Engine::Get() ? Engine::Get()->GetCamera() : nullptr;

    entt::entity canvasEntity = selectedEntity;
    while (canvasEntity != entt::null && !scene->HasComponent<UICanvasComponent>(canvasEntity))
    {
        canvasEntity = scene->GetParent(canvasEntity);
    }

    glm::mat4 customProjMatrix(1.0f);
    if (canvasEntity != entt::null && scene->HasComponent<UICanvasComponent>(canvasEntity))
    {
        const auto& canvas = scene->GetComponent<UICanvasComponent>(canvasEntity);
        if (canvas.mode == CanvasMode::WorldSpace && camera != nullptr)
        {
            glm::mat4 canvasWorldMatrix = UILayout::CalculateWorldSpaceCanvasMatrix(scene, canvasEntity, canvas, camera);

            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));
            glm::mat4 viewProj = camera->GetProjectionMatrix() * camera->GetViewMatrix();
            customProjMatrix = viewProj * canvasWorldMatrix * scaleMatrix;
        }
        else
        {
            customProjMatrix = glm::ortho(0.0f, viewportWidth, 0.0f, viewportHeight, -100.0f, 100.0f);
        }
    }
    else
    {
        customProjMatrix = glm::ortho(0.0f, viewportWidth, 0.0f, viewportHeight, -100.0f, 100.0f);
    }

    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean depthMaskWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskWasEnabled);
    GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);

    const auto& rect = scene->GetComponent<RectTransformComponent>(selectedEntity);
    UIRenderer::Begin(viewportWidth, viewportHeight, customProjMatrix);
    UIRenderer::DrawQuadRotated(rect.computedScreenRect, rect.computedRotation, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    UIRenderer::End();

    if (blendWasEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (depthTestWasEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(depthMaskWasEnabled);
    if (cullFaceWasEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
}

void UIRenderPass::RenderEntityRecursively(Scene* scene, entt::entity entity)
{
    if (!scene->IsEntityActive(entity))
        return;

    if (!scene->HasComponent<RectTransformComponent>(entity))
    {
        if (scene->HasComponent<UITextComponent>(entity) ||
            scene->HasComponent<UIImageComponent>(entity) ||
            scene->HasComponent<UIButtonComponent>(entity) ||
            scene->HasComponent<UISliderComponent>(entity) ||
            scene->HasComponent<UIProgressBarComponent>(entity) ||
            scene->HasComponent<UICheckboxComponent>(entity) ||
            scene->HasComponent<UIPanelComponent>(entity) ||
            scene->HasComponent<UITextInputComponent>(entity) ||
            scene->HasComponent<UIScrollRectComponent>(entity) ||
            scene->HasComponent<UITabContainerComponent>(entity))
        {
            scene->AddComponent<RectTransformComponent>(entity);
        }
    }

    if (scene->HasComponent<RectTransformComponent>(entity))
    {
        const auto& rect = scene->GetComponent<RectTransformComponent>(entity);
        const glm::vec4& screenRect = rect.computedScreenRect;
        glm::vec2 rectCenter = { (screenRect.x + screenRect.z) * 0.5f, (screenRect.y + screenRect.w) * 0.5f };

        if (scene->HasComponent<UIButtonComponent>(entity))
        {
            const auto& button = scene->GetComponent<UIButtonComponent>(entity);
            glm::vec4 currentColor = button.normalColor;
            if (!button.interactable) currentColor = button.disabledColor;
            else if (button.isPressed) currentColor = button.pressedColor;
            else if (button.isHovered) currentColor = button.hoverColor;

            uint32_t texId = 0;
            AssetUUID targetUUID = AssetUUID::Invalid();
            if (button.normalTextureUUID.IsValid())
            {
                if (!button.interactable && button.disabledTextureUUID.IsValid()) targetUUID = button.disabledTextureUUID;
                else if (button.isPressed && button.pressedTextureUUID.IsValid()) targetUUID = button.pressedTextureUUID;
                else if (button.isHovered && button.hoverTextureUUID.IsValid()) targetUUID = button.hoverTextureUUID;
                else targetUUID = button.normalTextureUUID;
            }
            else if (scene->HasComponent<UIImageComponent>(entity))
            {
                const auto& img = scene->GetComponent<UIImageComponent>(entity);
                targetUUID = img.textureUUID;
            }

            if (targetUUID.IsValid())
            {
                auto texAsset = AssetManager::Get().GetAsset<TextureAsset>(targetUUID);
                if (texAsset) texId = TextureCache::Get().GetOrCreate(texAsset).GetID();
            }

            if (button.imageType == ImageType::Sliced && texId != 0)
            {
                UIRenderer::DrawNineSlice(screenRect, currentColor, button.borderSize, texId);
            }
            else
            {
                UIRenderer::DrawQuadRotated(screenRect, rect.computedRotation, currentColor, texId, button.cornerRadius);
            }

            // button text
            if (!button.text.empty()) {
                glm::vec2 textPos = rectCenter;
                float maxW = screenRect.z - screenRect.x;
                UIRenderer::DrawTextString(button.text, textPos, button.fontSize, button.textColor, TextAlignment::Center, rect.computedRotation, rectCenter, nullptr, button.isBold, button.isOutline, button.outlineSize, button.outlineColor, maxW);
            }
        }
        else if (scene->HasComponent<UIImageComponent>(entity))
        {
            const auto& image = scene->GetComponent<UIImageComponent>(entity);
            uint32_t texId = 0;
            if (image.textureUUID.IsValid()) {
                auto texAsset = AssetManager::Get().GetAsset<TextureAsset>(image.textureUUID);
                if (texAsset) texId = TextureCache::Get().GetOrCreate(texAsset).GetID();
            }

            if (image.imageType == ImageType::Sliced)
            {
                UIRenderer::DrawNineSlice(screenRect, image.color, 8.0f, texId);
            }
            else
            {
                UIRenderer::DrawQuadRotated(screenRect, rect.computedRotation, image.color, texId, image.cornerRadius);
            }
        }

        if (scene->HasComponent<UIPanelComponent>(entity))
        {
            const auto& panel = scene->GetComponent<UIPanelComponent>(entity);
            uint32_t texId = 0;
            if (panel.backgroundTextureUUID.IsValid()) {
                auto texAsset = AssetManager::Get().GetAsset<TextureAsset>(panel.backgroundTextureUUID);
                if (texAsset) texId = TextureCache::Get().GetOrCreate(texAsset).GetID();
            }

            if (panel.imageType == ImageType::Sliced && texId != 0)
            {
                UIRenderer::DrawNineSlice(screenRect, panel.backgroundColor, 8.0f, texId);
            }
            else
            {
                UIRenderer::DrawQuadRotated(screenRect, rect.computedRotation, panel.backgroundColor, texId, panel.cornerRadius);
            }

            if (panel.borderThickness > 0.0f && panel.borderColor.a > 0.0f)
            {
                float bt = panel.borderThickness;
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.x, screenRect.y, screenRect.x + bt, screenRect.w), rect.computedRotation, panel.borderColor, 0, 0.0f, rectCenter);
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.z - bt, screenRect.y, screenRect.z, screenRect.w), rect.computedRotation, panel.borderColor, 0, 0.0f, rectCenter);
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.x, screenRect.y, screenRect.z, screenRect.y + bt), rect.computedRotation, panel.borderColor, 0, 0.0f, rectCenter);
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.x, screenRect.w - bt, screenRect.z, screenRect.w), rect.computedRotation, panel.borderColor, 0, 0.0f, rectCenter);
            }

            if (panel.isDraggable || !panel.title.empty())
            {
                glm::vec4 headerRect = glm::vec4(screenRect.x, screenRect.w - panel.headerHeight, screenRect.z, screenRect.w);
                UIRenderer::DrawQuadRotated(headerRect, rect.computedRotation, panel.headerColor, 0, panel.cornerRadius, rectCenter);
                if (!panel.title.empty())
                {
                    glm::vec2 titlePos = { screenRect.x + 12.0f, screenRect.w - panel.headerHeight * 0.5f };
                    UIRenderer::DrawTextString(panel.title, titlePos, panel.titleFontSize, panel.titleColor, TextAlignment::Left, rect.computedRotation, rectCenter, nullptr, true);
                }
            }
        }

        if (scene->HasComponent<UISliderComponent>(entity))
        {
            const auto& slider = scene->GetComponent<UISliderComponent>(entity);
            UIRenderer::DrawQuadRotated(screenRect, rect.computedRotation, slider.backgroundColor, 0, 4.0f, rectCenter);

            float width = screenRect.z - screenRect.x;
            float t = std::clamp((slider.value - slider.minValue) / (slider.maxValue - slider.minValue), 0.0f, 1.0f);
            
            glm::vec4 fillRect = glm::vec4(screenRect.x, screenRect.y, screenRect.x + width * t, screenRect.w);
            UIRenderer::DrawQuadRotated(fillRect, rect.computedRotation, slider.fillColor, 0, 4.0f, rectCenter);

            float handleW = 12.0f;
            float handleX = screenRect.x + width * t - handleW * 0.5f;
            glm::vec4 handleRect = glm::vec4(handleX, screenRect.y - 2.0f, handleX + handleW, screenRect.w + 2.0f);
            UIRenderer::DrawQuadRotated(handleRect, rect.computedRotation, slider.handleColor, 0, 6.0f, rectCenter);
        }

        if (scene->HasComponent<UIProgressBarComponent>(entity))
        {
            const auto& pb = scene->GetComponent<UIProgressBarComponent>(entity);
            uint32_t bgTexId = 0;
            if (pb.backgroundTextureUUID.IsValid()) {
                auto texAsset = AssetManager::Get().GetAsset<TextureAsset>(pb.backgroundTextureUUID);
                if (texAsset) bgTexId = TextureCache::Get().GetOrCreate(texAsset).GetID();
            }
            uint32_t fillTexId = 0;
            if (pb.fillTextureUUID.IsValid()) {
                auto texAsset = AssetManager::Get().GetAsset<TextureAsset>(pb.fillTextureUUID);
                if (texAsset) fillTexId = TextureCache::Get().GetOrCreate(texAsset).GetID();
            }

            UIRenderer::DrawQuadRotated(screenRect, rect.computedRotation, pb.backgroundColor, bgTexId, pb.cornerRadius, rectCenter);

            float width = screenRect.z - screenRect.x;
            float height = screenRect.w - screenRect.y;
            float t = std::clamp(pb.value, 0.0f, 1.0f);

            glm::vec4 fillRect = screenRect;
            if (pb.direction == UIProgressBarDirection::LeftToRight)
                fillRect = glm::vec4(screenRect.x, screenRect.y, screenRect.x + width * t, screenRect.w);
            else if (pb.direction == UIProgressBarDirection::RightToLeft)
                fillRect = glm::vec4(screenRect.z - width * t, screenRect.y, screenRect.z, screenRect.w);
            else if (pb.direction == UIProgressBarDirection::BottomToTop)
                fillRect = glm::vec4(screenRect.x, screenRect.y, screenRect.z, screenRect.y + height * t);
            else if (pb.direction == UIProgressBarDirection::TopToBottom)
                fillRect = glm::vec4(screenRect.x, screenRect.w - height * t, screenRect.z, screenRect.w);

            UIRenderer::DrawQuadRotated(fillRect, rect.computedRotation, pb.fillColor, fillTexId, pb.cornerRadius, rectCenter);

            if (pb.borderThickness > 0.0f && pb.borderColor.a > 0.0f)
            {
                float bt = pb.borderThickness;
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.x, screenRect.y, screenRect.x + bt, screenRect.w), rect.computedRotation, pb.borderColor, 0, 0.0f, rectCenter);
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.z - bt, screenRect.y, screenRect.z, screenRect.w), rect.computedRotation, pb.borderColor, 0, 0.0f, rectCenter);
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.x, screenRect.y, screenRect.z, screenRect.y + bt), rect.computedRotation, pb.borderColor, 0, 0.0f, rectCenter);
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.x, screenRect.w - bt, screenRect.z, screenRect.w), rect.computedRotation, pb.borderColor, 0, 0.0f, rectCenter);
            }

            if (pb.showPercentageText)
            {
                std::string pctText = std::to_string(static_cast<int>(t * 100.0f + 0.5f)) + "%";
                UIRenderer::DrawTextString(pctText, rectCenter, pb.percentageFontSize, pb.percentageTextColor, TextAlignment::Center, rect.computedRotation, rectCenter, nullptr, true);
            }
        }

        if (scene->HasComponent<UITextInputComponent>(entity))
        {
            const auto& input = scene->GetComponent<UITextInputComponent>(entity);
            UIRenderer::DrawQuadRotated(screenRect, rect.computedRotation, input.backgroundColor, 0, input.cornerRadius, rectCenter);

            if (input.borderThickness > 0.0f)
            {
                glm::vec4 bColor = input.isFocused ? input.focusedBorderColor : (input.isHovered ? input.focusedBorderColor * 0.8f : input.borderColor);
                float bt = input.borderThickness;
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.x, screenRect.y, screenRect.x + bt, screenRect.w), rect.computedRotation, bColor, 0, 0.0f, rectCenter);
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.z - bt, screenRect.y, screenRect.z, screenRect.w), rect.computedRotation, bColor, 0, 0.0f, rectCenter);
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.x, screenRect.y, screenRect.z, screenRect.y + bt), rect.computedRotation, bColor, 0, 0.0f, rectCenter);
                UIRenderer::DrawQuadRotated(glm::vec4(screenRect.x, screenRect.w - bt, screenRect.z, screenRect.w), rect.computedRotation, bColor, 0, 0.0f, rectCenter);
            }

            glm::vec2 textPos = { screenRect.x + input.textPadding.x, rectCenter.y };
            float maxW = screenRect.z - textPos.x - 4.0f;
            if (input.text.empty())
            {
                UIRenderer::DrawTextString(input.placeholder, textPos, input.fontSize, input.placeholderColor, TextAlignment::Left, rect.computedRotation, rectCenter, nullptr, false, false, 0.0f, glm::vec4(0), maxW);
            }
            else
            {
                std::string displayText = input.isPassword ? std::string(input.text.length(), '*') : input.text;
                UIRenderer::DrawTextString(displayText, textPos, input.fontSize, input.textColor, TextAlignment::Left, rect.computedRotation, rectCenter, nullptr, false, false, 0.0f, glm::vec4(0), maxW);
            }

            if (input.isFocused && input.cursorVisible)
            {
                float cursorOffset = input.cursorPosition * (input.fontSize * 0.52f);
                glm::vec4 cursorRect = glm::vec4(textPos.x + cursorOffset, rectCenter.y - input.fontSize * 0.45f, textPos.x + cursorOffset + 2.0f, rectCenter.y + input.fontSize * 0.45f);
                UIRenderer::DrawQuadRotated(cursorRect, rect.computedRotation, input.cursorColor, 0, 0.0f, rectCenter);
            }
        }

        if (scene->HasComponent<UITabContainerComponent>(entity))
        {
            const auto& tabs = scene->GetComponent<UITabContainerComponent>(entity);
            float viewW = screenRect.z - screenRect.x;
            int numTabs = static_cast<int>(tabs.tabTitles.size());

            glm::vec4 contentRect = glm::vec4(screenRect.x, screenRect.y, screenRect.z, screenRect.w - tabs.tabHeight);
            UIRenderer::DrawQuadRotated(contentRect, rect.computedRotation, tabs.contentBackgroundColor, 0, tabs.contentCornerRadius, rectCenter);

            if (numTabs > 0)
            {
                float tabW = (viewW - (tabs.tabSpacing * (numTabs - 1))) / static_cast<float>(numTabs);
                float tabTop = screenRect.w;
                float tabBottom = tabTop - tabs.tabHeight;

                for (int i = 0; i < numTabs; i++)
                {
                    float tabLeft = screenRect.x + i * (tabW + tabs.tabSpacing);
                    glm::vec4 tabRect = glm::vec4(tabLeft, tabBottom, tabLeft + tabW, tabTop);
                    glm::vec4 tabCol = (i == tabs.activeTabIndex) ? tabs.tabActiveColor : ((i == tabs.hoveredTabIndex) ? tabs.tabHoverColor : tabs.tabNormalColor);
                    UIRenderer::DrawQuadRotated(tabRect, rect.computedRotation, tabCol, 0, tabs.tabCornerRadius, rectCenter);

                    glm::vec2 tabTextCenter = { (tabRect.x + tabRect.z) * 0.5f, (tabRect.y + tabRect.w) * 0.5f };
                    UIRenderer::DrawTextString(tabs.tabTitles[i], tabTextCenter, tabs.tabFontSize, tabs.tabTextColor, TextAlignment::Center, rect.computedRotation, rectCenter, nullptr, (i == tabs.activeTabIndex));
                }
            }
        }

        if (scene->HasComponent<UICheckboxComponent>(entity))
        {
            const auto& cb = scene->GetComponent<UICheckboxComponent>(entity);
            float boxSize = (cb.boxSize > 0.0f) ? cb.boxSize : 20.0f;

            float boxY = rectCenter.y - boxSize * 0.5f;
            glm::vec4 boxRect = glm::vec4(screenRect.x, boxY, screenRect.x + boxSize, boxY + boxSize);

            glm::vec4 bColor = cb.boxColor;
            if (!cb.interactable) bColor *= 0.6f;
            else if (cb.isHovered) bColor += glm::vec4(0.1f, 0.1f, 0.1f, 0.0f);
            UIRenderer::DrawQuadRotated(boxRect, rect.computedRotation, bColor, 0, 4.0f, rectCenter);

            if (cb.isChecked)
            {
                float innerMargin = boxSize * 0.25f;
                glm::vec4 checkRect = glm::vec4(boxRect.x + innerMargin, boxRect.y + innerMargin, boxRect.z - innerMargin, boxRect.w - innerMargin);
                UIRenderer::DrawQuadRotated(checkRect, rect.computedRotation, cb.checkmarkColor, 0, 2.0f, rectCenter);
            }

            if (!cb.label.empty())
            {
                glm::vec2 labelPos = { screenRect.x + boxSize + 8.0f, rectCenter.y };
                float maxW = (screenRect.z - labelPos.x) > 0.0f ? (screenRect.z - labelPos.x) : 0.0f;
                UIRenderer::DrawTextString(cb.label, labelPos, cb.fontSize, cb.textColor, TextAlignment::Left, rect.computedRotation, rectCenter, nullptr, false, false, 0.0f, glm::vec4(0), maxW);
            }
        }

        if (scene->HasComponent<UITextComponent>(entity))
        {
            const auto& textComp = scene->GetComponent<UITextComponent>(entity);
            glm::vec2 pos = { screenRect.x, rectCenter.y };
            if (textComp.alignment == TextAlignment::Center) {
                pos.x = rectCenter.x;
            } else if (textComp.alignment == TextAlignment::Right) {
                pos.x = screenRect.z;
            }

            if (textComp.verticalAlignment == TextVerticalAlignment::Top) {
                pos.y = screenRect.w;
            } else if (textComp.verticalAlignment == TextVerticalAlignment::Bottom) {
                pos.y = screenRect.y;
            } else {
                pos.y = rectCenter.y;
            }

            float maxW = textComp.wordWrap ? (screenRect.z - screenRect.x) : 0.0f;
            UIRenderer::DrawTextString(textComp.text, pos, textComp.fontSize, textComp.color, textComp.alignment, rect.computedRotation, rectCenter, nullptr, textComp.isBold, textComp.isOutline, textComp.outlineSize, textComp.outlineColor, maxW, textComp.lineSpacing, textComp.characterSpacing, textComp.verticalAlignment);
        }
    }

    bool clipScissor = false;
    if (scene->HasComponent<UIScrollRectComponent>(entity))
    {
        clipScissor = true;
    }
    else if (scene->HasComponent<UIPanelComponent>(entity))
    {
        clipScissor = scene->GetComponent<UIPanelComponent>(entity).clipChildren;
    }

    if (scene->HasComponent<RectTransformComponent>(entity) && clipScissor)
    {
        const auto& rect = scene->GetComponent<RectTransformComponent>(entity);
        UIRenderer::PushScissorRect(rect.computedScreenRect);
    }

    if (scene->HasComponent<SceneTreeComponent>(entity))
    {
        const auto& children = scene->GetComponent<SceneTreeComponent>(entity).children;

        if (scene->HasComponent<UITabContainerComponent>(entity))
        {
            const auto& tabs = scene->GetComponent<UITabContainerComponent>(entity);
            if (tabs.activeTabIndex >= 0 && tabs.activeTabIndex < (int)children.size())
            {
                RenderEntityRecursively(scene, children[tabs.activeTabIndex]);
            }
            else
            {
                for (auto child : children)
                {
                    RenderEntityRecursively(scene, child);
                }
            }
        }
        else
        {
            for (auto child : children)
            {
                RenderEntityRecursively(scene, child);
            }
        }
    }

    if (scene->HasComponent<RectTransformComponent>(entity) && clipScissor)
    {
        UIRenderer::PopScissorRect();
    }

    if (scene->HasComponent<RectTransformComponent>(entity) && scene->HasComponent<UIScrollRectComponent>(entity))
    {
        const auto& rect = scene->GetComponent<RectTransformComponent>(entity);
        const auto& scroll = scene->GetComponent<UIScrollRectComponent>(entity);
        const glm::vec4& screenRect = rect.computedScreenRect;
        glm::vec2 rectCenter = { (screenRect.x + screenRect.z) * 0.5f, (screenRect.y + screenRect.w) * 0.5f };
        float viewH = screenRect.w - screenRect.y;

        if (scroll.showScrollbar && scroll.vertical && scroll.contentSize.y > viewH)
        {
            glm::vec4 trackRect = glm::vec4(screenRect.z - scroll.scrollbarWidth, screenRect.y, screenRect.z, screenRect.w);
            UIRenderer::DrawQuadRotated(trackRect, rect.computedRotation, scroll.trackColor, 0, scroll.cornerRadius, rectCenter);

            float thumbH = std::max(20.0f, viewH * (viewH / scroll.contentSize.y));
            float maxTravel = viewH - thumbH;
            float thumbY = screenRect.w - thumbH - (scroll.scrollPosition.y * maxTravel);
            glm::vec4 thumbRect = glm::vec4(trackRect.x, thumbY, trackRect.z, thumbY + thumbH);
            glm::vec4 thumbCol = (scroll.isDraggingVertical || scroll.isThumbHovered) ? scroll.thumbHoverColor : scroll.thumbColor;
            UIRenderer::DrawQuadRotated(thumbRect, rect.computedRotation, thumbCol, 0, scroll.cornerRadius, rectCenter);
        }
    }
}

void UIRenderPass::RenderEntityIDRecursively(Scene* scene, entt::entity entity, std::shared_ptr<Shader> shader)
{
    if (!scene->IsEntityActive(entity))
        return;

    if (!scene->HasComponent<RectTransformComponent>(entity))
    {
        if (scene->HasComponent<UITextComponent>(entity) ||
            scene->HasComponent<UIImageComponent>(entity) ||
            scene->HasComponent<UIButtonComponent>(entity) ||
            scene->HasComponent<UISliderComponent>(entity) ||
            scene->HasComponent<UIProgressBarComponent>(entity) ||
            scene->HasComponent<UICheckboxComponent>(entity) ||
            scene->HasComponent<UIPanelComponent>(entity) ||
            scene->HasComponent<UITextInputComponent>(entity) ||
            scene->HasComponent<UIScrollRectComponent>(entity) ||
            scene->HasComponent<UITabContainerComponent>(entity))
        {
            scene->AddComponent<RectTransformComponent>(entity);
        }
    }

    if (scene->HasComponent<RectTransformComponent>(entity))
    {
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
        if (hasUI)
        {
            const auto& rect = scene->GetComponent<RectTransformComponent>(entity);
            shader->use();
            shader->setUInt("u_EntityID", static_cast<uint32_t>(entity));
            UIRenderer::DrawQuadRotated(rect.computedScreenRect, rect.computedRotation, glm::vec4(1.0f));
            UIRenderer::End();
        }
    }

    if (scene->HasComponent<SceneTreeComponent>(entity))
    {
        const auto& children = scene->GetComponent<SceneTreeComponent>(entity).children;
        for (auto child : children)
        {
            RenderEntityIDRecursively(scene, child, shader);
        }
    }
}

}
