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
	s["iconPath"] = m_Settings.iconPath;
	s["cursorIconPath"] = m_Settings.cursorIconPath;
	s["cursorHotspotX"] = m_Settings.cursorHotspotX;
	s["cursorHotspotY"] = m_Settings.cursorHotspotY;

	s["stretchMode"] = static_cast<int>(m_Settings.stretchMode);
	s["stretchAspect"] = static_cast<int>(m_Settings.stretchAspect);
	s["scalingFilter"] = static_cast<int>(m_Settings.scalingFilter);

	s["msaaSamples"] = m_Settings.msaaSamples;
	s["fxaaEnabled"] = m_Settings.fxaaEnabled;
	s["shadowResolution"] = m_Settings.shadowResolution;
	s["pointShadowResolution"] = m_Settings.pointShadowResolution;
	s["spotShadowResolution"] = m_Settings.spotShadowResolution;

	s["ssaoHalfResolution"] = m_Settings.ssaoHalfResolution;
	s["ssaoSliceCount"] = m_Settings.ssaoSliceCount;
	s["ssaoStepCount"] = m_Settings.ssaoStepCount;

	s["contactShadowsHalfResolution"] = m_Settings.contactShadowsHalfResolution;
	s["contactShadowsSteps"] = m_Settings.contactShadowsSteps;

	s["ssrHalfResolution"] = m_Settings.ssrHalfResolution;
	s["ssrMaxSteps"] = m_Settings.ssrMaxSteps;

	s["ssgiHalfResolution"] = m_Settings.ssgiHalfResolution;
	s["ssgiSteps"] = m_Settings.ssgiSteps;

	s["volumetricFogHalfResolution"] = m_Settings.volumetricFogHalfResolution;
	s["volumetricFogSteps"] = m_Settings.volumetricFogSteps;

	s["motionBlurSamples"] = m_Settings.motionBlurSamples;

	s["defaultTextureFilter"] = static_cast<int>(m_Settings.defaultTextureFilter);
	s["defaultTextureWrap"] = static_cast<int>(m_Settings.defaultTextureWrap);
	s["defaultAnisotropy"] = static_cast<int>(m_Settings.defaultAnisotropy);

	json autoArr = json::array();
	for (const auto& a : m_Settings.autoloads)
	{
		json item;
		item["name"] = a.name;
		item["path"] = a.path;
		item["enabled"] = a.enabled;
		autoArr.push_back(item);
	}
	s["autoloads"] = autoArr;

	s["physicsGravity"] = { m_Settings.physicsGravity.x, m_Settings.physicsGravity.y, m_Settings.physicsGravity.z };
	s["physicsTicksPerSecond"] = m_Settings.physicsTicksPerSecond;
	s["physicsSubSteps"] = m_Settings.physicsSubSteps;
	s["physicsMaxBodies"] = m_Settings.physicsMaxBodies;
	s["physicsMaxBodyPairs"] = m_Settings.physicsMaxBodyPairs;
	s["physicsMaxContactConstraints"] = m_Settings.physicsMaxContactConstraints;

	json layerArr = json::array();
	for (const auto& lName : m_Settings.physicsLayerNames)
	{
		layerArr.push_back(lName);
	}
	s["physicsLayers"] = layerArr;

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
	m_Settings.pointShadowResolution = 512;
	m_Settings.spotShadowResolution = 1024;
	m_Settings.ssaoHalfResolution = true;
	m_Settings.ssaoSliceCount = 3;
	m_Settings.ssaoStepCount = 3;
	m_Settings.contactShadowsHalfResolution = true;
	m_Settings.contactShadowsSteps = 16;
	m_Settings.ssrHalfResolution = true;
	m_Settings.ssrMaxSteps = 64;
	m_Settings.ssgiHalfResolution = true;
	m_Settings.ssgiSteps = 16;
	m_Settings.volumetricFogHalfResolution = true;
	m_Settings.volumetricFogSteps = 24;
	m_Settings.motionBlurSamples = 16;

	m_Settings.physicsGravity = glm::vec3(0.0f, -9.81f, 0.0f);
	m_Settings.physicsTicksPerSecond = 60;
	m_Settings.physicsSubSteps = 1;
	m_Settings.physicsMaxBodies = 1024;
	m_Settings.physicsMaxBodyPairs = 1024;
	m_Settings.physicsMaxContactConstraints = 1024;

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
		m_Settings.iconPath = s.value("iconPath", "");
		m_Settings.cursorIconPath = s.value("cursorIconPath", "");
		m_Settings.cursorHotspotX = s.value("cursorHotspotX", 0);
		m_Settings.cursorHotspotY = s.value("cursorHotspotY", 0);

		m_Settings.stretchMode = static_cast<StretchMode>(s.value("stretchMode", static_cast<int>(StretchMode::Viewport)));
		m_Settings.stretchAspect = static_cast<StretchAspect>(s.value("stretchAspect", static_cast<int>(StretchAspect::Keep)));
		m_Settings.scalingFilter = static_cast<ScalingFilter>(s.value("scalingFilter", static_cast<int>(ScalingFilter::Nearest)));

		m_Settings.msaaSamples = s.value("msaaSamples", 1u);
		m_Settings.fxaaEnabled = s.value("fxaaEnabled", true);
		m_Settings.shadowResolution = s.value("shadowResolution", 2048u);
		m_Settings.pointShadowResolution = s.value("pointShadowResolution", 512u);
		m_Settings.spotShadowResolution = s.value("spotShadowResolution", 1024u);

		m_Settings.ssaoHalfResolution = s.value("ssaoHalfResolution", true);
		m_Settings.ssaoSliceCount = s.value("ssaoSliceCount", 3);
		m_Settings.ssaoStepCount = s.value("ssaoStepCount", 3);

		m_Settings.contactShadowsHalfResolution = s.value("contactShadowsHalfResolution", true);
		m_Settings.contactShadowsSteps = s.value("contactShadowsSteps", 16);

		m_Settings.ssrHalfResolution = s.value("ssrHalfResolution", true);
		m_Settings.ssrMaxSteps = s.value("ssrMaxSteps", 64);

		m_Settings.ssgiHalfResolution = s.value("ssgiHalfResolution", true);
		m_Settings.ssgiSteps = s.value("ssgiSteps", 16);

		m_Settings.volumetricFogHalfResolution = s.value("volumetricFogHalfResolution", true);
		m_Settings.volumetricFogSteps = s.value("volumetricFogSteps", 24);

		m_Settings.motionBlurSamples = s.value("motionBlurSamples", 16);

		m_Settings.defaultTextureFilter = static_cast<TextureFilter>(s.value("defaultTextureFilter", static_cast<int>(TextureFilter::LinearMipmap)));
		m_Settings.defaultTextureWrap = static_cast<TextureWrap>(s.value("defaultTextureWrap", static_cast<int>(TextureWrap::Repeat)));
		m_Settings.defaultAnisotropy = static_cast<TextureAnisotropy>(s.value("defaultAnisotropy", static_cast<int>(TextureAnisotropy::Aniso8x)));

		m_Settings.autoloads.clear();
		if (s.contains("autoloads") && s["autoloads"].is_array())
		{
			for (const auto& item : s["autoloads"])
			{
				AutoloadEntry entry;
				entry.name = item.value("name", "");
				entry.path = item.value("path", "");
				entry.enabled = item.value("enabled", true);
				if (!entry.name.empty() && !entry.path.empty())
					m_Settings.autoloads.push_back(entry);
			}
		}

		if (s.contains("physicsGravity") && s["physicsGravity"].is_array() && s["physicsGravity"].size() >= 3)
		{
			m_Settings.physicsGravity = glm::vec3(s["physicsGravity"][0], s["physicsGravity"][1], s["physicsGravity"][2]);
		}
		m_Settings.physicsTicksPerSecond = s.value("physicsTicksPerSecond", 60);
		m_Settings.physicsSubSteps = s.value("physicsSubSteps", 1);
		m_Settings.physicsMaxBodies = s.value("physicsMaxBodies", 1024u);
		m_Settings.physicsMaxBodyPairs = s.value("physicsMaxBodyPairs", 1024u);
		m_Settings.physicsMaxContactConstraints = s.value("physicsMaxContactConstraints", 1024u);

		for (size_t i = 0; i < m_Settings.physicsLayerNames.size(); ++i)
			m_Settings.physicsLayerNames[i] = (i == 0) ? "Default" : "";

		if (s.contains("physicsLayers") && s["physicsLayers"].is_array())
		{
			size_t i = 0;
			for (const auto& val : s["physicsLayers"])
			{
				if (i < m_Settings.physicsLayerNames.size())
				{
					m_Settings.physicsLayerNames[i] = val.get<std::string>();
					i++;
				}
			}
		}
	}

	if (j.contains("busLayout"))
	{
		AudioManager::Get().LoadBusLayout(j["busLayout"].dump());
	}
}
