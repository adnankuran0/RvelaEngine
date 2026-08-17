#include "rvelapch.h"
#include "GeometryPass.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"

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

    glGenTextures(1, &o_Roughness);
    glBindTexture(GL_TEXTURE_2D, o_Roughness);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, o_Roughness, 0);

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
    auto& commands = frame.opaqueCommands;
    auto& resourceRegistry = frame.registry;

    Shader& geometryShader = ShaderManager::Get("Geometry");

    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    geometryShader.use();

    glm::mat4 projection = ctx.camera->GetProjectionMatrix();
    glm::mat4 view = ctx.camera->GetViewMatrix();

    geometryShader.setMat4("view", view);
    geometryShader.setMat4("projection", projection);
    geometryShader.setVec3("camPos", ctx.camera->Position);

    auto ApplyCullMode = [](CullMode mode) {
        if (mode == CullMode::Disabled) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
            glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
        }
        };

    for (auto& command : commands) {
        if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue;

        auto& material = command.material;
        ApplyCullMode(material->GetCullMode());

        glm::mat4 model = command.transform->GetWorldMatrix();

        geometryShader.setMat4("model", model);
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(view * model)));
        geometryShader.setMat3("normalMatrix", normalMatrix);
        geometryShader.setVec2("UVScale", material->GetUVScale());
        geometryShader.setVec2("UVOffset", material->GetUVOffset());

        geometryShader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
        geometryShader.setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
        geometryShader.setFloat("alphaCutoff", material->GetAlphaCutoff());
        geometryShader.setVec4("albedoColor", material->GetAlbedoColor());

        // Alpha scissor
        bool useAlb = material->IsUsingAlbedoMap() && material->GetAlbedoTexture();
        geometryShader.setBool("useAlbedoMap", useAlb);
        if (useAlb)
        {
            geometryShader.setInt("albedoMap", 0);
            TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
            material->GetSampler().Bind(0);
        }
        else
        {
            glBindSampler(0, 0);
            glBindTextureUnit(0, 0);
        }

        // Roughness (Slot 1)
        if (material->IsUsingRoughnessMap() && material->GetRoughnessTexture())
        {
            geometryShader.setBool("useRoughnessMap", true);
            geometryShader.setInt("roughnessMap", 1);
            TextureCache::Get().GetOrCreate(material->GetRoughnessTexture()).Bind(1);
            material->GetSampler().Bind(1);
        }
        else
        {
            geometryShader.setBool("useRoughnessMap", false);
            geometryShader.setFloat("roughness", material->GetRoughness());
            glBindSampler(1, 0);
            glBindTextureUnit(1, 0);
        }

        // Metallic (Slot 2)
        if (material->IsUsingMetallicMap() && material->GetMetallicTexture())
        {
            geometryShader.setBool("useMetallicMap", true);
            geometryShader.setInt("metallicMap", 2);
            TextureCache::Get().GetOrCreate(material->GetMetallicTexture()).Bind(2);
            material->GetSampler().Bind(2);
        }
        else
        {
            geometryShader.setBool("useMetallicMap", false);
            geometryShader.setFloat("metallic", material->GetMetallic());
            glBindSampler(2, 0);
            glBindTextureUnit(2, 0);
        }

        command.mesh->VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

        glBindSampler(0, 0);
        glBindSampler(1, 0);
        glBindSampler(2, 0);
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    resourceRegistry.Register("DepthTexture", { RenderResourceType::Texture, o_Depth });
    resourceRegistry.Register("NormalTexture", { RenderResourceType::Texture, o_Normal });
    resourceRegistry.Register("RoughnessTexture", { RenderResourceType::Texture, o_Roughness });
    resourceRegistry.Register("MetallicTexture", { RenderResourceType::Texture, o_Metallic });
}