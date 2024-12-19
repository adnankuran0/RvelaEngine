#include "Renderer.h"

#include "../Resources/cube.h"
#include "../RvelaLog.h"

RendererData Renderer::s_Data;


Renderer::Renderer() 
{
    Init();
};

Renderer::~Renderer()
{
    Shutdown();
}

void Renderer::Init()
{
    LoadShaders();
    SetupBuffers();
}

RendererData& Renderer::GetData()
{
    return s_Data;
}

void Renderer::LoadShaders()
{
    s_Data.m_Shader = new Shader("D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/vertex.glsl", "D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/fragment.glsl");
}

void Renderer::SetupBuffers()
{
    s_Data.m_VAO = new VertexArray();
    s_Data.m_VBO = new VertexBuffer(vertices,sizeof(vertices));
    s_Data.m_Layout = new BufferLayout();
    s_Data.m_Layout->BindVertexBuffer(s_Data.m_VBO->getID());
    s_Data.m_Layout->Push<float>(3);
    s_Data.m_Layout->Push<float>(3);
    s_Data.m_Layout->Push<float>(2);
    s_Data.m_VAO->SetBufferLayout(s_Data.m_Layout);

    s_Data.m_EBO = new ElementBuffer(indices,sizeof(indices));

    
    s_Data.m_Texture = new Texture();
    s_Data.m_Texture->GenerateFromImage("D:/GitHub/RvelaEngine/Engine/Source/Resources/Textures/wall.jpg");
    
    s_Data.m_Camera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f));

}

void Renderer::Render(GLFWwindow* window) {

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.05, 0.0, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    s_Data.m_Shader->use();
    s_Data.m_Shader->setInt("texture1",0);
    s_Data.m_Shader->setMat4("view", s_Data.m_Camera->GetViewMatrix());
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(Time::getTime()*20), glm::vec3(0.5f, 1.0f, 0.0f));
    s_Data.m_Shader->setMat4("model", model);
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)1280 / (float)720,
        0.1f, 100.0f 
    );
    s_Data.m_Shader->setMat4("projection", projection);

    s_Data.m_VAO->Bind();
    s_Data.m_Texture->Bind();
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERROR << "OpenGL Error: " << err;
    }

    glfwSwapBuffers(window);


}



void Renderer::Shutdown()
{
    delete s_Data.m_VAO;
    delete s_Data.m_VBO;
    delete s_Data.m_EBO;
    delete s_Data.m_Shader;
    delete s_Data.m_Layout;
}