#include "Core/Engine.h"
#include "RuntimeBlitLayer.h"
#include "Core/Log.h"
#include "Core/VFS.h"
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
	std::filesystem::path pakPath;

	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if ((arg == "--pak" || arg == "--rpak") && i + 1 < argc)
		{
			pakPath = argv[++i];
			break;
		}
		else if (arg.ends_with(".rpak") || arg.ends_with(".pak"))
		{
			pakPath = arg;
			break;
		}
	}

	std::filesystem::path exeDir = Path::GetExecutableDirectory();
	if (pakPath.empty() && std::filesystem::exists(exeDir))
	{
#if defined(_WIN32)
		char exePathBuf[MAX_PATH];
		if (GetModuleFileNameA(nullptr, exePathBuf, MAX_PATH) > 0)
		{
			std::filesystem::path exeFile(exePathBuf);
			std::filesystem::path matchingRpak = exeDir / (exeFile.stem().string() + ".rpak");
			std::filesystem::path matchingPak = exeDir / (exeFile.stem().string() + ".pak");
			if (std::filesystem::exists(matchingRpak))
			{
				pakPath = matchingRpak;
			}
			else if (std::filesystem::exists(matchingPak))
			{
				pakPath = matchingPak;
			}
		}
#endif
		if (pakPath.empty())
		{
			if (std::filesystem::exists(exeDir / "Game.rpak"))
				pakPath = exeDir / "Game.rpak";
			else if (std::filesystem::exists(exeDir / "Game.pak"))
				pakPath = exeDir / "Game.pak";
		}

		if (pakPath.empty())
		{
			for (const auto& entry : std::filesystem::directory_iterator(exeDir))
			{
				if (entry.path().extension() == ".rpak" || entry.path().extension() == ".pak")
				{
					pakPath = entry.path();
					break;
				}
			}
		}
	}

	std::filesystem::path currentDir = std::filesystem::current_path();
	if (pakPath.empty())
	{
		if (std::filesystem::exists(currentDir / "Game.rpak"))
			pakPath = currentDir / "Game.rpak";
		else if (std::filesystem::exists(currentDir / "Game.pak"))
			pakPath = currentDir / "Game.pak";
	}

	if (!pakPath.empty() && std::filesystem::exists(pakPath))
	{
		if (VFS::Get().Mount(pakPath.string()))
		{
			auto files = VFS::Get().ListFiles();
			for (const auto& f : files)
			{
				if (f.ends_with(".rproj"))
				{
					return f;
				}
			}
			LOG_ERROR("Runtime: Mounted PAK does not contain a .rproj project file!");
		}
		else
		{
			LOG_ERROR("Runtime: Failed to mount PAK: {}", pakPath.string());
		}
	}

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

	if (std::filesystem::exists(exeDir))
	{
		for (const auto& entry : std::filesystem::directory_iterator(exeDir))
		{
			if (entry.path().extension() == ".rproj")
				return entry.path();
		}
	}

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
	RvelaLog::Init("log.txt");
	std::filesystem::path projectPath = FindProjectToRun(argc, argv);
	if (projectPath.empty())
	{
		const std::string err = "No valid .rproj project or .rpak archive found to launch!\n\nPlease place your .rpak archive in the executable directory or specify it as a command line argument.";
		LOG_ERROR("Runtime: {}", err);
		ShowFatalError("Rvela Runtime - Project Not Found", err);
		return -1;
	}

	rv::Engine engine;

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
		ShowFatalError("Rvela Runtime Crash", std::string("An unhandled exception occurred:\n") + ex.what());
		return -1;
	}
	catch (...)
	{
		ShowFatalError("Rvela Runtime Crash", "An unexpected fatal error occurred.");
		return -1;
	}

	return 0;
}
