#include "rvelapch.h"
#include "MaterialOutlinePass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/TextureCache.h"
#include <glm/gtc/type_ptr.hpp>

using namespace rv;

void MaterialOutlinePass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    FramebufferDesc desc;
    desc.width = ctx.viewportWidth;
    desc.height = ctx.viewportHeight;
    desc.colorAttachments = {
        { FramebufferTextureFormat::RGBA16F, FramebufferFilterMode::Nearest, FramebufferWrapMode::ClampToEdge },
        { FramebufferTextureFormat::RGBA16F, FramebufferFilterMode::Nearest, FramebufferWrapMode::ClampToEdge }
    };
    desc.hasDepth = true;
    desc.depthAttachment = { FramebufferTextureFormat::Depth32F };

    m_MaskFramebuffer = Framebuffer(desc);

    frame.registry.Register("MaterialMaskColor", { RenderResourceType::Texture, m_MaskFramebuffer.GetColorAttachment(0) });
    frame.registry.Register("MaterialMaskInfo",  { RenderResourceType::Texture, m_MaskFramebuffer.GetColorAttachment(1) });
    frame.registry.Register("MaterialMaskFBO",   { RenderResourceType::Framebuffer, m_MaskFramebuffer.GetID() });
}

void MaterialOutlinePass::OnResize(uint32_t width, uint32_t height, RenderFrame& frame)
{
    if (m_MaskFramebuffer.GetWidth() == width && m_MaskFramebuffer.GetHeight() == height)
        return;

    m_MaskFramebuffer.Resize(width, height);

    frame.registry.Register("MaterialMaskColor", { RenderResourceType::Texture, m_MaskFramebuffer.GetColorAttachment(0) });
    frame.registry.Register("MaterialMaskInfo",  { RenderResourceType::Texture, m_MaskFramebuffer.GetColorAttachment(1) });
    frame.registry.Register("MaterialMaskFBO",   { RenderResourceType::Framebuffer, m_MaskFramebuffer.GetID() });
}

