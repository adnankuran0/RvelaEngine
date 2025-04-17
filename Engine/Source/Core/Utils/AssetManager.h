#pragma once
#include "Scene/Components.h"

struct MeshData
{
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	unsigned int indexCount;
	std::string name;
};

class AssetManager
{
public:
	static std::vector<MeshData> LoadModel(const std::string& path);
};