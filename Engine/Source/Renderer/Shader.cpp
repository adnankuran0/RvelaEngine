#include "rvelapch.h"
#include "Shader.h"
#include "Core/Log.h"

using namespace rv;

Shader::Shader(const std::string& name, const Path& shaderPath)
{
    Init(name,shaderPath);
}

Shader::Shader(Shader&& other) noexcept
{
    ID = other.ID;
    m_Name = std::move(other.m_Name);
    m_Path = std::move(other.m_Path);
    uniformLocationCache = std::move(other.uniformLocationCache);

    other.ID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        Destroy();

        ID = other.ID;
        m_Name = std::move(other.m_Name);
        m_Path = std::move(other.m_Path);
        uniformLocationCache = std::move(other.uniformLocationCache);

        other.ID = 0;
    }
    return *this;
}

bool Shader::Init(const std::string& name, const Path& shaderPath)
{
    m_Path = shaderPath;
    m_Name = name;

    GLuint program = 0;
    if (!CompileInternal(shaderPath, program))
        return false;

    ID = program;
    return true;
}

void Shader::Destroy()
{
    if (ID != 0)
    {
        glDeleteProgram(ID);
        ID = 0;
    }
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

bool Shader::Recompile()
{
    GLuint newProgram = 0;

    if (!CompileInternal(m_Path, newProgram))
        return false;

    glDeleteProgram(ID);
    ID = newProgram;
    uniformLocationCache.clear();
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

std::string Shader::ProcessIncludes(const std::string& source, const Path& shaderPath, std::unordered_set<std::string>& includedPaths)
{
    std::istringstream stream(source);
    std::stringstream result;
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.find("#include") != std::string::npos)
        {
            size_t start = line.find_first_of("\"<");
            size_t end = line.find_last_of("\">");
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string includeFilename = line.substr(start + 1, end - start - 1);
                Path includePath = shaderPath.GetParentPath() / includeFilename;
                std::string absPathStr = includePath.GetAbsoluteStr();

                if (includedPaths.find(absPathStr) == includedPaths.end())
                {
                    includedPaths.insert(absPathStr);

                    std::ifstream includeFile(includePath.GetAbsolute());
                    if (includeFile.is_open())
                    {
                        std::stringstream incBuffer;
                        incBuffer << includeFile.rdbuf();

                        result << "// [BEGIN INCLUDE: " << includeFilename << "]\n";
                        result << ProcessIncludes(incBuffer.str(), includePath, includedPaths) << "\n";
                        result << "// [END INCLUDE: " << includeFilename << "]\n";
                    }
                    else
                    {
                        LOG_ERROR("Can't open shader include file: {}", absPathStr);
                    }
                }
                continue;
            }
        }
        result << line << "\n";
    }

    return result.str();
}

bool Shader::CompileInternal(const Path& path, GLuint& outProgram)
{
    std::ifstream shaderFile(path.GetAbsolute());
    if (!shaderFile.is_open())
    {
        LOG_ERROR("ERROR::SHADER::FILE_NOT_OPENED: {}", path.GetAbsolute().string());
        return false;
    }

    std::stringstream buffer;
    buffer << shaderFile.rdbuf();

    std::unordered_set<std::string> includedPaths;
    includedPaths.insert(path.GetAbsoluteStr());
    std::string source = ProcessIncludes(buffer.str(), path, includedPaths);

    std::unordered_map<std::string, std::string> shaderSources;
    std::string currentType;
    std::stringstream currentSource;

    std::istringstream stream(source);
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (!currentType.empty())
            {
                shaderSources[currentType] = currentSource.str();
                currentSource.str("");
                currentSource.clear();
            }

            currentType = line.substr(line.find("#shader") + 7);
            currentType.erase(0, currentType.find_first_not_of(" \t"));
            currentType.erase(currentType.find_last_not_of(" \t\r\n") + 1);
        }
        else
        {
            currentSource << line << '\n';
        }
    }

    if (!currentType.empty())
        shaderSources[currentType] = currentSource.str();

    std::vector<GLuint> compiledShaders;

    for (const auto& [type, src] : shaderSources)
    {
        GLenum shaderType = 0;

        if (type == "vertex")        shaderType = GL_VERTEX_SHADER;
        else if (type == "fragment") shaderType = GL_FRAGMENT_SHADER;
        else if (type == "geometry") shaderType = GL_GEOMETRY_SHADER;
        else if (type == "compute")  shaderType = GL_COMPUTE_SHADER;
        else
        {
            LOG_ERROR("ERROR::SHADER::UNKNOWN_SHADER_TYPE: {}", type);
            continue;
        }

        GLuint shader = glCreateShader(shaderType);
        const char* code = src.c_str();
        glShaderSource(shader, 1, &code, nullptr);
        glCompileShader(shader);

        if (!checkCompileErrors(shader, type))
        {
            glDeleteShader(shader);
            for (auto s : compiledShaders)
                glDeleteShader(s);
            return false;
        }

        compiledShaders.push_back(shader);
    }

    GLuint program = glCreateProgram();

    for (auto shader : compiledShaders)
        glAttachShader(program, shader);

    glLinkProgram(program);

    if (!checkCompileErrors(program, "PROGRAM"))
    {
        glDeleteProgram(program);
        for (auto s : compiledShaders)
            glDeleteShader(s);
        return false;
    }

    for (auto s : compiledShaders)
        glDeleteShader(s);

    outProgram = program;
    return true;
}

