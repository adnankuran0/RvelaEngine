#include "rvelapch.h"
#include "ShaderManager.h"

std::unordered_map<std::string, std::shared_ptr<Shader>> ShaderManager::shaderMap;

std::shared_ptr<Shader> ShaderManager::LoadOrGetShader(const std::string& name, const Path& vertexPath, const Path& fragmentPath)
{
    auto it = shaderMap.find(name);
    if (it != shaderMap.end())
        return it->second;

    auto shader = std::make_shared<Shader>();
    if (!shader->Init(vertexPath, fragmentPath))
    {
        std::cerr << "Shader failed to load: " << name << std::endl;
        return nullptr;
    }

    shaderMap[name] = shader;
    return shader;
}

std::shared_ptr<Shader> ShaderManager::GetShader(const std::string& name)
{
    auto it = shaderMap.find(name);
    if (it != shaderMap.end())
        return it->second;

    std::cerr << "Shader not found: " << name << std::endl;
    return nullptr;
}

void ShaderManager::Clear()
{
    shaderMap.clear();
}
