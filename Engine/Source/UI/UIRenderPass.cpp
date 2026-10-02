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

    s_UIEntityShader->use();

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
        }
        else
        {
            customProjMatrix = glm::ortho(0.0f, viewportWidth, 0.0f, viewportHeight, -100.0f, 100.0f);
        }

        UIRenderer::Begin(viewportWidth, viewportHeight, customProjMatrix, s_UIEntityShader);
        RenderEntityIDRecursively(scene, canvasEntity, s_UIEntityShader);
    }
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

    const auto& rect = scene->GetComponent<RectTransformComponent>(selectedEntity);
    UIRenderer::Begin(viewportWidth, viewportHeight, customProjMatrix);
    UIRenderer::DrawQuadRotated(rect.computedScreenRect, rect.computedRotation, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    UIRenderer::End();
}

void UIRenderPass::RenderEntityRecursively(Scene* scene, entt::entity entity)
{
    if (!scene->HasComponent<RectTransformComponent>(entity))
    {
        if (scene->HasComponent<UITextComponent>(entity) ||
            scene->HasComponent<UIImageComponent>(entity) ||
            scene->HasComponent<UIButtonComponent>(entity) ||
            scene->HasComponent<UISliderComponent>(entity) ||
            scene->HasComponent<UIProgressBarComponent>(entity))
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
            if (scene->HasComponent<UIImageComponent>(entity)) {
                const auto& img = scene->GetComponent<UIImageComponent>(entity);
                if (img.textureUUID.IsValid()) {
                    auto texAsset = AssetManager::Get().GetAsset<TextureAsset>(img.textureUUID);
                    if (texAsset) texId = TextureCache::Get().GetOrCreate(texAsset).GetID();
                }
            }

            UIRenderer::DrawQuadRotated(screenRect, rect.computedRotation, currentColor, texId, button.cornerRadius);

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

        if (scene->HasComponent<UISliderComponent>(entity))
        {
            const auto& slider = scene->GetComponent<UISliderComponent>(entity);
            // Background
            UIRenderer::DrawQuadRotated(screenRect, rect.computedRotation, slider.backgroundColor, 0, 4.0f, rectCenter);

            float width = screenRect.z - screenRect.x;
            float t = std::clamp((slider.value - slider.minValue) / (slider.maxValue - slider.minValue), 0.0f, 1.0f);
            
            // Fill
            glm::vec4 fillRect = glm::vec4(screenRect.x, screenRect.y, screenRect.x + width * t, screenRect.w);
            UIRenderer::DrawQuadRotated(fillRect, rect.computedRotation, slider.fillColor, 0, 4.0f, rectCenter);

            // Handle
            float handleW = 12.0f;
            float handleX = screenRect.x + width * t - handleW * 0.5f;
            glm::vec4 handleRect = glm::vec4(handleX, screenRect.y - 2.0f, handleX + handleW, screenRect.w + 2.0f);
            UIRenderer::DrawQuadRotated(handleRect, rect.computedRotation, slider.handleColor, 0, 6.0f, rectCenter);
        }

        if (scene->HasComponent<UIProgressBarComponent>(entity))
        {
            const auto& pb = scene->GetComponent<UIProgressBarComponent>(entity);
            // Background
            UIRenderer::DrawQuadRotated(screenRect, rect.computedRotation, pb.backgroundColor, 0, 4.0f, rectCenter);

            float width = screenRect.z - screenRect.x;
            float t = std::clamp(pb.value, 0.0f, 1.0f);

            // Fill
            glm::vec4 fillRect = glm::vec4(screenRect.x, screenRect.y, screenRect.x + width * t, screenRect.w);
            UIRenderer::DrawQuadRotated(fillRect, rect.computedRotation, pb.fillColor, 0, 4.0f, rectCenter);
        }

        if (scene->HasComponent<UICheckboxComponent>(entity))
        {
            const auto& cb = scene->GetComponent<UICheckboxComponent>(entity);
            float boxSize = (cb.boxSize > 0.0f) ? cb.boxSize : 20.0f;

            float boxY = rectCenter.y - boxSize * 0.5f;
            glm::vec4 boxRect = glm::vec4(screenRect.x, boxY, screenRect.x + boxSize, boxY + boxSize);

            // Draw Box
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

            float maxW = textComp.wordWrap ? (screenRect.z - screenRect.x) : 0.0f;
            UIRenderer::DrawTextString(textComp.text, pos, textComp.fontSize, textComp.color, textComp.alignment, rect.computedRotation, rectCenter, nullptr, textComp.isBold, textComp.isOutline, textComp.outlineSize, textComp.outlineColor, maxW, textComp.lineSpacing, textComp.characterSpacing);
        }
    }

    if (scene->HasComponent<SceneTreeComponent>(entity))
    {
        const auto& children = scene->GetComponent<SceneTreeComponent>(entity).children;
        for (auto child : children)
        {
            RenderEntityRecursively(scene, child);
        }
    }
}

void UIRenderPass::RenderEntityIDRecursively(Scene* scene, entt::entity entity, std::shared_ptr<Shader> shader)
{
    if (!scene->HasComponent<RectTransformComponent>(entity))
    {
        if (scene->HasComponent<UITextComponent>(entity) ||
            scene->HasComponent<UIImageComponent>(entity) ||
            scene->HasComponent<UIButtonComponent>(entity) ||
            scene->HasComponent<UISliderComponent>(entity) ||
            scene->HasComponent<UIProgressBarComponent>(entity) ||
            scene->HasComponent<UICheckboxComponent>(entity))
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
                     scene->HasComponent<UICheckboxComponent>(entity);
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
