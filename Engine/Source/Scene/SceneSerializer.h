#pragma once
#include "Scene.h"

class SceneSerializer
{
public:
	void SaveScene(Scene& scene, const std::string& path);
	void LoadScene(Scene& scene, const std::string& path);

private:
	json SerializeEntity(Scene& scene, entt::entity e);

};