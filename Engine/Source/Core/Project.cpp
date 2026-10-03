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
	s["windowWidth"] = m_Settings.windowWidth;
	s["windowHeight"] = m_Settings.windowHeight;
	s["vsync"] = m_Settings.vsync;
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
	m_Settings.windowWidth = 1600;
	m_Settings.windowHeight = 900;
	m_Settings.vsync = true;

	if (j.contains("settings") && j["settings"].is_object())
	{
		const auto& s = j["settings"];
		m_Settings.name = s.value("name", name);
		m_Settings.startScene = s.value("startScene", "Assets/Scenes/main.rscene");
		m_Settings.windowWidth = s.value("windowWidth", 1600u);
		m_Settings.windowHeight = s.value("windowHeight", 900u);
		m_Settings.vsync = s.value("vsync", true);
	}

	if (j.contains("busLayout"))
	{
		AudioManager::Get().LoadBusLayout(j["busLayout"].dump());
	}
}
