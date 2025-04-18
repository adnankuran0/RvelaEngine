#include "rvelapch.h"
#include "ProjectManager.h"
#include "Core/Utils/Serializer.h"
#include <filesystem>

std::shared_ptr<Project> ProjectManager::m_ActiveProject;


bool ProjectManager::CreateProject(const std::string& name, const std::string& path)
{
	std::filesystem::create_directories(path +"\\" + name + "\\Assets");
	std::filesystem::create_directories(path +"\\" + name + "\\Scenes");

	m_ActiveProject = std::make_shared<Project>(name, path);
	Serializer::SaveToFile(*m_ActiveProject, path + "\\" + name + "\\" + name + ".rproj");
	return true;
}

bool ProjectManager::LoadProject(const std::string& projectFilePath)
{
	if (!std::filesystem::exists(projectFilePath))
		return false;

	auto project = std::make_shared<Project>();
	Serializer::LoadFromFile(*project, projectFilePath);
	m_ActiveProject = project;
	return true;
}

void ProjectManager::SaveActiveProject()
{
	if (m_ActiveProject)
	{
		std::string file = m_ActiveProject->projectPath + "/" + m_ActiveProject->name + ".rproj";
		Serializer::SaveToFile(*m_ActiveProject, file);
	}
}

std::shared_ptr<Project> ProjectManager::GetActiveProject()
{
	return m_ActiveProject;
}
