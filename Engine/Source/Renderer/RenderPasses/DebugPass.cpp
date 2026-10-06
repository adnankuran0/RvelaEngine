#include "rvelapch.h"
#include "DebugPass.h"
#include "Renderer/DebugRenderer.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Camera.h"
#include "Scene/Scene.h"
#include <algorithm>
#include <cmath>

using namespace rv;

static float GetGridSpacing(float cameraHeight)
{
    const float targetSpacing = std::max(1.0f, std::abs(cameraHeight) * 0.1f);
    const float decade = std::pow(10.0f, std::floor(std::log10(targetSpacing)));
    const float normalized = targetSpacing / decade;
    return (normalized < 1.5f ? 1.0f : normalized < 3.5f ? 2.0f : normalized < 7.5f ? 5.0f : 10.0f) * decade;
}

static float GridFade(float distance, float fadeStart, float fadeEnd)
{
    const float t = std::clamp((distance - fadeStart) / (fadeEnd - fadeStart), 0.0f, 1.0f);
    return 1.0f - t * t * (3.0f - 2.0f * t);
}

static void DrawFadedGridLine(bool alongX, float fixedCoordinate, const glm::vec3& cameraPosition,
    float gridY, float fadeStart, float fadeEnd, const glm::vec4& color)
{
    const float perpendicularDistance = alongX
        ? std::abs(fixedCoordinate - cameraPosition.z)
        : std::abs(fixedCoordinate - cameraPosition.x);
    if (perpendicularDistance >= fadeEnd)
        return;

    const float nearDistance = perpendicularDistance < fadeStart
        ? std::sqrt(fadeStart * fadeStart - perpendicularDistance * perpendicularDistance)
        : 0.0f;
    const float farDistance = std::sqrt(fadeEnd * fadeEnd - perpendicularDistance * perpendicularDistance);
    const glm::vec3 center = alongX
        ? glm::vec3(cameraPosition.x, gridY, fixedCoordinate)
        : glm::vec3(fixedCoordinate, gridY, cameraPosition.z);

    for (float direction : { -1.0f, 1.0f })
    {
        const glm::vec3 nearPoint = alongX
            ? center + glm::vec3(direction * nearDistance, 0.0f, 0.0f)
            : center + glm::vec3(0.0f, 0.0f, direction * nearDistance);
        const glm::vec3 farPoint = alongX
            ? center + glm::vec3(direction * farDistance, 0.0f, 0.0f)
            : center + glm::vec3(0.0f, 0.0f, direction * farDistance);

        if (nearDistance > 0.0f)
        {
            glm::vec4 nearColor = color;
            nearColor.a *= GridFade(perpendicularDistance, fadeStart, fadeEnd);
            DebugRenderer::Get().DrawLine(center, nearColor, nearPoint, nearColor);
        }

        glm::vec4 fadeStartColor = color;
        const float fadeStartRadius = std::sqrt(perpendicularDistance * perpendicularDistance + nearDistance * nearDistance);
        fadeStartColor.a *= GridFade(fadeStartRadius, fadeStart, fadeEnd);
        glm::vec4 fadeEndColor = color;
        fadeEndColor.a = 0.0f;
        DebugRenderer::Get().DrawLine(nearPoint, fadeStartColor, farPoint, fadeEndColor);
    }
}

static void DrawInfiniteGrid(const RenderContext& ctx)
{
    const glm::vec3 cameraPosition = ctx.camera->Position;
    const float spacing = GetGridSpacing(cameraPosition.y);
    const float fadeStart = std::max(12.0f, std::abs(cameraPosition.y) * 1.2f);
    const float fadeEnd = std::max(90.0f, std::abs(cameraPosition.y) * 9.0f);
    const float gridY = 0.001f;
    const int firstX = static_cast<int>(std::floor((cameraPosition.x - fadeEnd) / spacing));
    const int lastX = static_cast<int>(std::ceil((cameraPosition.x + fadeEnd) / spacing));
    const int firstZ = static_cast<int>(std::floor((cameraPosition.z - fadeEnd) / spacing));
    const int lastZ = static_cast<int>(std::ceil((cameraPosition.z + fadeEnd) / spacing));

    for (int i = firstX; i <= lastX; ++i)
    {
        const float x = static_cast<float>(i) * spacing;
        const glm::vec4 color = i == 0
            ? glm::vec4(0.58f, 0.28f, 0.28f, 0.36f)
            : (i % 10 == 0
                ? glm::vec4(0.48f, 0.52f, 0.60f, 0.30f)
                : glm::vec4(0.36f, 0.40f, 0.48f, 0.16f));
        DrawFadedGridLine(false, x, cameraPosition, gridY, fadeStart, fadeEnd, color);
    }

    for (int i = firstZ; i <= lastZ; ++i)
    {
        const float z = static_cast<float>(i) * spacing;
        const glm::vec4 color = i == 0
            ? glm::vec4(0.28f, 0.40f, 0.58f, 0.36f)
            : (i % 10 == 0
                ? glm::vec4(0.48f, 0.52f, 0.60f, 0.30f)
                : glm::vec4(0.36f, 0.40f, 0.48f, 0.16f));
        DrawFadedGridLine(true, z, cameraPosition, gridY, fadeStart, fadeEnd, color);
    }
}

void DebugPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
	DebugRenderer::Get().Init();
}

void DebugPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (DebugRenderer::Get().GetSettings().drawBoundingBoxes)
        DrawAABBs(ctx);

    if (ctx.scene->GetState() == SceneState::EDIT && DebugRenderer::Get().GetSettings().drawInfiniteGrid)
        DrawInfiniteGrid(ctx);

    GLuint finalFramebuffer = frame.registry.Get("FinalFramebuffer")->id;
    GLuint depthTexture = frame.registry.Get("DepthTexture")->id;

    glBindFramebuffer(GL_FRAMEBUFFER, finalFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, depthTexture, 0);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); 

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLint previousBlendSrcRGB = GL_ONE;
    GLint previousBlendDstRGB = GL_ZERO;
    GLint previousBlendSrcAlpha = GL_ONE;
    GLint previousBlendDstAlpha = GL_ZERO;
    glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDstAlpha);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 mvp = ctx.camera->GetProjectionMatrix() * ctx.camera->GetViewMatrix();
    DebugRenderer::Get().EndFrame(mvp);

    glBlendFuncSeparate(previousBlendSrcRGB, previousBlendDstRGB, previousBlendSrcAlpha, previousBlendDstAlpha);
    if (!blendWasEnabled)
        glDisable(GL_BLEND);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, 0, 0);

    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DebugPass::DrawAABBs(const RenderContext& ctx)
{
    auto& reg = ctx.scene->GetRegistry();

    auto meshView = reg.view<MeshRendererComponent>();
    for (auto e : meshView)
    {
        if (!ctx.scene->IsEntityActive(e))
            continue;

        auto& comp = reg.get<MeshRendererComponent>(e);
        DebugRenderer::Get().DrawBox(comp.worldAABB.min, comp.worldAABB.max, { 0.0f, 1.0f, 1.0f, 1.0f });
    }

    auto skelView = reg.view<SkeletalMeshRendererComponent>();
    for (auto e : skelView)
    {
        if (!ctx.scene->IsEntityActive(e))
            continue;

        auto& comp = reg.get<SkeletalMeshRendererComponent>(e);
        DebugRenderer::Get().DrawBox(comp.worldAABB.min, comp.worldAABB.max, { 0.0f, 1.0f, 1.0f, 1.0f });
    }
}
