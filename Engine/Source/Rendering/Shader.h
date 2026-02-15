#pragma once

#include "GLAD/gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Utils/FileUtils.h"

namespace rv {

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

    inline void setBool(const std::string& name, bool value) const {
        glUniform1i(GetUniformLocation(name), (int)value);
    }
    inline void setInt(const std::string& name, int value) const {
        glUniform1i(GetUniformLocation(name), value);
    }
    inline void setFloat(const std::string& name, float value) const {
        glUniform1f(GetUniformLocation(name), value);
    }
    inline void setVec2(const std::string& name, const glm::vec2& value) const {
        glUniform2f(GetUniformLocation(name), value.x, value.y);
    }
    inline void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
    }
    inline void setVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
    }
    inline void setMat3(const std::string& name, const glm::mat3& value) const {
        glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }
    inline void setMat4(const std::string& name, const glm::mat4& value) const {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }

    GLint GetUniformLocation(const std::string& name) const; 

private:
    bool checkCompileErrors(unsigned int shader, std::string type);
    mutable std::unordered_map<std::string, GLint> uniformLocationCache;
};


}