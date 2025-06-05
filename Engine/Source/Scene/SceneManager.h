#pragma once
#include "Scene.h"
#include "Entity.h"

class SceneManager
{
public:
    void CreateScene(Scene& scene, const std::string& path);
    void SaveScene(Scene& scene, const std::string& path);
    void LoadScene(Scene& scene, const std::string& path);
};