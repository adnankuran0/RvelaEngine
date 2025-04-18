#include "rvelapch.h"
#include "Project.h"

#include "../nlohmann/json.hpp"

using json = nlohmann::json;

std::string Project::Serialize() const
{
	json j;
	j["name"] = name;
	j["projectPath"] = projectPath;
	j["assetPath"] = assetPath;
	j["lastScenePath"] = lastScenePath;
	return j.dump(4);
}

void Project::Deserialize(const std::string& jsonStr)
{
	json j = json::parse(jsonStr);
	name = j["name"];
	projectPath = j["projectPath"];
	assetPath = j["assetPath"];
	lastScenePath = j["lastScenePath"];
}