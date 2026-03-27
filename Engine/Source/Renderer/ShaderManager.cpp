#include "rvelapch.h"
#include "ShaderManager.h"

using namespace rv;

void ShaderManager::Add(Shader&& shader)
{
	s_Shaders.push_back(std::move(shader));
}

Shader& ShaderManager::Get(const std::string& shaderName)
{
	for (auto& shader : s_Shaders)
	{
		if (shader.GetName() == shaderName)
			return shader;
	}
    //TODO: return default shader
    LOG_ERROR("Shader {} not found.", shaderName);
    assert(false);
}

void ShaderManager::Reload(const std::string& name)
{
    for (auto& shader : s_Shaders)
    {
        if (shader.GetName() == name)
        {
            if (!shader.Recompile())
                LOG_WARN("Shader reload failed. Keeping old program.");
        }
    }

    LOG_ERROR("Shader {} not found.", name);
    assert(false);
}

void ShaderManager::ReloadAll()
{
    for (auto& shader : s_Shaders)
    {
        if (!shader.Recompile())
            LOG_WARN("Shader reload failed for '{}'. Keeping old program.", shader.GetName());
    }
}

void ShaderManager::Clear()
{
	s_Shaders.clear();
}
