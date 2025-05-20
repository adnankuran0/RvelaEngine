#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "Renderer/Shader.h"

class ShaderManager
{
public:
    static std::shared_ptr<Shader> LoadOrGetShader(const std::string& name, const Path& vertexPath, const Path& fragmentPath);
    static std::shared_ptr<Shader> GetShader(const std::string& name);
    static void Clear();

private:
    static std::unordered_map<std::string, std::shared_ptr<Shader>> shaderMap;
};
