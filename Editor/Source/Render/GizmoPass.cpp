#include "rvelapch.h"
#include "GizmoPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/Camera.h"
#include "Renderer/DebugRenderer.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Renderer/ShaderManager.h"

using namespace rv;

void GizmoPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    m_CameraFrustumProj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1.0f);

    m_AudioEmitterIcon.Init();
    m_AudioEmitterIcon.GenerateFromImage(EDITOR_PATH("Icons\\audioemitter.png").GetAbsoluteStr());
    m_CameraIcon.Init();
    m_CameraIcon.GenerateFromImage(EDITOR_PATH("Icons\\camera.png").GetAbsoluteStr());
    m_DirectionalLightIcon.Init();
    m_DirectionalLightIcon.GenerateFromImage(EDITOR_PATH("Icons\\directionallight.png").GetAbsoluteStr());
    m_PointLightIcon.Init();
    m_PointLightIcon.GenerateFromImage(EDITOR_PATH("Icons\\pointlight.png").GetAbsoluteStr());
    m_SpotLightIcon.Init();
    m_SpotLightIcon.GenerateFromImage(EDITOR_PATH("Icons\\spotlight.png").GetAbsoluteStr());
    m_ParticleEmitterIcon.Init();
    m_ParticleEmitterIcon.GenerateFromImage(EDITOR_PATH("Icons\\particleemitter.png").GetAbsoluteStr());

    static constexpr float quadVertices[] =
    {
        -1.0f,  1.0f, 0.0f,    0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,    0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,    1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f,    0.0f, 1.0f,
         1.0f, -1.0f, 0.0f,    1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,    1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);
    glBindVertexArray(m_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

void GizmoPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (ctx.scene->GetState() != SceneState::EDIT)
        return;

    DrawCameraFrustum(ctx, frame);
    DrawDirectionalLightArrow(ctx, frame);

    DrawEditorIconsVisual(ctx, frame);
    DrawEditorIconsPicking(ctx, frame);
}

GizmoInfo GizmoPass::GetEntityPrimaryIcon(entt::registry& reg, entt::entity e)
{
    if (reg.all_of<CameraComponent>(e)) return { m_CameraIcon.GetID(), GizmoPriority::Camera };
    if (reg.all_of<ParticleEmitterComponent>(e)) return { m_ParticleEmitterIcon.GetID(), GizmoPriority::Particle };
    if (reg.all_of<DirectionalLightComponent>(e)) return { m_DirectionalLightIcon.GetID(), GizmoPriority::Light };
    if (reg.all_of<PointLightComponent>(e)) return { m_PointLightIcon.GetID(), GizmoPriority::Light };
    if (reg.all_of<SpotLightComponent>(e)) return { m_SpotLightIcon.GetID(), GizmoPriority::Light };
    if (reg.all_of<AudioEmitterComponent>(e)) return { m_AudioEmitterIcon.GetID(), GizmoPriority::Audio };

    return { 0, GizmoPriority::None };
}

glm::mat4 GizmoPass::CalculateBillboardTransform(const glm::vec3& worldPos, const glm::vec3& camPos, const glm::mat4& viewMat)
{
    float dist = glm::distance(worldPos, camPos);
    float scale = dist * m_GizmoScale;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), worldPos);

    model[0][0] = viewMat[0][0]; model[0][1] = viewMat[1][0]; model[0][2] = viewMat[2][0];
    model[1][0] = viewMat[0][1]; model[1][1] = viewMat[1][1]; model[1][2] = viewMat[2][1];
    model[2][0] = viewMat[0][2]; model[2][1] = viewMat[1][2]; model[2][2] = viewMat[2][2];

    return glm::scale(model, glm::vec3(scale, scale, 1.0f));
}

