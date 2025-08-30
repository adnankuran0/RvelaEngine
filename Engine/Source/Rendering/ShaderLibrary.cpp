#include "rvelapch.h"
#if 0
#include "ShaderLibrary.h"

bool ShaderLibrary::Init()
{
    Load("PBR", VRT_PATH("Assets\\Shaders\\pbr.glsl"));
}

void ShaderLibrary::Load(const std::string& name, const Path& path)
{
    if (m_Shaders.contains(name)) {
        LOG_WARN("Shader '{}' already loaded.", name);
        return;
    }

    Ref<Shader> shader = CreateRef<Shader>(path);
    shader->Init(path);

    m_Shaders[name] = shader;
}

void ShaderLibrary::ReloadAll()
{
}

Ref<Shader> ShaderLibrary::Get(const std::string& name)
{
	return Ref<Shader>();
}
#endif