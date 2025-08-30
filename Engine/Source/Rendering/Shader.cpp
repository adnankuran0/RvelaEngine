#include "rvelapch.h"
#include "Shader.h"
#include "Core/Log.h"

Shader::Shader(const Path& shaderPath)
{
    Init(shaderPath);
}

bool Shader::Init(const Path& shaderPath)
{
    std::ifstream shaderFile(shaderPath.GetAbsolute());
    if (!shaderFile.is_open()) {
        LOG_ERROR("ERROR::SHADER::FILE_NOT_OPENED");
        return false;
    }

    std::stringstream buffer;
    buffer << shaderFile.rdbuf();
    std::string source = buffer.str();

    std::unordered_map<std::string, std::string> shaderSources;
    std::string currentType;
    std::stringstream currentSource;

    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("#shader") != std::string::npos) {
            if (!currentType.empty()) {
                shaderSources[currentType] = currentSource.str();
                currentSource.str("");
                currentSource.clear();
            }
            currentType = line.substr(line.find("#shader") + 7);
            currentType.erase(0, currentType.find_first_not_of(" \t"));
        }
        else {
            currentSource << line << '\n';
        }
    }
    if (!currentType.empty()) {
        shaderSources[currentType] = currentSource.str();
    }

    std::vector<unsigned int> compiledShaders;

    for (const auto& [type, src] : shaderSources) {
        GLenum shaderType;
        if (type == "vertex") shaderType = GL_VERTEX_SHADER;
        else if (type == "fragment") shaderType = GL_FRAGMENT_SHADER;
        else if (type == "geometry") shaderType = GL_GEOMETRY_SHADER;
        else if (type == "compute") shaderType = GL_COMPUTE_SHADER;
        else {
            LOG_ERROR("ERROR::SHADER::UNKNOWN_SHADER_TYPE: {}", type);
            continue;
        }

        const char* code = src.c_str();
        unsigned int shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &code, nullptr);
        glCompileShader(shader);

        if (!checkCompileErrors(shader, type)) {
            for (auto id : compiledShaders)
                glDeleteShader(id);
            return false;
        }

        compiledShaders.push_back(shader);
    }

    ID = glCreateProgram();
    for (auto shader : compiledShaders)
        glAttachShader(ID, shader);
    glLinkProgram(ID);

    if (!checkCompileErrors(ID, "PROGRAM")) {
        for (auto shader : compiledShaders)
            glDeleteShader(shader);
        return false;
    }

    for (auto shader : compiledShaders)
        glDeleteShader(shader);

    return true;
}

void Shader::Destroy()
{
    glDeleteProgram(ID);
}

void Shader::use()
{
    glUseProgram(ID);
}

void Shader::dispatch(unsigned int x, unsigned int y, unsigned int z) const
{
    glUseProgram(ID);
    glDispatchCompute(x, y, z);
}

void Shader::wait() const {
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

bool Shader::checkCompileErrors(unsigned int shader, const std::string type)
{
    int success;
    char infoLog[1024];

    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            LOG_ERROR("ERROR::SHADER_COMPILATION_ERROR of type: {} \n {}" ,type , infoLog);
            return false;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            LOG_ERROR("ERROR::PROGRAM_LINKING_ERROR of type: {} \n {}", type, infoLog);
            return false;
        }
    }

    return true;
}


GLint Shader::GetUniformLocation(const std::string& name) const {
    auto it = uniformLocationCache.find(name);
    if (it != uniformLocationCache.end())
        return it->second;

    GLint location = glGetUniformLocation(ID, name.c_str());
    uniformLocationCache[name] = location;
    return location;
}