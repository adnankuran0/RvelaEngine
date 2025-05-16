#pragma once
#include "../Project.h"
#include <filesystem>


class ProjectManager
{
public:
	ProjectManager() = default;
	~ProjectManager() = default;
	bool CreateProject(const std::string& name, const std::string& path);
	bool LoadProject(const std::string& projectFilePath);
	void SaveActiveProject();

	static std::shared_ptr<Project> GetActiveProject();
	static std::filesystem::path GetProjectPath() { return m_ProjectFolderPath; }

private:

	ProjectManager(const ProjectManager&) = delete;
	ProjectManager& operator=(const ProjectManager&) = delete;

	static std::shared_ptr<Project> m_ActiveProject;
	static std::filesystem::path m_ProjectFolderPath;


};
