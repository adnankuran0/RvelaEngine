#include "rvelapch.h"
#include "ShadowPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"

using namespace rv;

void ShadowPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    FramebufferDesc desc;
    desc.width = SHADOW_WIDTH;
    desc.height = SHADOW_HEIGHT;
    desc.hasDepth = true;
    desc.depthAttachment = {
        FramebufferTextureFormat::Depth16,
        FramebufferFilterMode::Linear,
        FramebufferWrapMode::ClampToBorder,
        true
    };

    m_DirectionalShadowFramebuffer = Framebuffer(desc);

    InitPointShadowMap();

    frame.registry.Register("DirectionalShadowMap", { RenderResourceType::Texture, m_DirectionalShadowFramebuffer.GetDepthAttachment() });
    frame.registry.Register("PointShadowMap", { RenderResourceType::Texture, o_PointShadowMap });
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
    auto& skeletalCommands = frame.skeletalOpaqueCommands;

    if (ctx.directionalLight && ctx.directionalLight->castShadows)
    {
        m_DirectionalShadowFramebuffer.BindViewport();
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 2.0f);

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

        if (!commands.empty())
        {
            Shader& shadowShader = ShaderManager::Get("DirectionalShadow");
            shadowShader.use();
            shadowShader.setInt("billboardMode", 0);

            for (auto& command : commands) {
                if (!ctx.camera->Intersects(ctx.directionalLight->lightSpace, command.mesh->worldAABB)) continue;
                if (!command.mesh->IsCastShadow()) continue;

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
            Shader& skeletalShadowShader = ShaderManager::Get("DirectionalShadow_Skeletal");
            skeletalShadowShader.use();

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

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

void ShadowPass::RenderPointShadowMap(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.opaqueCommands;
    auto& skeletalCommands = frame.skeletalOpaqueCommands;

    glBindFramebuffer(GL_FRAMEBUFFER, pointFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_PointShadowMap, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);

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

        if (!commands.empty())
        {
            pointShadowShader.use();
            pointShadowShader.setFloat("far_plane", far_plane);
            pointShadowShader.setVec3("lightPos", lightPos);
            pointShadowShader.setInt("baseLayer", light.shadowIndex * 6);

            for (auto& command : commands)
            {
                if (!command.mesh->IsCastShadow()) continue;

                auto& material = command.material;
                CullMode mode = material->GetCullMode();
                if (mode == CullMode::Disabled) glDisable(GL_CULL_FACE);
                else { glEnable(GL_CULL_FACE); glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT); }

                pointShadowShader.setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
                bool useAlb = BindMaterial(pointShadowShader, material);

                pointShadowShader.setMat4("model", command.transform->GetWorldMatrix());
                command.mesh->VAO.Bind();

                for (unsigned int face = 0; face < 6; ++face)
                {
                    if (!ctx.camera->Intersects(shadowTransforms[face], command.mesh->worldAABB)) continue;
                    pointShadowShader.setMat4("shadowMatrix", shadowTransforms[face]);
                    pointShadowShader.setInt("currentFace", face);
                    glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);
                }

                if (useAlb) glBindSampler(0, 0);
            }
        }

        if (!skeletalCommands.empty())
        {
            skeletalPointShadowShader.use();
            skeletalPointShadowShader.setFloat("far_plane", far_plane);
            skeletalPointShadowShader.setVec3("lightPos", lightPos);
            skeletalPointShadowShader.setInt("baseLayer", light.shadowIndex * 6);

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

                for (unsigned int face = 0; face < 6; ++face)
                {
                    skeletalPointShadowShader.setMat4("shadowMatrix", shadowTransforms[face]);
                    skeletalPointShadowShader.setInt("currentFace", face);
                    glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);
                }

                if (useAlb) glBindSampler(0, 0);
            }
        }
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
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
    glDeleteTextures(1, &o_PointShadowMap);
    glDeleteFramebuffers(1, &pointFBO);
}

