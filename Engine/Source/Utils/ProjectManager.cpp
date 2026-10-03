#include "rvelapch.h"
#include "ProjectManager.h"
#include "Utils/Serializer.h"
#include "Utils/FileUtils.h"
#include "json.hpp"
#include <fstream>
#include <algorithm>

using namespace rv;
using json = nlohmann::json;

std::shared_ptr<Project> ProjectManager::m_ActiveProject;
std::filesystem::path ProjectManager::m_ProjectFolderPath;

bool ProjectManager::CreateProject(const std::string& name, const std::string& parentPath)
{
	std::filesystem::path projectRoot = std::filesystem::path(parentPath) / name;
	std::filesystem::path assetsDir = projectRoot / "Assets";
	std::filesystem::path materialsDir = assetsDir / "Materials";
	std::filesystem::path shadersDir = assetsDir / "Shaders";
	std::filesystem::path modelsDir = assetsDir / "Models";
	std::filesystem::path texturesDir = assetsDir / "Textures";
	std::filesystem::path animationsDir = assetsDir / "Animations";
	std::filesystem::path soundsDir = assetsDir / "Sounds";
	std::filesystem::path prefabsDir = assetsDir / "Prefabs";
	std::filesystem::path scriptsDir = assetsDir / "Scripts";
	std::filesystem::path scenesDir = assetsDir / "Scenes";
	std::filesystem::path projectFile = projectRoot / (name + ".rproj");

	std::error_code ec;
	std::filesystem::create_directories(assetsDir, ec);
	std::filesystem::create_directories(materialsDir, ec);
	std::filesystem::create_directories(shadersDir, ec);
	std::filesystem::create_directories(modelsDir, ec);
	std::filesystem::create_directories(texturesDir, ec);
	std::filesystem::create_directories(animationsDir, ec);
	std::filesystem::create_directories(soundsDir, ec);
	std::filesystem::create_directories(prefabsDir, ec);
	std::filesystem::create_directories(scriptsDir, ec);
	std::filesystem::create_directories(scenesDir, ec);

	std::filesystem::path templateAssets = EDITOR_PATH("TemplateProject/Assets").GetAbsolute();
	if (std::filesystem::exists(templateAssets))
	{
		std::filesystem::copy(templateAssets, assetsDir, 
			std::filesystem::copy_options::recursive | std::filesystem::copy_options::skip_existing, ec);
	}

	auto project = std::make_shared<Project>(name, projectRoot.string());
	ProjectSettings settings;
	settings.name = name;
	settings.startScene = "Assets/Scenes/main.rscene";
	project->SetSettings(settings);

	m_ActiveProject = project;
	m_ProjectFolderPath = projectRoot;

	Serializer::SaveToFile(*m_ActiveProject, projectFile.string());
	AddRecentProject(projectFile.string());

	return true;
}

bool ProjectManager::LoadProject(const std::string& projectFilePath)
{
	if (!std::filesystem::exists(projectFilePath))
		return false;

	std::filesystem::path resolvedFolder = std::filesystem::path(projectFilePath).parent_path();
	if (resolvedFolder.filename().string() == "TemplateProject")
	{
		LOG_WARN("Cannot open TemplateProject directly. It is a read-only template!");
		return false;
	}

	auto project = std::make_shared<Project>();
	Serializer::LoadFromFile(*project, projectFilePath);

	if (!std::filesystem::exists(project->projectFolderPath))
	{
		project->projectFolderPath = resolvedFolder;
		m_ActiveProject = project;
		m_ProjectFolderPath = project->projectFolderPath;
		SaveActiveProject();
	}
	else
	{
		m_ActiveProject = project;
		m_ProjectFolderPath = project->projectFolderPath;
	}

	AddRecentProject(projectFilePath);
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

void ProjectManager::CloseProject()
{
	m_ActiveProject.reset();
	m_ProjectFolderPath.clear();
}

std::filesystem::path ProjectManager::GetAssetDirectory()
{
	return m_ProjectFolderPath / ProjectSettings::AssetDirectory;
}

std::filesystem::path ProjectManager::GetCacheDirectory()
{
	return m_ProjectFolderPath / ProjectSettings::CacheDirectory;
}

std::filesystem::path ProjectManager::GetRecentProjectsFilePath()
{
	return EDITOR_PATH("recent_projects.json").GetAbsolute();
}

std::vector<std::string> ProjectManager::GetRecentProjects()
{
	std::vector<std::string> recents;
	std::filesystem::path filePath = GetRecentProjectsFilePath();

	if (!std::filesystem::exists(filePath))
		return recents;

	std::ifstream file(filePath);
	if (!file.is_open())
		return recents;

	try
	{
		json j;
		file >> j;
		if (j.contains("recentProjects") && j["recentProjects"].is_array())
		{
			for (const auto& item : j["recentProjects"])
			{
				if (item.is_string())
				{
					std::string pathStr = item.get<std::string>();
					if (std::filesystem::exists(pathStr))
					{
						std::filesystem::path p(pathStr);
						if (p.parent_path().filename().string() != "TemplateProject")
						{
							recents.push_back(pathStr);
						}
					}
				}
			}
		}
	}
	catch (...)
	{
	}

	return recents;
}

void ProjectManager::AddRecentProject(const std::string& projectFilePath)
{
	std::vector<std::string> recents = GetRecentProjects();
	std::filesystem::path normalized = std::filesystem::path(projectFilePath).lexically_normal();
	std::string normalizedStr = normalized.string();

	recents.erase(std::remove(recents.begin(), recents.end(), normalizedStr), recents.end());
	recents.insert(recents.begin(), normalizedStr);

	constexpr size_t maxRecent = 10;
	if (recents.size() > maxRecent)
	{
		recents.resize(maxRecent);
	}

	json j;
	j["recentProjects"] = recents;

	std::filesystem::path filePath = GetRecentProjectsFilePath();
	std::error_code ec;
	std::filesystem::create_directories(filePath.parent_path(), ec);

	std::ofstream file(filePath);
	if (file.is_open())
	{
		file << j.dump(4);
	}
}

void ProjectManager::ClearRecentProjects()
{
	std::filesystem::path filePath = GetRecentProjectsFilePath();
	std::error_code ec;
	std::filesystem::remove(filePath, ec);
}
