#include "rvelapch.h"
#include "BloomPass.h"
#include "Scene/Components.h"
#include "Renderer/Camera.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderContext.h"
#include "Renderer/ShaderManager.h"
#include "Scene/Environment.h"
#include <algorithm>

using namespace rv;

void BloomPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    m_DownsampleFBOs.clear();
    m_DownsampleFBOs.reserve(s_MipLevels);

    for (int i = 0; i < s_MipLevels; ++i)
    {
        uint32_t w = std::max(1u, ctx.viewportWidth >> i);
        uint32_t h = std::max(1u, ctx.viewportHeight >> i);

        FramebufferDesc desc;
        desc.width = w;
        desc.height = h;
        desc.hasDepth = false;
        desc.colorAttachments = {
            { FramebufferTextureFormat::RGB16F, FramebufferFilterMode::Linear, FramebufferWrapMode::ClampToEdge }
        };

        m_DownsampleFBOs.emplace_back(desc);
    }

    frame.registry.Register("BloomTexture", { RenderResourceType::Texture, m_DownsampleFBOs[0].GetColorAttachment(0) });
}

void BloomPass::Downsample(const RenderContext& ctx, RenderFrame& frame)
{
    auto i_BrightTexture = frame.registry.Get("BrightTexture")->id;
    Shader& downsampleShader = ShaderManager::Get("Downsample");

    downsampleShader.use();
    downsampleShader.setInt("u_Texture", 0);

    for (int i = 0; i < s_MipLevels; ++i)
    {
        m_DownsampleFBOs[i].BindViewport();

        int w = std::max(1, static_cast<int>(ctx.viewportWidth >> i));
        int h = std::max(1, static_cast<int>(ctx.viewportHeight >> i));
        downsampleShader.setVec2("u_TexelSize", glm::vec2(1.0f / w, 1.0f / h));

        if (i == 0)
            glBindTextureUnit(0, i_BrightTexture);
        else
            glBindTextureUnit(0, m_DownsampleFBOs[i - 1].GetColorAttachment(0));

        Renderer::DrawFullScreenQuad();
    }
}

void BloomPass::Upsample(const RenderContext& ctx, RenderFrame& frame)
{
    Shader& upsampleShader = ShaderManager::Get("Upsample");
    upsampleShader.use();
    upsampleShader.setInt("u_LowMip", 0);
    upsampleShader.setInt("u_BaseMip", 1);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    for (int i = s_MipLevels - 2; i >= 0; --i)
    {
        m_DownsampleFBOs[i].BindViewport();

        glBindTextureUnit(0, m_DownsampleFBOs[i + 1].GetColorAttachment(0));
        glBindTextureUnit(1, m_DownsampleFBOs[i].GetColorAttachment(0));

        Renderer::DrawFullScreenQuad();
    }
}

void BloomPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (!ctx.environment->Bloom) return;

    if (!m_DownsampleFBOs.empty() &&
        (m_DownsampleFBOs[0].GetWidth() != ctx.viewportWidth || m_DownsampleFBOs[0].GetHeight() != ctx.viewportHeight))
    {
        for (int i = 0; i < s_MipLevels; ++i)
        {
            uint32_t w = std::max(1u, ctx.viewportWidth >> i);
            uint32_t h = std::max(1u, ctx.viewportHeight >> i);
            m_DownsampleFBOs[i].Resize(w, h);
        }
        frame.registry.Register("BloomTexture", { RenderResourceType::Texture, m_DownsampleFBOs[0].GetColorAttachment(0) });
    }

    Downsample(ctx, frame);
    Upsample(ctx, frame);

    Framebuffer::BindDefault();
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
}