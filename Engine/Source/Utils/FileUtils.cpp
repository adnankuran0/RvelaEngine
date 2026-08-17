#include "rvelapch.h"
#include "FileUtils.h"
#include "ProjectManager.h"
#include "Core/Engine.h"

using namespace rv;

Path::Path(const std::filesystem::path& path, bool isVirual)
{
	if (isVirual)
	{
		m_VirtualPath = path;
	}
	else
	{
		m_VirtualPath = std::filesystem::relative(path, Engine::Get()->GetProjectManager().GetProjectPath());
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
	return Engine::Get()->GetProjectManager().GetProjectPath() / m_VirtualPath;
}

std::string Path::String() const
{
	return m_VirtualPath.string();
}


bool Path::IsValid() const
{
	return !m_VirtualPath.empty();
}

bool Path::operator==(const Path& other) const
{
	return m_VirtualPath == other.GetVirtual();
}

bool Path::operator!=(const Path& other) const
{
	return m_VirtualPath != other.GetVirtual();
}

bool Path::Exists() const
{
	return std::filesystem::exists(GetAbsolute());
}

std::string Path::GetFilename() const
{
	return m_VirtualPath.filename().string();
}

std::string Path::GetExtension() const
{
	return m_VirtualPath.extension().string();
}

Path Path::GetParentPath() const
{
	return Path(m_VirtualPath.parent_path(), true);
}

Path Path::operator/(const std::string& subPath) const
{
	return Path(m_VirtualPath / subPath, true);
}

Path Path::operator/(const Path& other) const
{
	return Path(m_VirtualPath / other.m_VirtualPath, true);
}

Path Path::operator/(const std::filesystem::path& subPath) const
{
	return Path(m_VirtualPath / subPath, true);
}