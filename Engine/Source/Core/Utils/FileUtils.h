#pragma once
#include <filesystem>
#include "ProjectManager.h"

#define TO_ABSOLUTE_PATH(x) Path::FromVirtual(x)
#define TO_VIRTUAL_PATH(x) Path::FromAbsolute(x)

class Path
{
public:
	Path() = default;
	Path(const std::filesystem::path& path, bool isVirual = true);

	static Path FromVirtual(const std::string& virtualPath);
	static Path FromAbsolute(const std::string& absoulePath);

	std::string String() const;
	const char* C_str() const;

	std::filesystem::path GetVirtual() const;
	std::filesystem::path GetAbsolute() const;

	inline std::string GetAbsoluteStr() const
	{
		return GetAbsolute().string();
	}

	inline std::string GetVirtualStr() const
	{
		return GetVirtual().string();
	}


	bool IsValid() const;

	bool operator==(const Path& other) const;
	bool operator!=(const Path& other) const;

private:
	std::filesystem::path m_VirtualPath;

};