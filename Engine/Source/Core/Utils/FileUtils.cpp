#include "rvelapch.h"
#include "FileUtils.h"

#define VIRTUAL_PATH(x) Path::FromVirtual(x)
#define ABSOLUTE_PATH(x) Path::FromAbsolute(x)

Path::Path(const std::filesystem::path& path, bool isVirual)
{
	if (isVirual)
	{
		m_VirtualPath = path;
	}
	else
	{
		m_VirtualPath = std::filesystem::relative(path, ProjectManager::GetProjectPath());
	}
}

Path Path::FromVirtual(const std::string& virtualPath)
{
	return Path(virtualPath, true);
}

Path Path::FromAbsolute(const std::string& absolutePath)
{
	return Path(absolutePath, false);
}

std::filesystem::path Path::GetVirtual() const
{
	return m_VirtualPath;
}

std::filesystem::path Path::GetAbsolute() const
{
	return ProjectManager::GetProjectPath() / m_VirtualPath;
}

std::string Path::String() const
{
	return m_VirtualPath.string();
}

bool Path::IsValid() const
{
	return !m_VirtualPath.empty();
}