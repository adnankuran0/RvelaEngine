#include "rvelapch.h"
#include "Project.h"

#include "json.hpp"

namespace rv { 

using json = nlohmann::json;

std::string Project::Serialize() const
{
	json j;
	j["name"] = name;
	j["projectPath"] = projectFolderPath.string();
	return j.dump(4);
}

void Project::Deserialize(const std::string& jsonStr)
{
	json j = json::parse(jsonStr);
	
	
	name = j.value("name", "");
	projectFolderPath = std::filesystem::path(j.value("projectPath", ""));
	
}

}