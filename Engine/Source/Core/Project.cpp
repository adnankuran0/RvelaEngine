#include "rvelapch.h"
#include "Project.h"
#include "json.hpp"
#include "Audio/AudioManager.h"

using namespace rv;
using json = nlohmann::json;

std::string Project::Serialize() const
{
	json j;
	j["name"] = name;
	j["projectPath"] = projectFolderPath.string();

	json s;
	s["name"] = m_Settings.name.empty() ? name : m_Settings.name;
	s["startScene"] = m_Settings.startScene;

	s["viewportWidth"] = m_Settings.viewportWidth;
	s["viewportHeight"] = m_Settings.viewportHeight;

	s["windowWidth"] = m_Settings.windowWidth;
	s["windowHeight"] = m_Settings.windowHeight;
	s["overrideWindowSize"] = m_Settings.overrideWindowSize;
	s["windowOverrideWidth"] = m_Settings.windowOverrideWidth;
	s["windowOverrideHeight"] = m_Settings.windowOverrideHeight;
	s["windowMode"] = static_cast<int>(m_Settings.windowMode);
	s["resizable"] = m_Settings.resizable;
	s["borderless"] = m_Settings.borderless;
	s["vsync"] = m_Settings.vsync;
	s["maxFPS"] = m_Settings.maxFPS;

	s["stretchMode"] = static_cast<int>(m_Settings.stretchMode);
	s["stretchAspect"] = static_cast<int>(m_Settings.stretchAspect);
	s["scalingFilter"] = static_cast<int>(m_Settings.scalingFilter);

	s["msaaSamples"] = m_Settings.msaaSamples;
	s["fxaaEnabled"] = m_Settings.fxaaEnabled;
	s["shadowResolution"] = m_Settings.shadowResolution;
	j["settings"] = s;

	j["busLayout"] = AudioManager::Get().SaveBusLayout();

	return j.dump(4);
}

void Project::Deserialize(const std::string& jsonStr)
{
	json j = json::parse(jsonStr);

	name = j.value("name", "");
	projectFolderPath = std::filesystem::path(j.value("projectPath", ""));

	// default settings
	m_Settings.name = name;
	m_Settings.startScene = "Assets/Scenes/main.rscene";
	m_Settings.viewportWidth = 1920;
	m_Settings.viewportHeight = 1080;
	m_Settings.windowWidth = 1600;
	m_Settings.windowHeight = 900;
	m_Settings.overrideWindowSize = false;
	m_Settings.windowOverrideWidth = 1280;
	m_Settings.windowOverrideHeight = 720;
	m_Settings.windowMode = WindowMode::Windowed;
	m_Settings.resizable = true;
	m_Settings.borderless = false;
	m_Settings.vsync = true;
	m_Settings.stretchMode = StretchMode::Viewport;
	m_Settings.stretchAspect = StretchAspect::Keep;
	m_Settings.msaaSamples = 1;
	m_Settings.fxaaEnabled = true;
	m_Settings.shadowResolution = 2048;

	if (j.contains("settings") && j["settings"].is_object())
	{
		const auto& s = j["settings"];
		m_Settings.name = s.value("name", name);
		m_Settings.startScene = s.value("startScene", "Assets/Scenes/main.rscene");

		uint32_t legacyBaseW = s.value("baseWidth", 1920u);
		uint32_t legacyBaseH = s.value("baseHeight", 1080u);
		m_Settings.viewportWidth = s.value("viewportWidth", legacyBaseW);
		m_Settings.viewportHeight = s.value("viewportHeight", legacyBaseH);

		m_Settings.windowWidth = s.value("windowWidth", 1600u);
		m_Settings.windowHeight = s.value("windowHeight", 900u);

		bool legacyOverride = s.value("overrideViewportResolution", false);
		uint32_t legacyOverW = s.value("overrideWidth", 1280u);
		uint32_t legacyOverH = s.value("overrideHeight", 720u);
		m_Settings.overrideWindowSize = s.value("overrideWindowSize", legacyOverride);
		m_Settings.windowOverrideWidth = s.value("windowOverrideWidth", legacyOverW);
		m_Settings.windowOverrideHeight = s.value("windowOverrideHeight", legacyOverH);

		m_Settings.windowMode = static_cast<WindowMode>(s.value("windowMode", static_cast<int>(WindowMode::Windowed)));
		m_Settings.resizable = s.value("resizable", true);
		m_Settings.borderless = s.value("borderless", false);
		m_Settings.vsync = s.value("vsync", true);
		m_Settings.maxFPS = s.value("maxFPS", 0);

		m_Settings.stretchMode = static_cast<StretchMode>(s.value("stretchMode", static_cast<int>(StretchMode::Viewport)));
		m_Settings.stretchAspect = static_cast<StretchAspect>(s.value("stretchAspect", static_cast<int>(StretchAspect::Keep)));
		m_Settings.scalingFilter = static_cast<ScalingFilter>(s.value("scalingFilter", static_cast<int>(ScalingFilter::Nearest)));

		m_Settings.msaaSamples = s.value("msaaSamples", 1u);
		m_Settings.fxaaEnabled = s.value("fxaaEnabled", true);
		m_Settings.shadowResolution = s.value("shadowResolution", 2048u);
	}

	if (j.contains("busLayout"))
	{
		AudioManager::Get().LoadBusLayout(j["busLayout"].dump());
	}
}
