#include "Renderer.h"

#include "../Resources/cube.h"
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

    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void Renderer::EndFrame()
{
    glfwSwapBuffers(activeWindow);

}

void Renderer::Render(TransformComponent& transform, MeshComponent& data, MetarialComponent& metarial, Camera& camera) {

    metarial.shader.use();
    metarial.shader.setInt("albedoMap", 0);
    metarial.shader.setInt("normalMap", 1);
    metarial.shader.setInt("metallicMap", 2);
    metarial.shader.setInt("roughnessMap", 3);
    metarial.shader.setInt("aoMap", 4);
    metarial.shader.setInt("heightMap", 5);
    metarial.shader.setFloat("heightScale", 0.0f);
    metarial.shader.setFloat("lightIntensity", 100.0f);
    metarial.shader.setMat4("view", camera.GetViewMatrix());
    metarial.shader.setVec3("camPos", camera.Position);
    metarial.shader.setVec3("lightPosition", glm::vec3(0.5f, 2.0f, 1.0f));
    metarial.shader.setVec3("lightColor", glm::vec3(1.0f));
 

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, transform.position);
    model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); 
    model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); 
    model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, transform.scale);
    metarial.shader.setMat4("model", model);

    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)1280 / (float)720,
        0.1f, 100.0f
    );
    metarial.shader.setMat4("projection", projection);

    data.VAO.Bind();
    metarial.Albedo.Bind(0);
    metarial.Normal.Bind(1);
    metarial.Metallic.Bind(2);
    metarial.Roughness.Bind(3);
    metarial.Ao.Bind(4);
    metarial.Height.Bind(5);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERROR << "OpenGL Error: " << err;
    }

    


}



void Renderer::Shutdown()
{
}