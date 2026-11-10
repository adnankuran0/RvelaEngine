#include "rvelapch.h"
#include "ReflectionProbeManager.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"
#include "Renderer/Skybox.h"
#include "Scene/Environment.h"
#include "Scene/LightSystem.h"
#include "Core/Log.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

namespace rv {

void ReflectionProbeManager::Init()
{
    if (m_Initialized)
        return;

    const GLfloat cubeVertices[] = {
        -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_UnitCubeVAO);
    glGenBuffers(1, &m_UnitCubeVBO);
    glBindVertexArray(m_UnitCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_UnitCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    glGenTextures(1, &m_CubeArrayTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_CubeArrayTexture);
    glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, m_MipLevels, GL_RGBA16F, m_Resolution, m_Resolution, m_MaxProbes * 6);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(m_MipLevels - 1));

    glGenTextures(1, &m_CaptureCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_CaptureCubeMap);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, m_MipLevels, GL_RGBA16F, m_Resolution, m_Resolution);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(m_MipLevels - 1));

    glGenFramebuffers(1, &m_CaptureFBO);
    glGenRenderbuffers(1, &m_CaptureRBO);

    const float clearColor[4] = { 0.05f, 0.05f, 0.05f, 1.0f };
    for (uint32_t mip = 0; mip < m_MipLevels; ++mip)
    {
        glClearTexImage(m_CubeArrayTexture, static_cast<GLint>(mip), GL_RGBA, GL_FLOAT, clearColor);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_Initialized = true;
}

void ReflectionProbeManager::Shutdown()
{
    if (!m_Initialized)
        return;

    if (m_CaptureFBO != 0) { glDeleteFramebuffers(1, &m_CaptureFBO); m_CaptureFBO = 0; }
    if (m_CaptureRBO != 0) { glDeleteRenderbuffers(1, &m_CaptureRBO); m_CaptureRBO = 0; }
    if (m_CaptureCubeMap != 0) { glDeleteTextures(1, &m_CaptureCubeMap); m_CaptureCubeMap = 0; }
    if (m_CubeArrayTexture != 0) { glDeleteTextures(1, &m_CubeArrayTexture); m_CubeArrayTexture = 0; }
    if (m_UnitCubeVAO != 0) { glDeleteVertexArrays(1, &m_UnitCubeVAO); m_UnitCubeVAO = 0; }
    if (m_UnitCubeVBO != 0) { glDeleteBuffers(1, &m_UnitCubeVBO); m_UnitCubeVBO = 0; }

    m_Initialized = false;
}

