#pragma once
#if 0
#include <string>
#include <filesystem>
#include <unordered_map>
#include "Core/Ref.h"
#include "Shader.h"
class ShaderLibrary {
public:
    bool Init();
    void Load(const std::string& name, const Path& path);
    void ReloadAll();
    Ref<Shader> Get(const std::string& name);
private:
    std::unordered_map<std::string, Ref<Shader>> m_Shaders;
};
#endif