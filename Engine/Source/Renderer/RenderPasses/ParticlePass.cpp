#include "rvelapch.h"
#include "ParticlePass.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"
#include "Core/Log.h"
#include "Asset/AssetManager.h"
#include "Scene/Components.h"
#include "Renderer/RenderContext.h"
#include "Scene/Environment.h"
#include "Scene/ParticleSystem.h"
#include "Scene/Scene.h"

namespace rv {

ParticlePass::~ParticlePass()
{
    if (m_InstanceSSBO != 0)
    {
        glDeleteBuffers(1, &m_InstanceSSBO);
    }
}

void ParticlePass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    glCreateBuffers(1, &m_InstanceSSBO);
}

void ParticlePass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto& pCommands = frame.particleCommands;
    if (pCommands.empty()) return;

    auto& resourceRegistry = frame.registry;
    auto screenFBO = resourceRegistry.Get("ScreenBuffer")->id;
    auto intermediateFBO = resourceRegistry.Get("IntermediateBuffer")->id;

    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    auto& packedData = ctx.scene->GetParticleSystem().GetPackedInstanceBuffer();
    glNamedBufferData(m_InstanceSSBO, packedData.size() * sizeof(ParticleInstanceData), packedData.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_InstanceSSBO);

    Shader& shader = ShaderManager::Get("Particle");
    shader.use();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);

    auto ApplyCullMode = [](CullMode mode) {
        if (mode == CullMode::Disabled) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
            glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
        }
        };

    auto ApplyBlendMode = [](BlendMode mode) {
        switch (mode) {
        case BlendMode::Mix:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBlendEquation(GL_FUNC_ADD);
            break;
        case BlendMode::Add:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
            break;
        case BlendMode::Subtract:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
            break;
        case BlendMode::Multiply:
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            glBlendEquation(GL_FUNC_ADD);
            break;
        }
        };

    for (const auto& cmd : pCommands) {
        auto* material = cmd.material;

        ApplyCullMode(material->GetCullMode());
        ApplyBlendMode(material->GetBlendMode());

        shader.setVec4("albedoColor", material->GetAlbedoColor());
        shader.setVec3("emmisiveColor", material->GetEmissiveColor());
        shader.setFloat("emmisiveIntensity", material->GetEmissiveIntensity());
        shader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
        shader.setFloat("alphaCutoff", material->GetAlphaCutoff());

        shader.setVec2("UVScale", material->GetUVScale());
        shader.setVec2("UVOffset", material->GetUVOffset());

        shader.setInt("billboardMode", 1);
        shader.setInt("instanceOffset", cmd.instanceOffset);

        if (material->IsUsingAlbedoMap() && material->GetAlbedoTexture()) {
            shader.setInt("albedoMap", 0);
            TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
            material->GetSampler().Bind(0);
            shader.setBool("useAlbedoMap", true);
        }
        else {
            shader.setBool("useAlbedoMap", false);
            glBindSampler(0, 0);
        }

        cmd.mesh->VAO.Bind();
        glDrawElementsInstanced(GL_TRIANGLES, cmd.mesh->indexCount, GL_UNSIGNED_INT, 0, cmd.instanceCount);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, screenFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
    glBlitFramebuffer(
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
}

}