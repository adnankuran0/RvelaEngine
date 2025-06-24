#pragma once
#include <filesystem>

#define VRT_PATH(x) Path::FromVirtual(x)
#define ABS_PATH(x) Path::FromAbsolute(x)

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

	inline std::string GetAbsoluteStr() const
	{
		return GetAbsolute().string();
	}

	inline std::string GetVirtualStr() const
	{
		return GetVirtual().string();
	}


	bool IsValid() const;
	bool Exists() const;
	std::string GetFilename() const;
	std::string GetExtension() const;

	bool operator==(const Path& other) const;
	bool operator!=(const Path& other) const;

private:
	std::filesystem::path m_VirtualPath;

};