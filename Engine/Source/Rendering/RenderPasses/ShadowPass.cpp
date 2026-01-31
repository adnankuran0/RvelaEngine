#include "rvelapch.h"
#include "ShadowPass.h"

void ShadowPass::Init()
{
    
    InitDirectionalShadowMap();
    InitPointShadowMap();
}

void ShadowPass::InitDirectionalShadowMap()
{
    glGenFramebuffers(1, &fbo);

    glGenTextures(1, &o_DirectionalShadowMap);
    glBindTexture(GL_TEXTURE_2D, o_DirectionalShadowMap);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT32F,
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
    // POINT LIGHT SHADOWMAP SETUP
    glGenFramebuffers(1, &pointFBO);

    glGenTextures(1, &o_PointShadowMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, o_PointShadowMap);
    glTexImage3D(
        GL_TEXTURE_CUBE_MAP_ARRAY,
        0,
        GL_DEPTH_COMPONENT32F,
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

void ShadowPass::RenderDirectionalShadowMap()
{
    if (ctx.directionalLight && ctx.directionalLight->castShadows)
    {
        Shader& shadowShader = Renderer::GetDirectionalShadowShader();
        shadowShader.use();

        glm::vec3 lightDir = ctx.directionalLight->direction;
        glm::vec3 sceneCenter = ctx.camera->GetPosition();
        glm::mat4 lightView = glm::lookAt(sceneCenter - lightDir * 50.0f, sceneCenter, glm::vec3(0, 1, 0));
        float orthoSize = 35.0f;
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 120.0f);
        o_LightSpaceMatrix = lightProjection * lightView;

        shadowShader.setMat4("lightSpaceMatrix", o_LightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 2.0f);

        glEnable(GL_CULL_FACE);
        if (ctx.directionalLight->reverseCullFace)
        {
            glCullFace(GL_BACK);
        }
        else
        {
            glCullFace(GL_FRONT);
        }

        for (auto& command : commands) {
            if (!ctx.camera->Intersects(lightProjection * lightView,command.mesh.worldAABB)) continue;
            if (!command.mesh.IsCastShadow()) continue;

            shadowShader.setMat4("model", command.transform.GetWorldMatrix());
            command.mesh.VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        glCullFace(GL_BACK);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

}

void ShadowPass::RenderPointShadowMap()
{
    Shader& pointShadowShader = Renderer::GetPointShadowShader();
    pointShadowShader.use();
    glBindFramebuffer(GL_FRAMEBUFFER, pointFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_PointShadowMap, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);
    glDisable(GL_CULL_FACE);

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
            pointShadowShader.setMat4("model", command.transform.GetWorldMatrix());
            command.mesh.VAO.Bind();

            for (unsigned int face = 0; face < 6; ++face) 
            {
                if (!ctx.camera->Intersects(shadowTransforms[face], command.mesh.worldAABB)) continue;
                if (!command.mesh.IsCastShadow()) continue;
                // Set the current face's matrix
                pointShadowShader.setMat4("shadowMatrix", shadowTransforms[face]);
                // Set the current face layer
                pointShadowShader.setInt("currentFace", face);

                glDrawElements(
                    GL_TRIANGLES,
                    command.mesh.indexCount,
                    GL_UNSIGNED_INT,
                    0
                );
            }
        }
    }

    glEnable(GL_CULL_FACE);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
}

ShadowPass::~ShadowPass()
{
    //TODO: Fill this function
}

void ShadowPass::Execute()
{
    if (commands.empty() || !ctx.IsValid()) return;

    RenderDirectionalShadowMap();
    RenderPointShadowMap();

    commands.clear();
}