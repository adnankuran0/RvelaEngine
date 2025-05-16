#pragma once
#include <filesystem>
#include "ProjectManager.h"

class Path
{
public:
	Path() = default;
	Path(const std::filesystem::path& path, bool isVirual = true);

	static Path FromVirtual(const std::string& virtualPath);
	static Path FromAbsolute(const std::string& absoulePath);

	std::string String() const;
	std::filesystem::path GetVirtual() const;
	std::filesystem::path GetAbsolute() const;

	bool IsValid() const;

	bool operator==(const Path& other) const;
	bool operator!=(const Path& other) const;

private:
	std::filesystem::path m_VirtualPath;

};