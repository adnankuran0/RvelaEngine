#include "rvelapch.h"
#include "Renderer.h"

#include "../RvelaLog.h"
#include "Core/Input/Input.h"
#include "Core/Time.h"
#include "Core/Utils/MaterialManager.h"
#include "Core/Utils/TextureManager.h"
#include <algorithm>

GLFWwindow* Renderer::activeWindow = nullptr;
Shader Renderer::m_DefaultShader;
Shader Renderer::m_SkyboxShader;
Skybox Renderer::m_Skybox;



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
    m_DefaultShader.Init("C:/RvelaEngine/Resources/Engine/Shaders/vertex.glsl", "C:/RvelaEngine/Resources/Engine/Shaders/fragment.glsl");
    m_SkyboxShader.Init("C:/RvelaEngine/Resources/Engine/Shaders/skyboxVert.glsl", "C:/RvelaEngine/Resources/Engine/Shaders/skyboxFrag.glsl");

    std::vector<std::string> faces = {
    "C:/RvelaEngine/Resources/Engine/Textures/skybox/right.jpg",
    "C:/RvelaEngine/Resources/Engine/Textures/skybox/left.jpg",
    "C:/RvelaEngine/Resources/Engine/Textures/skybox/top.jpg",
    "C:/RvelaEngine/Resources/Engine/Textures/skybox/bottom.jpg",
    "C:/RvelaEngine/Resources/Engine/Textures/skybox/front.jpg",
    "C:/RvelaEngine/Resources/Engine/Textures/skybox/back.jpg"
    };
    m_Skybox.Init(faces);
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

void Renderer::RenderSkybox(EditorCamera* camera)
{
    m_Skybox.Render(m_SkyboxShader, camera->projection, camera->GetViewMatrix());
}

void Renderer::Render(WorldTransformComponent& transform,
    MeshRendererComponent& meshComponent,
    MaterialComponent& materialComponent,
    EditorCamera* camera,
    const std::vector<PointLightData>& pointLights,
    const DirectionalLightData* directionalLight)
{
    auto material = materialComponent.material;
    if (!material) return;


    m_DefaultShader.use();

    m_DefaultShader.setBool("hasDirectionalLight", directionalLight != nullptr);
    if (directionalLight) {
        m_DefaultShader.setVec3("directionalLight.direction", directionalLight->direction);
        m_DefaultShader.setVec3("directionalLight.color", directionalLight->color); // intensity çarpımı KALDIRILDI
        m_DefaultShader.setFloat("directionalLight.intensity", directionalLight->intensity); // Yeni ekleme
        m_DefaultShader.setBool("directionalLight.castShadows", false);
    }

    m_DefaultShader.setFloat("heightScale", 0.0f);

    const int MAX_POINT_LIGHTS = 10;
    for (int i = 0; i < std::min(static_cast<int>(pointLights.size()), MAX_POINT_LIGHTS); ++i) {
        std::string baseName = "pointLights[" + std::to_string(i) + "]";
        m_DefaultShader.setVec3(baseName + ".position", pointLights[i].position);
        m_DefaultShader.setVec3(baseName + ".color", pointLights[i].color);
        m_DefaultShader.setFloat(baseName + ".intensity", pointLights[i].intensity);
        m_DefaultShader.setFloat(baseName + ".radius", pointLights[i].radius);
    }
    m_DefaultShader.setInt("pointLightCount", std::min(static_cast<int>(pointLights.size()), MAX_POINT_LIGHTS));

    m_DefaultShader.setVec3("camPos", camera->Position);

    m_DefaultShader.setVec3("UVScale", material->UVScale);
    m_DefaultShader.setVec3("UVOffset", material->UVOffset);
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
    m_DefaultShader.setMat4("view", camera->GetViewMatrix());
    m_DefaultShader.setMat4("projection", camera->projection);

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