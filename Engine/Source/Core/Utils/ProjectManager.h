#pragma once
#include "../Project.h"
#include <filesystem>

#define GET_ABS_PATH(path) ProjectManager::GetAbsolutePath(path).string()
#define GET_VRT_PATH(path) ProjectManager::GetVirtualPath(path).string()

class ProjectManager
{
public:
	ProjectManager() = default;
	~ProjectManager() = default;
	bool CreateProject(const std::string& name, const std::string& path);
	bool LoadProject(const std::string& projectFilePath);
	void SaveActiveProject();

	static std::shared_ptr<Project> GetActiveProject();
	static std::filesystem::path GetAbsolutePath(const std::string& relativePath);
	static std::filesystem::path GetVirtualPath(const std::string& absolutePath);

private:

	ProjectManager(const ProjectManager&) = delete;
	ProjectManager& operator=(const ProjectManager&) = delete;

	static std::shared_ptr<Project> m_ActiveProject;
	static std::filesystem::path m_ProjectFolderPath;


};
