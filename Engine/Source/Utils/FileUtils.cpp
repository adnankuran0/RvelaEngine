#include "rvelapch.h"
#include "FileUtils.h"
#include "ProjectManager.h"
#include "Core/Engine.h"
using namespace rv;

std::filesystem::path Path::s_EngineResourcesPath;
std::filesystem::path Path::s_EditorResourcesPath;

Path::Path(const std::filesystem::path& path, bool isVirual, PathOrigin origin)
	: m_Origin(origin)
{
	if (isVirual)
		m_VirtualPath = path;
	else
		m_VirtualPath = std::filesystem::relative(path, Engine::Get()->GetProjectManager().GetProjectPath());
}

Path Path::FromVirtual(const std::string& virtualPath) { return Path(virtualPath, true, PathOrigin::Project); }
Path Path::FromAbsolute(const std::string& absolutePath) { return Path(absolutePath, false, PathOrigin::Project); }
Path Path::FromEngineResource(const std::string& p) { return Path(p, true, PathOrigin::EngineResource); }
Path Path::FromEditorResource(const std::string& p) { return Path(p, true, PathOrigin::EditorResource); }

void Path::SetEngineResourcesPath(const std::filesystem::path& path) { s_EngineResourcesPath = path; }
void Path::SetEditorResourcesPath(const std::filesystem::path& path) { s_EditorResourcesPath = path; }

std::filesystem::path Path::GetAbsolute() const
{
	switch (m_Origin)
	{
	case PathOrigin::EngineResource: return s_EngineResourcesPath / m_VirtualPath;
	case PathOrigin::EditorResource: return s_EditorResourcesPath / m_VirtualPath;
	default:                         return Engine::Get()->GetProjectManager().GetProjectPath() / m_VirtualPath;
	}
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

std::filesystem::path Path::GetVirtual() const
{
	return m_VirtualPath;
}

Path Path::GetParentPath() const
{
	return Path(m_VirtualPath.parent_path().lexically_normal(), true, m_Origin);
}
Path Path::operator/(const std::string& subPath) const
{
	return Path((m_VirtualPath / std::filesystem::path(subPath)).lexically_normal(), true, m_Origin);
}
Path Path::operator/(const Path& other) const
{
	return Path((m_VirtualPath / other.m_VirtualPath).lexically_normal(), true, m_Origin);
}
Path Path::operator/(const std::filesystem::path& subPath) const
{
	return Path((m_VirtualPath / subPath).lexically_normal(), true, m_Origin);
}