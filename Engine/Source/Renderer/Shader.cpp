#include "rvelapch.h"
#include "Shader.h"
#include "Core/Log.h"

using namespace rv;

Shader::Shader(const std::string& name, const Path& shaderPath, const std::vector<std::string>& enabledDefines)
{
    Init(name, shaderPath, enabledDefines);
}

Shader::Shader(Shader&& other) noexcept
{
    ID = other.ID;
    m_Name = std::move(other.m_Name);
    m_Path = std::move(other.m_Path);
    m_ActiveDefines = std::move(other.m_ActiveDefines);
    m_DiscoveredDefines = std::move(other.m_DiscoveredDefines);
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
        m_ActiveDefines = std::move(other.m_ActiveDefines);
        m_DiscoveredDefines = std::move(other.m_DiscoveredDefines);
        uniformLocationCache = std::move(other.uniformLocationCache);

        other.ID = 0;
    }
    return *this;
}

bool Shader::Init(const std::string& name, const Path& shaderPath, const std::vector<std::string>& enabledDefines)
{
    m_Path = shaderPath;
    m_Name = name;
    m_ActiveDefines = std::unordered_set<std::string>(enabledDefines.begin(), enabledDefines.end());

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

void Shader::wait() const
{
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

bool Shader::checkCompileErrors(unsigned int shader, const std::string& type)
{
    int success;
    char infoLog[1024];

    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            LOG_ERROR("ERROR::SHADER_COMPILATION_ERROR of type: {} \n {}", type, infoLog);
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

GLint Shader::GetUniformLocation(const std::string& name) const
{
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
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        size_t includePos = line.find("#include");
        if (includePos != std::string::npos)
        {
            size_t start = line.find_first_of("\"<", includePos);
            size_t end = line.find_last_of("\">");
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string includeFilename = line.substr(start + 1, end - start - 1);

                Path includePath = shaderPath.GetParentPath() / includeFilename;

                if (!includePath.Exists())
                {
                    includePath = Path::FromVirtual(std::string("Assets/Shaders/") + includeFilename);
                }

                std::string canonicalPathStr = std::filesystem::weakly_canonical(includePath.GetAbsolute()).string();

                if (includedPaths.find(canonicalPathStr) == includedPaths.end())
                {
                    includedPaths.insert(canonicalPathStr);

                    std::ifstream includeFile(canonicalPathStr);
                    if (includeFile.is_open())
                    {
                        std::stringstream incBuffer;
                        incBuffer << includeFile.rdbuf();

                        result << ProcessIncludes(incBuffer.str(), includePath, includedPaths) << "\n";
                    }
                    else
                    {
                        LOG_ERROR("Can't open shader include file: {}", canonicalPathStr);
                    }
                }
                continue;
            }
        }
        result << line << "\n";
    }

    return result.str();
}

void Shader::ParseDefines(const std::string& source)
{
    std::istringstream stream(source);
    std::string line;

    while (std::getline(stream, line))
    {
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos)
            line = line.substr(0, commentPos);

        size_t ifdefPos = line.find("#ifdef");
        size_t ifndefPos = line.find("#ifndef");
        size_t targetPos = std::string::npos;
        size_t offset = 0;

        if (ifdefPos != std::string::npos)
        {
            targetPos = ifdefPos;
            offset = 6;
        }
        else if (ifndefPos != std::string::npos)
        {
            targetPos = ifndefPos;
            offset = 7;
        }

        if (targetPos != std::string::npos)
        {
            std::string def = line.substr(targetPos + offset);
            size_t first = def.find_first_not_of(" \t\r\n");
            size_t last = def.find_last_not_of(" \t\r\n");
            if (first != std::string::npos && last != std::string::npos)
            {
                std::string defineName = def.substr(first, (last - first + 1));
                if (defineName.rfind("GL_", 0) != 0)
                {
                    m_DiscoveredDefines.insert(defineName);
                }
            }
        }
    }
}

std::string Shader::InjectActiveDefines(const std::string& source) const
{
    if (m_ActiveDefines.empty())
        return source;

    std::stringstream defStream;
    for (const auto& def : m_ActiveDefines)
    {
        defStream << "#define " << def << "\n";
    }

    std::string injectStr = defStream.str();
    size_t versionPos = source.find("#version");

    if (versionPos != std::string::npos)
    {
        size_t nextLine = source.find('\n', versionPos);
        if (nextLine != std::string::npos)
        {
            std::string result = source;
            result.insert(nextLine + 1, injectStr);
            return result;
        }
    }

    return injectStr + source;
}

void Shader::EnableDefine(const std::string& define)
{
    if (m_ActiveDefines.find(define) == m_ActiveDefines.end())
    {
        m_ActiveDefines.insert(define);
        Recompile();
    }
}

void Shader::DisableDefine(const std::string& define)
{
    if (m_ActiveDefines.erase(define) > 0)
    {
        Recompile();
    }
}

bool Shader::IsDefineEnabled(const std::string& define) const
{
    return m_ActiveDefines.find(define) != m_ActiveDefines.end();
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

    std::unordered_map<std::string, std::string> rawShaderSources;
    std::string currentType;
    std::stringstream currentSource;

    std::istringstream stream(buffer.str());
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (!currentType.empty())
            {
                rawShaderSources[currentType] = currentSource.str();
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
        rawShaderSources[currentType] = currentSource.str();

    std::vector<GLuint> compiledShaders;

    for (const auto& [type, rawSrc] : rawShaderSources)
    {
        std::string src = InjectActiveDefines(rawSrc);

        std::unordered_set<std::string> includedPaths;
        includedPaths.insert(path.GetAbsoluteStr());
        src = ProcessIncludes(src, path, includedPaths);

        ParseDefines(src);

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