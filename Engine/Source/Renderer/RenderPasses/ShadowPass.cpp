#include "rvelapch.h"
#include "ShadowPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"

using namespace rv;

void ShadowPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    InitDirectionalShadowMap();
    InitPointShadowMap();

    frame.registry.Register("DirectionalShadowMap", { RenderResourceType::Texture, o_DirectionalShadowMap });
    frame.registry.Register("PointShadowMap", { RenderResourceType::Texture, o_PointShadowMap });
}

void ShadowPass::InitDirectionalShadowMap()
{
    glGenFramebuffers(1, &fbo);

    glGenTextures(1, &o_DirectionalShadowMap);
    glBindTexture(GL_TEXTURE_2D, o_DirectionalShadowMap);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT16,
        SHADOW_WIDTH,
        SHADOW_HEIGHT,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, o_DirectionalShadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        LOG_ERROR("Shadow framebuffer not complete!");
}

void ShadowPass::InitPointShadowMap()
{
    glGenFramebuffers(1, &pointFBO);

    glGenTextures(1, &o_PointShadowMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, o_PointShadowMap);
    glTexImage3D(
        GL_TEXTURE_CUBE_MAP_ARRAY,
        0,
        GL_DEPTH_COMPONENT16,
        POINT_SHADOW_WIDTH,
        POINT_SHADOW_HEIGHT,
        6 * 20,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, pointFBO);
    for (int layer = 0; layer < 6 * 20; ++layer) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_PointShadowMap, 0, layer);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Point shadow framebuffer layer {} not complete!", layer);
        }
    }

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
}

void ShadowPass::RenderDirectionalShadowMap(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.opaqueCommands;

    if (ctx.directionalLight && ctx.directionalLight->castShadows)
    {
        Shader& shadowShader = ShaderManager::Get("DirectionalShadow");
        shadowShader.use();
        shadowShader.setMat4("cameraView", ctx.camera->GetViewMatrix());
        shadowShader.setVec3("camPos", ctx.camera->Position);
        shadowShader.setMat4("lightSpaceMatrix", ctx.directionalLight->lightSpace);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 2.0f);

        bool reverseCull = ctx.directionalLight->reverseCullFace;

        for (auto& command : commands) {
            if (!ctx.camera->Intersects(ctx.directionalLight->lightSpace, command.mesh->worldAABB)) continue;
            if (!command.mesh->IsCastShadow()) continue;

            auto& material = command.material;
            CullMode mode = material->GetCullMode();
            if (mode == CullMode::Disabled) {
                glDisable(GL_CULL_FACE);
            }
            else {
                glEnable(GL_CULL_FACE);
                if (reverseCull)
                    glCullFace(mode == CullMode::Back ? GL_FRONT : GL_BACK);
                else
                    glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
            }

            shadowShader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
            shadowShader.setFloat("alphaCutoff", material->GetAlphaCutoff());
            shadowShader.setVec4("albedoColor", material->GetAlbedoColor());
            shadowShader.setVec2("UVScale", material->GetUVScale());
            shadowShader.setVec2("UVOffset", material->GetUVOffset());
            shadowShader.setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
            

            bool useAlb = material->IsUsingAlbedoMap() && material->GetAlbedoTexture();
            shadowShader.setBool("useAlbedoMap", useAlb);
            if (useAlb) {
                shadowShader.setInt("albedoMap", 0);
                TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
                material->GetSampler().Bind(0);
            }
            else {
                glBindSampler(0, 0);
                glBindTextureUnit(0, 0);
            }

            shadowShader.setMat4("model", command.transform->GetWorldMatrix());
            command.mesh->VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

            if (useAlb) glBindSampler(0, 0);
        }

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

void ShadowPass::RenderPointShadowMap(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.opaqueCommands;

    Shader& pointShadowShader = ShaderManager::Get("PointShadow");
    pointShadowShader.use();
    pointShadowShader.setMat4("cameraView", ctx.camera->GetViewMatrix());
    pointShadowShader.setVec3("camPos", ctx.camera->Position);

    glBindFramebuffer(GL_FRAMEBUFFER, pointFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_PointShadowMap, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);



    for (auto& light : ctx.pointLights)
    {
        if (!light.castShadows) continue;

        glm::vec3 lightPos = light.position;

        float near_plane = 0.1f;
        float far_plane = light.radius;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)POINT_SHADOW_WIDTH / (float)POINT_SHADOW_HEIGHT, near_plane, far_plane);
        std::array<glm::mat4, 6> shadowTransforms;
        shadowTransforms[0] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadowTransforms[1] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadowTransforms[2] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        shadowTransforms[3] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        shadowTransforms[4] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadowTransforms[5] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        pointShadowShader.setFloat("far_plane", far_plane);
        pointShadowShader.setVec3("lightPos", lightPos);
        pointShadowShader.setInt("baseLayer", light.shadowIndex * 6);

        for (auto& command : commands)
        {
            if (!command.mesh->IsCastShadow()) continue;

            auto& material = command.material;
            CullMode mode = material->GetCullMode();
            if (mode == CullMode::Disabled) {
                glDisable(GL_CULL_FACE);
            }
            else {
                glEnable(GL_CULL_FACE);
                glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
            }

            pointShadowShader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
            pointShadowShader.setFloat("alphaCutoff", material->GetAlphaCutoff());
            pointShadowShader.setVec4("albedoColor", material->GetAlbedoColor());
            pointShadowShader.setVec2("UVScale", material->GetUVScale());
            pointShadowShader.setVec2("UVOffset", material->GetUVOffset());
            pointShadowShader.setInt("billboardMode", static_cast<int>(command.material->GetBillboardMode()));

            bool useAlb = material->IsUsingAlbedoMap() && material->GetAlbedoTexture();
            pointShadowShader.setBool("useAlbedoMap", useAlb);
            if (useAlb) {
                pointShadowShader.setInt("albedoMap", 0);
                TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
                material->GetSampler().Bind(0);
            }
            else {
                glBindSampler(0, 0);
                glBindTextureUnit(0, 0);
            }

            pointShadowShader.setMat4("model", command.transform->GetWorldMatrix());
            command.mesh->VAO.Bind();

            for (unsigned int face = 0; face < 6; ++face)
            {
                if (!ctx.camera->Intersects(shadowTransforms[face], command.mesh->worldAABB)) continue;

                pointShadowShader.setMat4("shadowMatrix", shadowTransforms[face]);
                pointShadowShader.setInt("currentFace", face);

                glDrawElements(
                    GL_TRIANGLES,
                    command.mesh->indexCount,
                    GL_UNSIGNED_INT,
                    0
                );
            }

            if (useAlb) glBindSampler(0, 0);
        }
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
}

ShadowPass::~ShadowPass()
{
}

void ShadowPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (frame.opaqueCommands.empty()) return;

    auto& resourceRegistry = frame.registry;

    RenderDirectionalShadowMap(ctx, frame);
    RenderPointShadowMap(ctx, frame);

    resourceRegistry.Register("DirectionalShadowMap", { RenderResourceType::Texture, o_DirectionalShadowMap });
    resourceRegistry.Register("PointShadowMap", { RenderResourceType::Texture, o_PointShadowMap });
}