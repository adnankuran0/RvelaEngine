#pragma once
#include <string>

namespace rv {

struct AutoloadComponent
{
	std::string name;
	std::string path;
	bool isPrefab = false;

	AutoloadComponent() = default;
	AutoloadComponent(const std::string& name, const std::string& path, bool isPrefab = false)
		: name(name), path(path), isPrefab(isPrefab) {}
};

}
