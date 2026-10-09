#include "rvelapch.h"
#include "DecalPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"
#include "Renderer/Camera.h"
#include "Scene/Environment.h"
#include <algorithm>

namespace rv {

DecalPass::~DecalPass()
{
    if (m_CubeVAO != 0)
    {
        glDeleteVertexArrays(1, &m_CubeVAO);
        m_CubeVAO = 0;
    }
    if (m_CubeVBO != 0)
    {
        glDeleteBuffers(1, &m_CubeVBO);
        m_CubeVBO = 0;
    }
}

void DecalPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    static const float cubeVertices[] = {
        // Back
        -0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,

        // Front
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        // Left
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        // Right
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,

        // Bottom
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        // Top
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f
    };

    glGenVertexArrays(1, &m_CubeVAO);
    glGenBuffers(1, &m_CubeVBO);

    glBindVertexArray(m_CubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DecalPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (frame.decalCommands.empty())
        return;

    auto* screenRes = frame.registry.Get("ScreenBuffer");
    auto* depthRes = frame.registry.Get("DepthTexture");
    auto* normalRes = frame.registry.Get("NormalTexture");

    if (!screenRes || !depthRes || !normalRes)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, screenRes->id);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    std::sort(frame.decalCommands.begin(), frame.decalCommands.end(),
        [](const DecalRenderCommand& a, const DecalRenderCommand& b) {
            if (a.decal->renderOrder != b.decal->renderOrder)
                return a.decal->renderOrder < b.decal->renderOrder;
            return a.distanceToCamera > b.distanceToCamera;
        });

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    Shader& shader = ShaderManager::Get("Decal");
    shader.use();

    glBindTextureUnit(0, depthRes->id);
    glBindTextureUnit(1, normalRes->id);

    auto& env = *ctx.environment;
    auto& skybox = env.GetSkybox();

    shader.setVec3("ambientColor", env.Lighting_AmbientColor);
    shader.setFloat("ambientIntensity", env.Lighting_AmbientIntensity);
    shader.setBool("useIBL", env.Lighting_IBL);
    shader.setFloat("iblIntensity", env.Lighting_IBLIntensity);

    auto* dirShadowRes = frame.registry.Get("DirectionalShadowMap");
    auto* pointShadowRes = frame.registry.Get("PointShadowMap");

    constexpr int DIR_SHADOW_MAP_SLOT = 6;
    constexpr int POINT_SHADOW_MAP_SLOT = 7;
    constexpr int IRRADIANCE_MAP_SLOT = 8;

    shader.setInt("shadowMap", DIR_SHADOW_MAP_SLOT);
    if (dirShadowRes)
        glBindTextureUnit(DIR_SHADOW_MAP_SLOT, dirShadowRes->id);

    shader.setInt("pointShadowMap", POINT_SHADOW_MAP_SLOT);
    if (pointShadowRes)
        glBindTextureUnit(POINT_SHADOW_MAP_SLOT, pointShadowRes->id);

    shader.setInt("irradianceMap", IRRADIANCE_MAP_SLOT);
    if (skybox.GetIrradianceMap() != 0)
        glBindTextureUnit(IRRADIANCE_MAP_SLOT, skybox.GetIrradianceMap());

    glBindVertexArray(m_CubeVAO);

    for (const auto& cmd : frame.decalCommands)
    {
        if (cmd.decal->distanceFade)
        {
            float maxDistSq = cmd.decal->fadeEnd * cmd.decal->fadeEnd;
            if (cmd.distanceToCamera > maxDistSq)
                continue;
        }

        glm::mat4 model = cmd.transform->GetWorldMatrix();
        glm::mat4 invModel = glm::inverse(model);

        shader.setMat4("u_Model", model);
        shader.setMat4("u_InvModel", invModel);

        shader.setVec4("u_Color", cmd.decal->color);
        shader.setFloat("u_Opacity", cmd.decal->opacity);
        shader.setFloat("u_AngleCutoff", cmd.decal->angleCutoff);

        shader.setBool("u_Lit", cmd.decal->lit);
        shader.setBool("u_ReceiveShadows", cmd.decal->receiveShadows);

        shader.setFloat("u_UpperFade", cmd.decal->upperFade);
        shader.setFloat("u_LowerFade", cmd.decal->lowerFade);
        shader.setBool("u_DistanceFade", cmd.decal->distanceFade);
        shader.setFloat("u_FadeStart", cmd.decal->fadeStart);
        shader.setFloat("u_FadeEnd", cmd.decal->fadeEnd);

        auto texAsset = cmd.decal->GetTexture();
        if (texAsset)
        {
            Texture& tex = TextureCache::Get().GetOrCreate(texAsset);
            glBindTextureUnit(2, tex.GetID());
            shader.setBool("u_HasTexture", true);
        }
        else
        {
            glBindTextureUnit(2, 0);
            shader.setBool("u_HasTexture", false);
        }

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);
    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);
    glBindTextureUnit(DIR_SHADOW_MAP_SLOT, 0);
    glBindTextureUnit(POINT_SHADOW_MAP_SLOT, 0);
    glBindTextureUnit(IRRADIANCE_MAP_SLOT, 0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glCullFace(GL_BACK);
}

}
