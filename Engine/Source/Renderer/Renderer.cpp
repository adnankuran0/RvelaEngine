#include "Renderer.h"

#include "../RvelaLog.h"
#include "Core/Input/Input.h"
#include "Core/Time.h"

GLFWwindow* Renderer::activeWindow = nullptr;




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

void Renderer::Render(WorldTransformComponent& transform, MeshComponent& data, MaterialComponent& metarial, Camera& camera) {

    metarial.shader.use();
    
    metarial.shader.setInt("albedoMap", 0);
    metarial.shader.setInt("normalMap", 1);
    metarial.shader.setInt("metallicMap", 2);
    metarial.shader.setInt("roughnessMap", 3);
    metarial.shader.setInt("aoMap", 4);
    metarial.shader.setInt("heightMap", 5);
    metarial.shader.setFloat("heightScale", 0.0f);
    metarial.shader.setFloat("lightIntensity", 100.0f);
    metarial.shader.setVec3("camPos", camera.Position);
    metarial.shader.setVec3("lightPosition", glm::vec3(0.5f, 2.0f, 1.0f));
    metarial.shader.setVec3("lightColor", glm::vec3(1.0f));
    
    

    metarial.shader.setMat4("model", transform.GetMatrix());
    metarial.shader.setMat4("view", camera.GetViewMatrix());
    metarial.shader.setMat4("projection", camera.projection);
    data.VAO.Bind();
    
    metarial.Albedo.Bind(0);
    metarial.Normal.Bind(1);
    metarial.Metallic.Bind(2);
    metarial.Roughness.Bind(3);
    metarial.Ao.Bind(4);
    metarial.Height.Bind(5);
    
    glDrawElements(GL_TRIANGLES, 20000, GL_UNSIGNED_INT, 0);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERROR << "OpenGL Error: " << err;

    }

    


}



void Renderer::Shutdown()
{
}