void GizmoPass::DrawEditorIconsVisual(const RenderContext& ctx, RenderFrame& frame)
{
    auto* finalFboRes = frame.registry.Get("FinalFramebuffer");
    auto* depthTexRes = frame.registry.Get("DepthTexture");
    if (!finalFboRes || !depthTexRes) return;

    glBindFramebuffer(GL_FRAMEBUFFER, finalFboRes->id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexRes->id, 0);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader& shader = ShaderManager::Get("Gizmo");
    shader.use();

    glm::mat4 view = ctx.camera->GetViewMatrix();
    glm::mat4 proj = ctx.camera->GetProjectionMatrix();
    shader.setMat4("u_ViewProj", proj * view);

    glBindVertexArray(m_QuadVAO);

    auto& reg = ctx.scene->GetRegistry();
    entt::entity selectedEntity = ctx.scene->GetSelectedEntity();

    for (auto e : reg.view<TransformComponent>())
    {
        GizmoInfo icon = GetEntityPrimaryIcon(reg, e);
        if (icon.priority == GizmoPriority::None) continue;

        auto& tc = reg.get<TransformComponent>(e);
        glm::mat4 model = CalculateBillboardTransform(tc.GetWorldPosition(), ctx.camera->Position, view);

        shader.setMat4("u_Model", model);

        if (e == selectedEntity)
            shader.setVec4("u_TintColor", glm::vec4(1.0f, 0.7f, 0.3f, 1.0f));
        else
            shader.setVec4("u_TintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        glBindTextureUnit(0, icon.textureID);
        shader.setInt("u_IconTexture", 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glDisable(GL_BLEND);

    glBindTextureUnit(0, 0);

    glDepthMask(GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void GizmoPass::DrawEditorIconsPicking(const RenderContext& ctx, RenderFrame& frame)
{
    auto* entityBufferRes = frame.registry.Get("EntityBuffer");
    if (!entityBufferRes) return;

    glBindFramebuffer(GL_FRAMEBUFFER, entityBufferRes->id);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    glDisable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    Shader& shader = ShaderManager::Get("EntityBuffer");
    shader.use();

    glBindVertexArray(m_QuadVAO);
    auto& reg = ctx.scene->GetRegistry();

    glm::mat4 view = ctx.camera->GetViewMatrix();

    for (auto e : reg.view<TransformComponent>())
    {
        GizmoInfo icon = GetEntityPrimaryIcon(reg, e);
        if (icon.priority == GizmoPriority::None) continue;

        auto& tc = reg.get<TransformComponent>(e);

        glm::mat4 model = CalculateBillboardTransform(tc.GetWorldPosition(), ctx.camera->Position, view);

        shader.setMat4("model", model);
        shader.setUInt("u_EntityID", static_cast<uint32_t>(e));

        shader.setInt("billboardMode", 0);

        shader.setInt("transparencyMode", 0);

        shader.setBool("useAlbedoMap", true);
        shader.setVec4("albedoColor", glm::vec4(1.0f));
        shader.setVec2("UVScale", glm::vec2(1.0f));
        shader.setVec2("UVOffset", glm::vec2(0.0f));

        glBindTextureUnit(0, icon.textureID);
        shader.setInt("albedoMap", 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindTextureUnit(0, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void GizmoPass::DrawCameraFrustum(const RenderContext& ctx, RenderFrame& frame)
{
    auto* finalFboRes = frame.registry.Get("FinalFramebuffer");
    auto* depthTexRes = frame.registry.Get("DepthTexture");
    if (!finalFboRes || !depthTexRes) return;

    glBindFramebuffer(GL_FRAMEBUFFER, finalFboRes->id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexRes->id, 0);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    DebugRenderer::Get().BeginFrame();

    auto& registry = ctx.scene->GetRegistry();
    for (auto e : registry.view<TransformComponent, CameraComponent>())
    {
        auto& tc = registry.get<TransformComponent>(e);

        glm::mat4 targetCameraView = glm::inverse(tc.GetWorldMatrix());

        glm::mat4 invTargetViewProj = glm::inverse(m_CameraFrustumProj * targetCameraView);

        DebugRenderer::Get().DrawFrustum(invTargetViewProj, glm::vec4(1.0f, 0.75f, 0.1f, 1.0f));
    }

    glm::mat4 editorViewProj = ctx.camera->GetProjectionMatrix() * ctx.camera->GetViewMatrix();
    DebugRenderer::Get().EndFrame(editorViewProj);


    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GizmoPass::DrawDirectionalLightArrow(const RenderContext& ctx, RenderFrame& frame)
{
    auto* finalFboRes = frame.registry.Get("FinalFramebuffer");
    auto* depthTexRes = frame.registry.Get("DepthTexture");
    if (!finalFboRes || !depthTexRes) return;

    glBindFramebuffer(GL_FRAMEBUFFER, finalFboRes->id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexRes->id, 0);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    DebugRenderer::Get().BeginFrame();

    auto& registry = ctx.scene->GetRegistry();
    const glm::vec4 arrowColor = glm::vec4(1.0f, 0.75f, 0.1f, 1.0f);

    for (auto e : registry.view<TransformComponent, DirectionalLightComponent>())
    {
        auto& tc = registry.get<TransformComponent>(e);
        glm::mat4 worldMat = tc.GetWorldMatrix();

        glm::vec3 pos = tc.GetWorldPosition();

        glm::vec3 forward = -glm::normalize(glm::vec3(worldMat[2]));
        glm::vec3 right = glm::normalize(glm::vec3(worldMat[0]));
        glm::vec3 up = glm::normalize(glm::vec3(worldMat[1]));

        float arrowLength = 1.5f;
        float headLength = 0.45f;
        float headRadius = 0.22f;

        glm::vec3 endPos = pos + (forward * arrowLength);
        glm::vec3 basePos = endPos - (forward * headLength);

        DebugRenderer::Get().DrawLine(pos, endPos, arrowColor);

        glm::vec3 pRight = basePos + (right * headRadius);
        glm::vec3 pLeft = basePos - (right * headRadius);
        glm::vec3 pUp = basePos + (up * headRadius);
        glm::vec3 pDown = basePos - (up * headRadius);

        DebugRenderer::Get().DrawLine(endPos, pRight, arrowColor);
        DebugRenderer::Get().DrawLine(endPos, pLeft, arrowColor);
        DebugRenderer::Get().DrawLine(endPos, pUp, arrowColor);
        DebugRenderer::Get().DrawLine(endPos, pDown, arrowColor);

        DebugRenderer::Get().DrawLine(pRight, pUp, arrowColor);
        DebugRenderer::Get().DrawLine(pUp, pLeft, arrowColor);
        DebugRenderer::Get().DrawLine(pLeft, pDown, arrowColor);
        DebugRenderer::Get().DrawLine(pDown, pRight, arrowColor);
    }

    glm::mat4 editorViewProj = ctx.camera->GetProjectionMatrix() * ctx.camera->GetViewMatrix();
    DebugRenderer::Get().EndFrame(editorViewProj);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}