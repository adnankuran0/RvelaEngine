#include "rvelapch.h"
#include "ProjectManager.h"
#include "Utils/Serializer.h"

using namespace rv;

std::shared_ptr<Project> ProjectManager::m_ActiveProject;
std::filesystem::path ProjectManager::m_ProjectFolderPath;


bool ProjectManager::CreateProject(const std::string& name, const std::string& path)
{
	std::filesystem::path projectRoot = std::filesystem::path(path) / name;
	std::filesystem::path assetsDir = projectRoot / "Assets";
	std::filesystem::path materialsDir = assetsDir / "Materials";
	std::filesystem::path shadersDir = assetsDir / "Shaders";
	std::filesystem::path modelsDir = assetsDir / "Models";
	std::filesystem::path texturesDir = assetsDir / "Textures";
	std::filesystem::path scenesDir = projectRoot / "Scenes";
	std::filesystem::path projectFile = projectRoot / (name + ".rproj");

	std::filesystem::create_directories(assetsDir);
	std::filesystem::create_directories(materialsDir);
	std::filesystem::create_directories(shadersDir);
	std::filesystem::create_directories(modelsDir);
	std::filesystem::create_directories(texturesDir);
	std::filesystem::create_directories(scenesDir);

	m_ActiveProject = std::make_shared<Project>(name, projectRoot.string());
	Serializer::SaveToFile(*m_ActiveProject, projectFile.string());

	return true;
}

bool ProjectManager::LoadProject(const std::string& projectFilePath)
{
	if (!std::filesystem::exists(projectFilePath))
		return false;

	auto project = std::make_shared<Project>();
	Serializer::LoadFromFile(*project, projectFilePath);
	m_ActiveProject = project;
	m_ProjectFolderPath = project->projectFolderPath;
	return true;
}

void ProjectManager::SaveActiveProject()
{
	if (m_ActiveProject)
	{
		std::filesystem::path filePath = m_ActiveProject->projectFolderPath / (m_ActiveProject->name + ".rproj");
		Serializer::SaveToFile(*m_ActiveProject, filePath.string());
	}
}

std::shared_ptr<Project> ProjectManager::GetActiveProject()
{
	return m_ActiveProject;
}
