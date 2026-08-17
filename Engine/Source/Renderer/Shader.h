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
    Shader(const std::string& name, const Path& shaderPath);
    Shader() = default;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;
    ~Shader() { Destroy(); }

    bool Init(const std::string& name, const Path& shaderPath);
    bool Recompile();
    void Destroy();

    void use();
    void dispatch(unsigned int x, unsigned int y = 1, unsigned int z = 1) const;
    void wait() const;

    inline const std::string& GetName() { return m_Name; }
    inline const Path& GetPath() { return m_Path; }

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
    inline void setUInt(const std::string& name, unsigned int value) const {
        glUniform1ui(GetUniformLocation(name), value);
    }

    GLint GetUniformLocation(const std::string& name) const; 

private:
    std::string m_Name;
    Path m_Path;
    bool checkCompileErrors(unsigned int shader, std::string type);
    bool CompileInternal(const Path& path, GLuint& outProgram);
    static std::string ProcessIncludes(const std::string& source, const Path& shaderPath, std::unordered_set<std::string>& includedPaths);
    mutable std::unordered_map<std::string, GLint> uniformLocationCache;
};


}