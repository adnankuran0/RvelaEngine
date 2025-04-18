#include "rvelapch.h"
#include "Renderer.h"

#include "../RvelaLog.h"
#include "Core/Input/Input.h"
#include "Core/Time.h"
#include "Core/Utils/MaterialManager.h"
#include "Core/Utils/TextureManager.h"

GLFWwindow* Renderer::activeWindow = nullptr;
Shader Renderer::m_DefaultShader;



Renderer::Renderer() 
{
    
};

Renderer::~Renderer()
{
    Shutdown();
}

void Renderer::Init(GLFWwindow* window)
{
    Renderer::activeWindow = window;
    m_DefaultShader.Init("D:/GitHub/RvelaEngine/Resources/Engine/Shaders/vertex.glsl", "D:/GitHub/RvelaEngine/Resources/Engine/Shaders/fragment.glsl");
}

void Renderer::StartFrame()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void Renderer::EndFrame()
{

}

void Renderer::Render(WorldTransformComponent& transform, MeshComponent& meshComponent, MaterialComponent& materialComponent, Camera& camera)
{
    auto material = materialComponent.material;
    if (!material) return;

    m_DefaultShader.use();

    m_DefaultShader.setFloat("heightScale", 0.0f);
    m_DefaultShader.setFloat("lightIntensity", 100.0f);
    m_DefaultShader.setVec3("camPos", camera.Position);
    m_DefaultShader.setVec3("lightPosition", glm::vec3(0.5f, 2.0f, 1.0f));
    m_DefaultShader.setVec3("lightColor", glm::vec3(1.0f));
    m_DefaultShader.setFloat("UVScale", 1.0f);
    m_DefaultShader.setVec3("albedoColor", material->albedoColor);
    m_DefaultShader.setFloat("metallicValue", material->metallic);
    m_DefaultShader.setFloat("roughnessValue", material->roughness);
    m_DefaultShader.setFloat("aoValue", material->ao);

    struct MapInfo {
        std::string path;
        std::string uniformName;
        int slot;
        std::string useUniform;
    };

    std::vector<MapInfo> maps = {
        { material->albedoMapPath,    "albedoMap",    0, "useAlbedoMap" },
        { material->normalMapPath,    "normalMap",    1, "useNormalMap" },
        { material->metallicMapPath,  "metallicMap",  2, "useMetallicMap" },
        { material->roughnessMapPath, "roughnessMap", 3, "useRoughnessMap" },
        { material->aoMapPath,        "aoMap",        4, "useAOMap" },
        { material->heightMapPath,    "heightMap",    5, "useHeightMap" },
    };

    for (const auto& map : maps) {
        bool hasMap = !map.path.empty();
        m_DefaultShader.setBool(map.useUniform, hasMap);
        if (hasMap) {
            auto tex = TextureManager::LoadOrGetTexture(map.path);
            if (tex) {
                m_DefaultShader.setInt(map.uniformName, map.slot);
                tex->Bind(map.slot);
            }
        }
    }

    m_DefaultShader.setMat4("model", transform.GetMatrix());
    m_DefaultShader.setMat4("view", camera.GetViewMatrix());
    m_DefaultShader.setMat4("projection", camera.projection);

    meshComponent.VAO.Bind();
    glDrawElements(GL_TRIANGLES, meshComponent.indexCount, GL_UNSIGNED_INT, 0);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERROR << "OpenGL Error: " << err;
    }
}



void Renderer::Shutdown()
{
    m_DefaultShader.Destroy();
}