#include "rvelapch.h"
#include "ShadowPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderTypes.h"
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
    glGenFramebuffers(1, &dirShadowFBO);

    glGenTextures(1, &o_DirectionalShadowMap);
    glBindTexture(GL_TEXTURE_2D_ARRAY, o_DirectionalShadowMap);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_DEPTH_COMPONENT32F,
        SHADOW_WIDTH,
        SHADOW_HEIGHT,
        NUM_SHADOW_CASCADES,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, dirShadowFBO);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_DirectionalShadowMap, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Directional shadow framebuffer not complete! Status: {:#x}", status);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowPass::InitPointShadowMap()
{
    glGenFramebuffers(1, &pointFBO);

    glGenTextures(1, &o_PointShadowMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, o_PointShadowMap);
    glTexImage3D(
        GL_TEXTURE_CUBE_MAP_ARRAY,
        0,
        GL_DEPTH_COMPONENT32F,
        POINT_SHADOW_WIDTH,
        POINT_SHADOW_HEIGHT,
        6 * MaxPointLights,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, pointFBO);
    for (int layer = 0; layer < 6 * MaxPointLights; ++layer) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_PointShadowMap, 0, layer);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Point shadow framebuffer layer {} not complete!", layer);
        }
    }

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowPass::RenderDirectionalShadowMap(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.opaqueCommands;
    auto& skeletalCommands = frame.skeletalOpaqueCommands;

    if (!ctx.directionalLight || !ctx.directionalLight->castShadows) return;
    if (commands.empty() && skeletalCommands.empty()) return;

    glBindFramebuffer(GL_FRAMEBUFFER, dirShadowFBO);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    bool reverseCull = ctx.directionalLight->reverseCullFace;

    auto ApplyCull = [&](CullMode mode) {
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
    };

    auto BindMaterial = [](Shader& shader, MaterialComponent* material) {
        shader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
        shader.setFloat("alphaCutoff", material->GetAlphaCutoff());
        shader.setVec4("albedoColor", material->GetAlbedoColor());
        shader.setVec2("UVScale", material->GetUVScale());
        shader.setVec2("UVOffset", material->GetUVOffset());

        bool useAlb = material->IsUsingAlbedoMap() && material->GetAlbedoTexture();
        shader.setBool("useAlbedoMap", useAlb);
        if (useAlb) {
            shader.setInt("albedoMap", 0);
            TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
            material->GetSampler().Bind(0);
        }
        else {
            glBindSampler(0, 0);
            glBindTextureUnit(0, 0);
        }
        return useAlb;
    };

    Shader& shadowShader = ShaderManager::Get("DirectionalShadow");
    Shader& skeletalShadowShader = ShaderManager::Get("DirectionalShadow_Skeletal");

    for (int cascade = 0; cascade < NUM_SHADOW_CASCADES; ++cascade)
    {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_DirectionalShadowMap, 0, cascade);
        glClear(GL_DEPTH_BUFFER_BIT);

        const glm::mat4& cascadeMatrix = ctx.directionalLight->cascadeMatrices[cascade];

        if (!commands.empty())
        {
            shadowShader.use();
            shadowShader.setMat4("lightSpaceMatrix", cascadeMatrix);
            shadowShader.setInt("billboardMode", 0);

            for (auto& command : commands) {
                if (!command.mesh || command.mesh->indexCount == 0) continue;
                if (!command.mesh->IsCastShadow()) continue;
                if (command.mesh->worldAABB.IsValid() && !ctx.camera->Intersects(cascadeMatrix, command.mesh->worldAABB)) continue;

                auto& material = command.material;
                ApplyCull(material->GetCullMode());
                shadowShader.setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));

                bool useAlb = BindMaterial(shadowShader, material);

                shadowShader.setMat4("model", command.transform->GetWorldMatrix());
                command.mesh->VAO.Bind();
                glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                if (useAlb) glBindSampler(0, 0);
            }
        }

        if (!skeletalCommands.empty())
        {
            skeletalShadowShader.use();
            skeletalShadowShader.setMat4("lightSpaceMatrix", cascadeMatrix);

            GLint boneLoc = glGetUniformLocation(skeletalShadowShader.ID, "u_BoneMatrices");

            for (auto& command : skeletalCommands) {
                if (!command.mesh || command.mesh->indexCount == 0) continue;
                if (!command.mesh->IsCastShadow()) continue;

                auto& material = command.material;
                ApplyCull(material->GetCullMode());

                bool useAlb = BindMaterial(skeletalShadowShader, material);

                skeletalShadowShader.setMat4("model", command.transform->GetWorldMatrix());

                if (boneLoc != -1 && command.skeleton && !command.skeleton->skinningPalette.empty())
                {
                    glUniformMatrix4fv(
                        boneLoc,
                        static_cast<GLsizei>(command.skeleton->skinningPalette.size()),
                        GL_FALSE,
                        glm::value_ptr(command.skeleton->skinningPalette[0])
                    );
                }

                command.mesh->VAO.Bind();
                glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                if (useAlb) glBindSampler(0, 0);
            }
        }
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowPass::RenderPointShadowMap(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.opaqueCommands;
    auto& skeletalCommands = frame.skeletalOpaqueCommands;

    glBindFramebuffer(GL_FRAMEBUFFER, pointFBO);
    glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    Shader& pointShadowShader = ShaderManager::Get("PointShadow");
    Shader& skeletalPointShadowShader = ShaderManager::Get("PointShadow_Skeletal");

    auto BindMaterial = [](Shader& shader, MaterialComponent* material) {
        shader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
        shader.setFloat("alphaCutoff", material->GetAlphaCutoff());
        shader.setVec4("albedoColor", material->GetAlbedoColor());
        shader.setVec2("UVScale", material->GetUVScale());
        shader.setVec2("UVOffset", material->GetUVOffset());

        bool useAlb = material->IsUsingAlbedoMap() && material->GetAlbedoTexture();
        shader.setBool("useAlbedoMap", useAlb);
        if (useAlb) {
            shader.setInt("albedoMap", 0);
            TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
            material->GetSampler().Bind(0);
        }
        else {
            glBindSampler(0, 0);
            glBindTextureUnit(0, 0);
        }
        return useAlb;
    };

    for (auto& light : ctx.pointLights)
    {
        if (!light.castShadows) continue;

        glm::vec3 lightPos = light.position;

        float near_plane = 0.05f;
        float far_plane = light.radius;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)POINT_SHADOW_WIDTH / (float)POINT_SHADOW_HEIGHT, near_plane, far_plane);
        std::array<glm::mat4, 6> shadowTransforms;
        shadowTransforms[0] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadowTransforms[1] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadowTransforms[2] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        shadowTransforms[3] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        shadowTransforms[4] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadowTransforms[5] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        int baseLayer = light.shadowIndex * 6;

        for (unsigned int face = 0; face < 6; ++face)
        {
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_PointShadowMap, 0, baseLayer + face);
            glClear(GL_DEPTH_BUFFER_BIT);

            if (!commands.empty())
            {
                pointShadowShader.use();
                pointShadowShader.setFloat("far_plane", far_plane);
                pointShadowShader.setVec3("lightPos", lightPos);
                pointShadowShader.setInt("baseLayer", baseLayer);
                pointShadowShader.setInt("currentFace", face);
                pointShadowShader.setMat4("shadowMatrix", shadowTransforms[face]);

                for (auto& command : commands)
                {
                    if (!command.mesh || command.mesh->indexCount == 0) continue;
                    if (!command.mesh->IsCastShadow()) continue;
                    if (command.mesh->worldAABB.IsValid() && !ctx.camera->Intersects(shadowTransforms[face], command.mesh->worldAABB)) continue;

                    auto& material = command.material;
                    CullMode mode = material->GetCullMode();
                    if (mode == CullMode::Disabled) glDisable(GL_CULL_FACE);
                    else { glEnable(GL_CULL_FACE); glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT); }

                    pointShadowShader.setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
                    bool useAlb = BindMaterial(pointShadowShader, material);

                    pointShadowShader.setMat4("model", command.transform->GetWorldMatrix());
                    command.mesh->VAO.Bind();
                    glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                    if (useAlb) glBindSampler(0, 0);
                }
            }

            if (!skeletalCommands.empty())
            {
                skeletalPointShadowShader.use();
                skeletalPointShadowShader.setFloat("far_plane", far_plane);
                skeletalPointShadowShader.setVec3("lightPos", lightPos);
                skeletalPointShadowShader.setInt("baseLayer", baseLayer);
                skeletalPointShadowShader.setInt("currentFace", face);
                skeletalPointShadowShader.setMat4("shadowMatrix", shadowTransforms[face]);

                GLint boneLoc = glGetUniformLocation(skeletalPointShadowShader.ID, "u_BoneMatrices");

                for (auto& command : skeletalCommands)
                {
                    if (!command.mesh || command.mesh->indexCount == 0) continue;
                    if (!command.mesh->IsCastShadow()) continue;

                    auto& material = command.material;
                    CullMode mode = material->GetCullMode();
                    if (mode == CullMode::Disabled) glDisable(GL_CULL_FACE);
                    else { glEnable(GL_CULL_FACE); glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT); }

                    bool useAlb = BindMaterial(skeletalPointShadowShader, material);

                    skeletalPointShadowShader.setMat4("model", command.transform->GetWorldMatrix());

                    if (boneLoc != -1 && command.skeleton && !command.skeleton->skinningPalette.empty())
                    {
                        glUniformMatrix4fv(
                            boneLoc,
                            static_cast<GLsizei>(command.skeleton->skinningPalette.size()),
                            GL_FALSE,
                            glm::value_ptr(command.skeleton->skinningPalette[0])
                        );
                    }

                    command.mesh->VAO.Bind();
                    glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                    if (useAlb) glBindSampler(0, 0);
                }
            }
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
}

void ShadowPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (frame.opaqueCommands.empty() && frame.skeletalOpaqueCommands.empty()) return;

    RenderDirectionalShadowMap(ctx, frame);
    RenderPointShadowMap(ctx, frame);
}

ShadowPass::~ShadowPass()
{
    glDeleteTextures(1, &o_DirectionalShadowMap);
    glDeleteFramebuffers(1, &dirShadowFBO);
    glDeleteTextures(1, &o_PointShadowMap);
    glDeleteFramebuffers(1, &pointFBO);
}
