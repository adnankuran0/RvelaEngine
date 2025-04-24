#pragma once

#include "GL/glew.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Shader.h"

class Skybox
{
public:
    Skybox();
    ~Skybox();
    void Init(const std::vector<std::string>& faces);
    void Render(Shader& shader, const glm::mat4& projection, const glm::mat4& view);

private:
    GLuint skyboxVAO, skyboxVBO;
    GLuint skyboxTexture;

    GLuint loadCubemap(const std::vector<std::string>& faces);
    void setupSkybox();
};
