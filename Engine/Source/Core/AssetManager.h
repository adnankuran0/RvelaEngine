#pragma once
#include "Scene/Components.h"


class AssetManager
{
public:
	void LoadAsset(const std::string& path,ModelComponent& model);
};