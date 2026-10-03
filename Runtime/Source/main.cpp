#include "Core/Engine.h"
#include "RuntimeBlitLayer.h"
#include "Core/Log.h"
#include <filesystem>
#include <iostream>

using namespace rv;

static std::filesystem::path FindProjectToRun(int argc, char** argv)
{
	// rproj path
	if (argc > 1)
	{
		std::filesystem::path argPath(argv[1]);
		if (std::filesystem::exists(argPath))
		{
			if (argPath.extension() == ".rproj")
				return argPath;

			if (std::filesystem::is_directory(argPath))
			{
				for (const auto& entry : std::filesystem::directory_iterator(argPath))
				{
					if (entry.path().extension() == ".rproj")
						return entry.path();
				}
			}
		}
	}

	std::filesystem::path currentDir = std::filesystem::current_path();
	for (const auto& entry : std::filesystem::directory_iterator(currentDir))
	{
		if (entry.path().extension() == ".rproj")
			return entry.path();
	}

	auto recents = ProjectManager::GetRecentProjects();
	for (const auto& r : recents)
	{
		if (std::filesystem::exists(r))
			return r;
	}

	std::filesystem::path defaultTest = std::filesystem::path(RVELA_ROOT_DIR) / "Resources" / "Editor" / "TestProject" / "TestProject.rproj";
	if (std::filesystem::exists(defaultTest))
		return defaultTest;

	return "";
}

int main(int argc, char** argv)
{
	rv::Engine engine;

	std::filesystem::path projectPath = FindProjectToRun(argc, argv);
	if (projectPath.empty())
	{
		LOG_ERROR("Runtime: No valid .rproj file found to launch!");
		return -1;
	}

	LOG_INFO("Runtime: Launching project: {}", projectPath.string());
	if (!engine.OpenProject(projectPath.string()))
	{
		LOG_ERROR("Runtime: Failed to open project: {}", projectPath.string());
		return -1;
	}

	auto activeProj = engine.GetProjectManager().GetActiveProject();
	if (activeProj)
	{
		engine.GetWindow().SetTitle(activeProj->name);
	}

	engine.GetActiveScene().SetState(rv::SceneState::PLAY);

	engine.PushLayer(new rv::RuntimeBlitLayer(&engine));

	engine.Run();

	return 0;
}