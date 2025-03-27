#pragma once
#include "Scene/Components.h"

class MaterialManager
{
public:
	using MaterialID = uint32_t;

	MaterialID createMaterial(const std::string& name);
	MaterialComponent* getMaterial(MaterialID id);
	MaterialID getMaterialID(const std::string& name);
private:
	std::unordered_map<MaterialID, MaterialComponent*> materials;
	std::unordered_map<std::string, MaterialID> nameToID;
	MaterialID nextID = 1;
};