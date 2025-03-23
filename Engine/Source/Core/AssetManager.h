#pragma once
#include "Scene/Components.h"

struct MeshData
{
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	unsigned int indexCount;
};

class AssetManager
{
public:
	std::vector<MeshData> LoadModel(const std::string& path);
};