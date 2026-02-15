#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "GLAD/gl.h"
#include "Utils/FileUtils.h"
#include "Shader.h"

namespace rv { 

class Skybox
{
public:
    Skybox();
    ~Skybox();
    void Init(const std::vector<Path>& faces);
    void Render(Shader& shader, const glm::mat4& projection, const glm::mat4& view,GLuint screenFBO);
    GLuint ConvertEquirectangularToCubemap(GLuint hdrTexture, Shader& shader);
    GLuint LoadHDRTexture(const Path& hdrPath);
    void InitHDR(const Path& hdrFilePath, Shader& conversionShader);
    inline GLuint GetTextureID() { return skyboxTexture; }
private:
    GLuint skyboxVAO, skyboxVBO;
    GLuint skyboxTexture;

    GLuint loadCubemap(const std::vector<Path>& faces);
    void setupSkybox();
};

}