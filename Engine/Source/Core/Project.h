#pragma once
#include "Utils/ISerializable.h"
#include <filesystem>

class Project : public ISerializable
{
public:
	std::string name;
	std::filesystem::path projectFolderPath;

	Project() = default;
	Project(const std::string& name, const std::string& projectFolderPath)
		: name(name)
	{

		this->projectFolderPath = std::filesystem::path(projectFolderPath) / name;
		
	}

	std::string Serialize() const override;
	void Deserialize(const std::string& jsonStr) override;
};