void MaterialOutlinePass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    bool hasAnyOutline = false;
    bool hasNonXRay = false;
    float maxOutlineWidth = 1.0f;
    bool hasDynamicScale = false;

    auto CheckCommands = [&](const auto& commands) {
        for (const auto& cmd : commands) {
            if (cmd.material && cmd.material->GetHasOutline()) {
                hasAnyOutline = true;
                if (!cmd.material->GetOutlineXRay())
                    hasNonXRay = true;
                maxOutlineWidth = std::max(maxOutlineWidth, cmd.material->GetOutlineWidth());
                if (!cmd.material->GetOutlineFixedSize())
                    hasDynamicScale = true;
            }
        }
    };

    CheckCommands(frame.opaqueCommands);
    CheckCommands(frame.transparentCommands);
    CheckCommands(frame.skeletalOpaqueCommands);
    CheckCommands(frame.skeletalTransparentCommands);

    if (!hasAnyOutline)
        return;

    int outlineRadius = static_cast<int>(std::ceil(maxOutlineWidth));
    if (hasDynamicScale)
        outlineRadius = std::max(outlineRadius, 4);
    outlineRadius = std::clamp(outlineRadius, 1, 8);

    if (m_MaskFramebuffer.GetWidth() != ctx.viewportWidth || m_MaskFramebuffer.GetHeight() != ctx.viewportHeight)
    {
        OnResize(ctx.viewportWidth, ctx.viewportHeight, frame);
    }

    m_MaskFramebuffer.BindViewport();

    GLfloat zeroColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 0, zeroColor);
    GLfloat zeroInfo[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 1, zeroInfo);
    glClear(GL_DEPTH_BUFFER_BIT);

    if (hasNonXRay)
    {
        auto* gbufferRes = frame.registry.Get("GBuffer");
        if (gbufferRes)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, gbufferRes->id);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_MaskFramebuffer.GetID());
            glBlitFramebuffer(0, 0, ctx.viewportWidth, ctx.viewportHeight, 0, 0, ctx.viewportWidth, ctx.viewportHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            m_MaskFramebuffer.BindViewport();
        }
    }

    auto ApplyCullMode = [](CullMode mode) {
        if (mode == CullMode::Disabled) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
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

    {
        Shader& shader = ShaderManager::Get("MaterialMask");
        shader.use();

        auto RenderList = [&](const auto& commands) {
            for (const auto& cmd : commands) {
                if (!cmd.mesh || cmd.mesh->indexCount == 0) continue;
                auto* mat = cmd.material;
                if (!mat || !mat->GetHasOutline()) continue;

                if (mat->GetOutlineXRay())
                {
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                }
                else
                {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LEQUAL);
                    glDepthMask(GL_FALSE);
                }

                ApplyCullMode(mat->GetCullMode());

                glm::mat4 model = cmd.transform->GetWorldMatrix();
                shader.setMat4("model", model);
                shader.setInt("billboardMode", static_cast<int>(mat->GetBillboardMode()));
                shader.setVec4("u_OutlineColor", mat->GetOutlineColor());
                shader.setFloat("u_OutlineWidth", mat->GetOutlineWidth());
                shader.setBool("u_OutlineFixedSize", mat->GetOutlineFixedSize());
                shader.setBool("u_OutlineXRay", mat->GetOutlineXRay());

                bool useAlb = BindMaterial(shader, mat);

                cmd.mesh->VAO.Bind();
                glDrawElements(GL_TRIANGLES, cmd.mesh->indexCount, GL_UNSIGNED_INT, 0);

                if (useAlb) glBindSampler(0, 0);
            }
        };

        RenderList(frame.opaqueCommands);
        RenderList(frame.transparentCommands);
    }

    if (!frame.skeletalOpaqueCommands.empty() || !frame.skeletalTransparentCommands.empty())
    {
        Shader& skeletalShader = ShaderManager::Get("MaterialMask_Skeletal");
        skeletalShader.use();
        GLint boneLoc = glGetUniformLocation(skeletalShader.ID, "u_BoneMatrices");

        auto RenderSkeletalList = [&](const auto& commands) {
            for (const auto& cmd : commands) {
                if (!cmd.mesh || cmd.mesh->indexCount == 0) continue;
                auto* mat = cmd.material;
                if (!mat || !mat->GetHasOutline()) continue;

                if (mat->GetOutlineXRay())
                {
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                }
                else
                {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LEQUAL);
                    glDepthMask(GL_FALSE);
                }

                ApplyCullMode(mat->GetCullMode());

                glm::mat4 model = cmd.transform->GetWorldMatrix();
                skeletalShader.setMat4("model", model);
                skeletalShader.setVec4("u_OutlineColor", mat->GetOutlineColor());
                skeletalShader.setFloat("u_OutlineWidth", mat->GetOutlineWidth());
                skeletalShader.setBool("u_OutlineFixedSize", mat->GetOutlineFixedSize());
                skeletalShader.setBool("u_OutlineXRay", mat->GetOutlineXRay());

                if (boneLoc != -1 && cmd.skeleton && !cmd.skeleton->skinningPalette.empty())
                {
                    glUniformMatrix4fv(
                        boneLoc,
                        static_cast<GLsizei>(cmd.skeleton->skinningPalette.size()),
                        GL_FALSE,
                        glm::value_ptr(cmd.skeleton->skinningPalette[0])
                    );
                }

                bool useAlb = BindMaterial(skeletalShader, mat);

                cmd.mesh->VAO.Bind();
                glDrawElements(GL_TRIANGLES, cmd.mesh->indexCount, GL_UNSIGNED_INT, 0);

                if (useAlb) glBindSampler(0, 0);
            }
        };

        RenderSkeletalList(frame.skeletalOpaqueCommands);
        RenderSkeletalList(frame.skeletalTransparentCommands);
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    auto* finalFboRes = frame.registry.Get("ScreenBuffer");
    if (!finalFboRes)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, finalFboRes->id);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader& outlineShader = ShaderManager::Get("MaterialOutline");
    outlineShader.use();
    outlineShader.setInt("u_Radius", outlineRadius);
    glBindTextureUnit(0, m_MaskFramebuffer.GetColorAttachment(0));
    glBindTextureUnit(1, m_MaskFramebuffer.GetColorAttachment(1));

    auto* depthRes = frame.registry.Get("DepthTexture");
    if (depthRes && depthRes->id != 0)
    {
        glBindTextureUnit(2, depthRes->id);
    }

    Renderer::DrawFullScreenQuad();

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
