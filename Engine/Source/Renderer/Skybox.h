#pragma once

#include "GLAD/gl.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Shader.h"

class Skybox
{
public:
    Skybox();
    ~Skybox();
    void Init(const std::vector<Path>& faces);
    void Render(Shader& shader, const glm::mat4& projection, const glm::mat4& view,GLuint screenFBO);

private:
    GLuint skyboxVAO, skyboxVBO;
    GLuint skyboxTexture;

    GLuint loadCubemap(const std::vector<Path>& faces);
    void setupSkybox();
};
