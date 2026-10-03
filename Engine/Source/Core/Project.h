#pragma once
#include "Utils/ISerializable.h"
#include "ProjectSettings.h"
#include <filesystem>

namespace rv { 

class Project : public ISerializable
{
public:
	std::string name;
	std::filesystem::path projectFolderPath;

	Project() = default;
	Project(const std::string& name, const std::string& projectFolderPath)
		: name(name)
	{
		this->projectFolderPath = std::filesystem::path(projectFolderPath);
		m_Settings.name = name;
	}

	const ProjectSettings& GetSettings() const { return m_Settings; }
	ProjectSettings& GetSettings() { return m_Settings; }
	void SetSettings(const ProjectSettings& settings) { m_Settings = settings; }

	std::string Serialize() const override;
	void Deserialize(const std::string& jsonStr) override;

private:
	ProjectSettings m_Settings;
};

}