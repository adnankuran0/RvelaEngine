#include "Core/Engine.h"
#include "RuntimeBlitLayer.h"
#include "Core/Log.h"
#include "Utils/FileUtils.h"
#include <filesystem>
#include <iostream>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static void ShowFatalError(const std::string& title, const std::string& message)
{
	MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}
#else
static void ShowFatalError(const std::string& title, const std::string& message)
{
	std::cerr << "[" << title << "] " << message << std::endl;
}
#endif

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

	std::filesystem::path exeDir = Path::GetExecutableDirectory();
	if (std::filesystem::exists(exeDir))
	{
		for (const auto& entry : std::filesystem::directory_iterator(exeDir))
		{
			if (entry.path().extension() == ".rproj")
				return entry.path();
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

#if defined(RVELA_ROOT_DIR)
	std::filesystem::path defaultTest = std::filesystem::path(RVELA_ROOT_DIR) / "Resources" / "Editor" / "TestProject" / "TestProject.rproj";
	if (std::filesystem::exists(defaultTest))
		return defaultTest;
#endif

	return "";
}

int main(int argc, char** argv)
{
	rv::Engine engine;

	std::filesystem::path projectPath = FindProjectToRun(argc, argv);
	if (projectPath.empty())
	{
		const std::string err = "No valid .rproj project file found to launch!\n\nPlease place your .rproj project in the executable directory or specify it as a command line argument.";
		LOG_ERROR("Runtime: {}", err);
		ShowFatalError("Rvela Runtime - Project Not Found", err);
		return -1;
	}

	LOG_INFO("Runtime: Launching project: {}", projectPath.string());
	if (!engine.OpenProject(projectPath.string()))
	{
		const std::string err = "Failed to open project: " + projectPath.string() + "\n\n";
		LOG_ERROR("Runtime: {}", err);
		ShowFatalError("Rvela Runtime - Load Error", err);
		return -1;
	}

	auto activeProj = engine.GetProjectManager().GetActiveProject();
	if (activeProj)
	{
		engine.GetWindow().SetTitle(activeProj->name);
		engine.GetWindow().ApplySettings(activeProj->GetSettings());
	}

	engine.GetActiveScene().SetState(rv::SceneState::PLAY);

	engine.PushLayer(new rv::RuntimeBlitLayer(&engine));

	try
	{
		engine.Run();
	}
	catch (const std::exception& ex)
	{
		LOG_CRITICAL("Runtime unhandled exception: {}", ex.what());
		ShowFatalError("Rvela Runtime Crash", std::string("An unhandled exception occurred:\n") + ex.what());
		return -1;
	}
	catch (...)
	{
		LOG_CRITICAL("Runtime unhandled unknown exception");
		ShowFatalError("Rvela Runtime Crash", "An unexpected fatal error occurred.");
		return -1;
	}

	return 0;
}
