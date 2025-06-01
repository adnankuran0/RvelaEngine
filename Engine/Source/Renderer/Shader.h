#pragma once

#include "GL/glew.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Core/Utils/FileUtils.h"


class Shader
{
public:
    unsigned int ID;
    Shader(const Path& vertexPath, const Path& fragmentPath);
    Shader(const Path& vertexPath, const Path& fragmentPath, const Path& geometryPath);
    Shader(const Path& computePath);
    Shader() = default;

    bool Init(const Path& vertexPath, const Path& fragmentPath);
    bool Init(const Path& vertexPath, const Path& fragmentPath, const Path& geometryPath);
    bool Init(const Path& computePath);

    void Destroy();


    void use();
    void dispatch(unsigned int x, unsigned int y = 1, unsigned int z = 1) const;
    void wait() const;

    void setBool(const std::string& name, bool value) const;

    void setInt(const std::string& name, int value) const;

    void setFloat(const std::string& name, float value) const;

    void setVec2(const std::string& name, const glm::vec2& value) const;

    void setVec3(const std::string& name, const glm::vec3& value) const;

    void setVec4(const std::string& name, const glm::vec4& value) const;

    void setMat3(const std::string& name, const glm::mat3& value) const;

    void setMat4(const std::string& name, const glm::mat4& value) const;


private:
    bool checkCompileErrors(unsigned int shader, std::string type);
  
};