void ReflectionProbeManager::RenderUnitCube()
{
    if (m_UnitCubeVAO != 0)
    {
        glBindVertexArray(m_UnitCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
}

void ReflectionProbeManager::BakeProbe(Scene& scene, entt::entity entity, ReflectionProbeComponent& probe)
{
    if (!m_Initialized)
        Init();

    auto& registry = scene.GetRegistry();
    if (!registry.all_of<TransformComponent>(entity))
        return;

    if (probe.cubeIndex < 0 || probe.cubeIndex >= static_cast<int>(m_MaxProbes))
    {
        std::vector<bool> used(m_MaxProbes, false);
        for (auto otherE : registry.view<ReflectionProbeComponent>())
        {
            if (otherE != entity)
            {
                int idx = registry.get<ReflectionProbeComponent>(otherE).cubeIndex;
                if (idx >= 0 && idx < static_cast<int>(m_MaxProbes))
                    used[idx] = true;
            }
        }
        probe.cubeIndex = 0;
        for (size_t i = 0; i < m_MaxProbes; ++i)
        {
            if (!used[i])
            {
                probe.cubeIndex = static_cast<int>(i);
                break;
            }
        }
    }

    auto& tc = registry.get<TransformComponent>(entity);
    glm::vec3 capturePos = tc.GetWorldPosition() + probe.originOffset;

    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);
    GLint prevCullFaceMode = GL_BACK;
    glGetIntegerv(GL_CULL_FACE_MODE, &prevCullFaceMode);
    GLint prevDepthFunc = GL_LESS;
    glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    GLboolean prevDepthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);

    glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);
    glm::mat4 captureViews[6] = {
        glm::lookAt(capturePos, capturePos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(capturePos, capturePos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(capturePos, capturePos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(capturePos, capturePos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(capturePos, capturePos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(capturePos, capturePos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    glm::mat4 prefilterProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 prefilterViews[6] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    auto& lightSystem = scene.GetLightSystem();
    auto dirLightOpt = lightSystem.CollectDirectionalLight(capturePos);
    glm::vec3 lightDir = dirLightOpt ? dirLightOpt->direction : glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 lightColor = dirLightOpt ? (dirLightOpt->color * dirLightOpt->intensity) : glm::vec3(1.0f);
    auto& env = scene.GetEnvironment();
    glm::vec3 ambientColor = env.Lighting_AmbientColor * env.Lighting_AmbientIntensity;
    if (glm::length(ambientColor) < 0.01f)
        ambientColor = glm::vec3(0.1f);

    auto& skybox = env.GetSkybox();
    if (skybox.GetEnvironmentMap() == 0)
    {
        if (skybox.GetHDRUUID().IsValid()) skybox.InitHDR(skybox.GetHDRUUID());
        else if (skybox.GetPath().IsValid()) skybox.InitHDR(skybox.GetPath());
        else {
            Path defaultPath = VRT_PATH("Assets\\Textures\\skybox\\environment.hdr");
            if (std::filesystem::exists(defaultPath.GetAbsolute())) skybox.InitHDR(defaultPath);
        }
    }

    Shader& captureShader = ShaderManager::Get("ProbeCapture");
    auto pointLights = lightSystem.CollectPointLights();
    int ptCount = std::min(static_cast<int>(pointLights.size()), 16);

    glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_Resolution, m_Resolution);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_CaptureRBO);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glViewport(0, 0, m_Resolution, m_Resolution);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    for (int face = 0; face < 6; ++face)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_CaptureCubeMap, 0);

        GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            LOG_ERROR("ReflectionProbeManager Phase 1 FBO incomplete on face {}: {:#x}", face, fboStatus);
        }

        glm::vec3 clearCol = glm::max(ambientColor, glm::vec3(0.05f));
        glClearColor(clearCol.r, clearCol.g, clearCol.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!probe.interior && skybox.GetEnvironmentMap() != 0)
        {
            skybox.Render(captureProj, captureViews[face], m_CaptureFBO);
        }

        captureShader.use();
        captureShader.setInt("u_AlbedoMap", 0);
        captureShader.setMat4("u_Projection", captureProj);
        captureShader.setMat4("u_View", captureViews[face]);
        captureShader.setVec3("u_LightDir", lightDir);
        captureShader.setVec3("u_LightColor", lightColor);
        captureShader.setVec3("u_AmbientColor", ambientColor);
        captureShader.setInt("u_PointLightCount", ptCount);
        for (int i = 0; i < ptCount; ++i)
        {
            captureShader.setVec4("u_PointLights[" + std::to_string(i) + "].positionRadius",
                glm::vec4(pointLights[i].position, pointLights[i].radius));
            captureShader.setVec4("u_PointLights[" + std::to_string(i) + "].colorIntensity",
                glm::vec4(pointLights[i].color, pointLights[i].intensity));
        }

        for (auto e : registry.view<TransformComponent, MeshRendererComponent>())
        {
            auto& meshComp = registry.get<MeshRendererComponent>(e);
            if (!meshComp.gpuMesh || meshComp.indexCount == 0) continue;

            auto& objTc = registry.get<TransformComponent>(e);
            captureShader.setMat4("u_Model", objTc.GetWorldMatrix());

            if (registry.all_of<MaterialComponent>(e))
            {
                auto& matComp = registry.get<MaterialComponent>(e);
                captureShader.setVec4("u_AlbedoColor", matComp.GetAlbedoColor());
                captureShader.setVec3("u_EmissiveColor", matComp.GetEmissiveColor());
                captureShader.setFloat("u_EmissiveIntensity", matComp.GetEmissiveIntensity());
                captureShader.setVec2("u_UVScale", matComp.GetUVScale());
                captureShader.setVec2("u_UVOffset", matComp.GetUVOffset());
                captureShader.setBool("u_UseTriplanar", matComp.GetUseTriplanar());
                captureShader.setBool("u_UseWorldTriplanar", matComp.GetUseWorldTriplanar());
                captureShader.setFloat("u_TriplanarSharpness", matComp.GetTriplanarSharpness());

                bool useAlb = matComp.IsUsingAlbedoMap() && matComp.GetAlbedoTexture();
                captureShader.setBool("u_UseAlbedoMap", useAlb);
                if (useAlb)
                {
                    glBindTextureUnit(0, TextureCache::Get().GetOrCreate(matComp.GetAlbedoTexture()).GetID());
                    matComp.GetSampler().Bind(0);
                }
                else
                {
                    glBindSampler(0, 0);
                    glBindTextureUnit(0, 0);
                }
            }
            else
            {
                captureShader.setVec4("u_AlbedoColor", glm::vec4(1.0f));
                captureShader.setVec3("u_EmissiveColor", glm::vec3(0.0f));
                captureShader.setFloat("u_EmissiveIntensity", 0.0f);
                captureShader.setVec2("u_UVScale", glm::vec2(1.0f));
                captureShader.setVec2("u_UVOffset", glm::vec2(0.0f));
                captureShader.setBool("u_UseTriplanar", false);
                captureShader.setBool("u_UseWorldTriplanar", false);
                captureShader.setFloat("u_TriplanarSharpness", 4.0f);
                captureShader.setBool("u_UseAlbedoMap", false);
                glBindSampler(0, 0);
                glBindTextureUnit(0, 0);
            }

            meshComp.Bind();
            glDrawElements(GL_TRIANGLES, meshComp.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    glBindSampler(0, 0);
    glBindTextureUnit(0, 0);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_CaptureCubeMap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    Shader& prefilterShader = ShaderManager::Get("Prefilter");
    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", prefilterProj);

    glBindSampler(0, 0);
    glBindTextureUnit(0, m_CaptureCubeMap);

    int baseLayer = probe.cubeIndex * 6;

    for (unsigned int mip = 0; mip < m_MipLevels; ++mip)
    {
        unsigned int mipW = static_cast<unsigned int>(m_Resolution * std::pow(0.5f, mip));
        unsigned int mipH = static_cast<unsigned int>(m_Resolution * std::pow(0.5f, mip));
        glViewport(0, 0, mipW, mipH);

        float roughness = static_cast<float>(mip) / static_cast<float>(m_MipLevels - 1);
        prefilterShader.setFloat("roughness", roughness);

        for (int face = 0; face < 6; ++face)
        {
            prefilterShader.setMat4("view", prefilterViews[face]);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                m_CubeArrayTexture, mip, baseLayer + face);

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
            {
                LOG_ERROR("ReflectionProbeManager Phase 3 FBO incomplete at mip {} face {}: {:#x}", mip, face, status);
            }

            glClear(GL_COLOR_BUFFER_BIT);
            RenderUnitCube();
        }
    }

    glBindTextureUnit(0, 0);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(prevDepthMask);
    glDepthFunc(prevDepthFunc);
    if (prevCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    glCullFace(prevCullFaceMode);

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    probe.isDirty = false;

    float testPixel[4] = { 0.0f };
    glGetTextureSubImage(m_CubeArrayTexture, 0, m_Resolution / 2, m_Resolution / 2, baseLayer, 1, 1, 1, GL_RGBA, GL_FLOAT, sizeof(testPixel), testPixel);
}

void ReflectionProbeManager::BakeAllDirtyProbes(Scene& scene)
{
    if (!m_Initialized)
        Init();

    auto& registry = scene.GetRegistry();
    int assignedIndex = 0;

    for (auto e : registry.view<TransformComponent, ReflectionProbeComponent>())
    {
        if (!scene.IsEntityActive(e))
            continue;

        auto& probe = registry.get<ReflectionProbeComponent>(e);
        if (assignedIndex < static_cast<int>(m_MaxProbes))
        {
            probe.cubeIndex = assignedIndex++;
        }
        else
        {
            probe.cubeIndex = -1;
        }

        if (probe.isDirty && probe.cubeIndex >= 0)
        {
            BakeProbe(scene, e, probe);
        }
    }
}

void ReflectionProbeManager::PrepareProbes(Scene& scene, GPUReflectionProbeUBO& outUbo)
{
    outUbo.probeCount = 0;
    auto& registry = scene.GetRegistry();

    for (auto e : registry.view<TransformComponent, ReflectionProbeComponent>())
    {
        if (!scene.IsEntityActive(e))
            continue;

        if (outUbo.probeCount >= static_cast<int>(m_MaxProbes))
            break;

        auto& tc = registry.get<TransformComponent>(e);
        auto& probe = registry.get<ReflectionProbeComponent>(e);

        if (probe.cubeIndex < 0)
            continue;

        glm::vec3 center = tc.GetWorldPosition();
        glm::vec3 extents = probe.extents * tc.GetWorldScale();

        auto& p = outUbo.probes[outUbo.probeCount];
        p.position = glm::vec4(center, probe.intensity);
        p.boxExtents = glm::vec4(extents, probe.blendDistance);
        p.originOffset = glm::vec4(probe.originOffset, static_cast<float>(probe.priority));
        p.params = glm::vec4(
            probe.boxProjection ? 1.0f : 0.0f,
            probe.interior ? 1.0f : 0.0f,
            static_cast<float>(probe.cubeIndex),
            0.0f
        );

        outUbo.probeCount++;
    }
}

}
