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

	j["busLayout"] = AudioManager::Get().SaveBusLayout();

	return j.dump(4);
}

void Project::Deserialize(const std::string& jsonStr)
{
	json j = json::parse(jsonStr);

	name = j.value("name", "");
	projectFolderPath = std::filesystem::path(j.value("projectPath", ""));

	if (j.contains("busLayout"))
	{
		AudioManager::Get().LoadBusLayout(j["busLayout"].dump());
	}

	
}
