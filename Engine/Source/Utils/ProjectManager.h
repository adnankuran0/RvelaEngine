#pragma once
#include "Core/Project.h"
#include <filesystem>
#include <vector>
#include <string>
#include <memory>

namespace rv {

class ProjectManager
{
public:
	ProjectManager() = default;
	~ProjectManager() = default;

	static bool CreateProject(const std::string& name, const std::string& parentPath);
	static bool LoadProject(const std::string& projectFilePath);
	static void SaveActiveProject();
	static void CloseProject();

	static bool IsProjectLoaded() { return m_ActiveProject != nullptr; }
	static std::shared_ptr<Project> GetActiveProject() { return m_ActiveProject; }
	static std::filesystem::path GetProjectPath() { return m_ProjectFolderPath; }

	static std::filesystem::path GetAssetDirectory();
	static std::filesystem::path GetCacheDirectory();

	static std::vector<std::string> GetRecentProjects();
	static void AddRecentProject(const std::string& projectFilePath);
	static void RemoveRecentProject(const std::string& projectFilePath);
	static void ClearRecentProjects();

private:
	ProjectManager(const ProjectManager&) = delete;
	ProjectManager& operator=(const ProjectManager&) = delete;

	static std::filesystem::path GetRecentProjectsFilePath();

	static std::shared_ptr<Project> m_ActiveProject;
	static std::filesystem::path m_ProjectFolderPath;
};

}