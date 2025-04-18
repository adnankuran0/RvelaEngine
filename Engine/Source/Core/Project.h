#pragma once
#include "Utils/ISerializable.h"

class Project : public ISerializable
{
public:
	std::string name;
	std::string projectPath;
	std::string assetPath;
	std::string lastScenePath;

	Project() = default;
	Project(const std::string& name, const std::string& projectPath)
		: name(name), projectPath(projectPath), assetPath(projectPath + "\\Assets"), lastScenePath("") {
	}

	std::string Serialize() const override;
	void Deserialize(const std::string& jsonStr) override;
};