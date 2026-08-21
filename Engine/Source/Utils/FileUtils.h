#pragma once
#include <filesystem>

#define VRT_PATH(x)    Path::FromVirtual(x)
#define ABS_PATH(x)    Path::FromAbsolute(x)
#define ENGINE_PATH(x) Path::FromEngineResource(x)
#define EDITOR_PATH(x) Path::FromEditorResource(x)

namespace rv {

enum class PathOrigin
{
	Project,
	EngineResource,
	EditorResource
};

class Path
{
public:
	Path() = default;
	Path(const std::filesystem::path& path, bool isVirual = true, PathOrigin origin = PathOrigin::Project);

	static Path FromVirtual(const std::string& virtualPath);
	static Path FromAbsolute(const std::string& absoulePath);
	static Path FromEngineResource(const std::string& relativePath);
	static Path FromEditorResource(const std::string& relativePath);

	std::string String() const;
	std::filesystem::path GetVirtual() const;
	std::filesystem::path GetAbsolute() const;
	Path GetParentPath() const;
	inline std::string GetAbsoluteStr() const { return GetAbsolute().string(); }
	inline std::string GetVirtualStr() const { return GetVirtual().string(); }
	bool IsValid() const;
	bool Exists() const;
	std::string GetFilename() const;
	std::string GetExtension() const;
	bool operator==(const Path& other) const;
	bool operator!=(const Path& other) const;
	Path operator/(const std::string& subPath) const;
	Path operator/(const Path& other) const;
	Path operator/(const std::filesystem::path& subPath) const;

	PathOrigin GetOrigin() const { return m_Origin; }

	static void SetEngineResourcesPath(const std::filesystem::path& path);
	static void SetEditorResourcesPath(const std::filesystem::path& path);

private:
	std::filesystem::path m_VirtualPath;
	PathOrigin m_Origin = PathOrigin::Project;

	static std::filesystem::path s_EngineResourcesPath;
	static std::filesystem::path s_EditorResourcesPath;
};
}