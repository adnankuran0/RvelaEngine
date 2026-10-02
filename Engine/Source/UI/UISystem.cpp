#include "rvelapch.h"
#include "UISystem.h"
#include "UIRenderer.h"
#include "UILayout.h"
#include "UIInteraction.h"
#include "UIRenderPass.h"

namespace rv {

void UISystem::Init()
{
    UIRenderer::Init();
}

void UISystem::Shutdown()
{
    UIRenderer::Shutdown();
}

void UISystem::Update(Scene* scene, float dt, const glm::vec2& viewportSize, const glm::vec2& mousePos, bool mousePressed, bool mouseHeld, bool mouseReleased)
{
    if (!scene) return;

    UILayout::UpdateCanvasLayouts(scene, viewportSize.x, viewportSize.y);

    UIInteraction::UpdateInteractions(scene, dt, viewportSize, mousePos, mousePressed, mouseHeld, mouseReleased);
}

void UISystem::Render(Scene* scene, float viewportWidth, float viewportHeight)
{
    UIRenderPass::Render(scene, viewportWidth, viewportHeight);
}

void UISystem::RenderEntityIDs(Scene* scene, float viewportWidth, float viewportHeight)
{
    UIRenderPass::RenderEntityIDs(scene, viewportWidth, viewportHeight);
}

void UISystem::RenderSelectionMask(Scene* scene, entt::entity selectedEntity, float viewportWidth, float viewportHeight)
{
    UIRenderPass::RenderSelectionMask(scene, selectedEntity, viewportWidth, viewportHeight);
}

void UISystem::UpdateLayoutRecursively(Scene* scene, entt::entity entity, const glm::vec2& parentPos, const glm::vec2& parentSize, const glm::vec2& parentCenter, float parentRotation)
{
    UILayout::UpdateLayoutRecursively(scene, entity, parentPos, parentSize, parentCenter, parentRotation);
}

void UISystem::DispatchLuaCallback(Scene* scene, entt::entity entity, const std::string& callbackName, float param)
{
    UIInteraction::DispatchLuaCallback(scene, entity, callbackName, param);
}

}
