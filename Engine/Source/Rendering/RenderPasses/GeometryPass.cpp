#include "rvelapch.h"
#include "GeometryPass.h"
#include "Rendering/Camera.h"
#include "Rendering/RenderContext.h"
#include <Rendering/RenderFrame.h>
#include "Rendering/ShaderManager.h"

using namespace rv;

void GeometryPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    glGenTextures(1, &o_Normal);
    glBindTexture(GL_TEXTURE_2D, o_Normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o_Normal, 0);

    glGenTextures(1, &o_Depth);
    glBindTexture(GL_TEXTURE_2D, o_Depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, ctx.viewportWidth, ctx.viewportHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, o_Depth, 0);

    // GL_R16F
    glGenTextures(1, &o_Roughness);
    glBindTexture(GL_TEXTURE_2D, o_Roughness);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, o_Roughness, 0);

    // GL_R16F
    glGenTextures(1, &o_Metallic);
    glBindTexture(GL_TEXTURE_2D, o_Metallic);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, o_Metallic, 0);

    GLuint attachments[3] = {
        GL_COLOR_ATTACHMENT0, // normal
        GL_COLOR_ATTACHMENT1, // roughness
        GL_COLOR_ATTACHMENT2  // metallic
    };
    glDrawBuffers(3, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        LOG_WARN("Geometry framebuffer not complete!");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frame.registry.Register("DepthTexture", { RenderResourceType::Texture, o_Depth });
    frame.registry.Register("NormalTexture", { RenderResourceType::Texture, o_Normal });
    frame.registry.Register("RoughnessTexture", { RenderResourceType::Texture, o_Roughness });
    frame.registry.Register("MetallicTexture", { RenderResourceType::Texture, o_Metallic });
}



GeometryPass::~GeometryPass()
{
    glDeleteFramebuffers(1, &gBuffer);
    glDeleteTextures(1, &o_Normal);
    glDeleteTextures(1, &o_Depth);
    glDeleteTextures(1, &o_Roughness);
    glDeleteTextures(1, &o_Metallic);
}

void GeometryPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.commands;
    auto& resourceRegisty = frame.registry;

    Shader& geometryShader = ShaderManager::Get("Geometry");

    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    geometryShader.use();


    glm::mat4 projection = ctx.camera->GetProjectionMatrix();
    glm::mat4 view = ctx.camera->GetViewMatrix();

    geometryShader.setMat4("view", view);
    geometryShader.setMat4("projection", projection);

    for (auto& command : commands) {
        if (!ctx.camera->Intersects(command.mesh.worldAABB)) continue;

        auto& material = command.material;
        glm::mat4 model = command.transform.GetWorldMatrix();

        geometryShader.setMat4("model", model);
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(view * model)));
        geometryShader.setMat3("normalMatrix", normalMatrix);
        geometryShader.setVec2("UVScale", material.GetUVScale());
        geometryShader.setVec2("UVOffset", material.GetUVOffset());

        if (material.IsUsingRoughnessMap())
        {
            Ref<TextureAsset> roughnessTexture = material.GetRoughnessTexture();
            geometryShader.setBool("useRoughnessMap", true);
            geometryShader.setInt("roughnessMap", 0);
            roughnessTexture->GetTexture().Bind(0);
            material.GetSampler().Bind(0);

        }
        else
        {
            geometryShader.setBool("useRoughnessMap", false);
            geometryShader.setFloat("roughness", material.GetRoughness());
        }

        if (material.IsUsingMetallicMap())
        {
            Ref<TextureAsset> metallicTexture = material.GetMetallicTexture();
            geometryShader.setBool("useMetallicMap", true);
            geometryShader.setInt("metallicMap", 1);
            metallicTexture->GetTexture().Bind(1);
            material.GetSampler().Bind(1);

        }
        else
        {
            geometryShader.setBool("useMetallicMap", false);
            geometryShader.setFloat("metallic", material.GetMetallic());
        }


        command.mesh.VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
        
        material.GetSampler().Unbind(0);
        material.GetSampler().Unbind(1);
    }

    

    resourceRegisty.Register("DepthTexture",{ RenderResourceType::Texture, o_Depth });
    resourceRegisty.Register("NormalTexture",{ RenderResourceType::Texture, o_Normal });
    resourceRegisty.Register("RoughnessTexture",{ RenderResourceType::Texture, o_Roughness });
    resourceRegisty.Register("MetallicTexture",{ RenderResourceType::Texture, o_Metallic });

}

