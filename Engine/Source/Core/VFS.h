#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace rv {

class VFS
{
public:
    static VFS& Get();

    bool Mount(const std::filesystem::path& pakPath);
    void Unmount();
    bool IsMounted() const;

    const std::filesystem::path& GetPakPath() const { return m_PakPath; }
    size_t GetFileCount() const;

    bool FileExists(const std::string& virtualPath) const;
    bool ReadFile(const std::string& virtualPath, std::vector<uint8_t>& outBuffer) const;
    bool ReadTextFile(const std::string& virtualPath, std::string& outText) const;
    std::vector<std::string> ListFiles(const std::string& directory = "") const;

    static std::string NormalizePath(const std::string& path);

private:
    VFS() = default;
    ~VFS();
    VFS(const VFS&) = delete;
    VFS& operator=(const VFS&) = delete;

    mutable std::mutex m_Mutex;
    bool m_IsMounted = false;
    std::filesystem::path m_PakPath;
    void* m_ZipHandle = nullptr;
    std::unordered_map<std::string, uint32_t> m_PathToIndex;
    std::vector<std::string> m_OriginalFilePaths;
};

}
