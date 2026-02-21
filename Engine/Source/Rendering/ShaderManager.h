#pragma once
#include <string>
#include <vector>
#include "Rendering/Shader.h"

namespace rv {


class ShaderManager
{
public:
	static void Add(Shader&& shader);
	static Shader& Get(const std::string& shaderName);
	std::vector<Shader>& GetAll() { return s_Shaders; }

	static void Reload(const std::string& shaderName);
	static void ReloadAll();

	static void Clear();
private:
	inline static std::vector<Shader> s_Shaders;

};

}