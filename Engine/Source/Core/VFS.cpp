#include "rvelapch.h"
#include "VFS.h"
#include "Core/Log.h"
#include <miniz/miniz.h>
#include <algorithm>

namespace rv {

VFS& VFS::Get()
{
    static VFS instance;
    return instance;
}

VFS::~VFS()
{
    Unmount();
}

std::string VFS::NormalizePath(const std::string& path)
{
    std::string norm = path;
    std::replace(norm.begin(), norm.end(), '\\', '/');

    while (!norm.empty() && (norm.front() == '/' || norm.front() == '.'))
    {
        if (norm.rfind("./", 0) == 0)
            norm = norm.substr(2);
        else if (norm.front() == '/')
            norm = norm.substr(1);
        else
            break;
    }

    std::transform(norm.begin(), norm.end(), norm.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return norm;
}

bool VFS::Mount(const std::filesystem::path& pakPath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_IsMounted)
        Unmount();

    std::error_code ec;
    if (!std::filesystem::exists(pakPath, ec) || !std::filesystem::is_regular_file(pakPath, ec))
    {
        LOG_WARN("[VFS] Cannot mount: '{}' does not exist or is not a file.", pakPath.string());
        return false;
    }

    auto* archive = new mz_zip_archive();
    memset(archive, 0, sizeof(mz_zip_archive));

    if (!mz_zip_reader_init_file(archive, pakPath.string().c_str(), 0))
    {
        mz_zip_error err = mz_zip_get_last_error(archive);
        LOG_ERROR("[VFS] Failed to open zip archive '{}': error {}", pakPath.string(), static_cast<int>(err));
        delete archive;
        return false;
    }

    m_ZipHandle = archive;
    m_PakPath = pakPath;
    m_PathToIndex.clear();
    m_OriginalFilePaths.clear();

    const mz_uint fileCount = mz_zip_reader_get_num_files(archive);
    m_OriginalFilePaths.reserve(fileCount);

    for (mz_uint i = 0; i < fileCount; ++i)
    {
        mz_zip_archive_file_stat stat;
        if (mz_zip_reader_file_stat(archive, i, &stat))
        {
            if (stat.m_is_directory)
                continue;

            std::string originalPath = stat.m_filename;
            std::string normalized = NormalizePath(originalPath);
            m_PathToIndex[normalized] = i;
            m_OriginalFilePaths.push_back(originalPath);
        }
    }

    m_IsMounted = true;
    return true;
}

void VFS::Unmount()
{
    if (!m_IsMounted && !m_ZipHandle)
        return;

    if (m_ZipHandle)
    {
        auto* archive = static_cast<mz_zip_archive*>(m_ZipHandle);
        mz_zip_reader_end(archive);
        delete archive;
        m_ZipHandle = nullptr;
    }

    m_PathToIndex.clear();
    m_OriginalFilePaths.clear();
    m_PakPath.clear();
    m_IsMounted = false;
}

bool VFS::IsMounted() const
{
    return m_IsMounted;
}

size_t VFS::GetFileCount() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_PathToIndex.size();
}

bool VFS::FileExists(const std::string& virtualPath) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!m_IsMounted)
        return false;

    std::string norm = NormalizePath(virtualPath);
    return m_PathToIndex.find(norm) != m_PathToIndex.end();
}

bool VFS::ReadFile(const std::string& virtualPath, std::vector<uint8_t>& outBuffer) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!m_IsMounted || !m_ZipHandle)
        return false;

    std::string norm = NormalizePath(virtualPath);
    auto it = m_PathToIndex.find(norm);
    if (it == m_PathToIndex.end())
        return false;

    auto* archive = static_cast<mz_zip_archive*>(m_ZipHandle);
    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(archive, it->second, &stat))
    {
        LOG_ERROR("[VFS] Failed to get file stat for '{}'", virtualPath);
        return false;
    }

    outBuffer.resize(static_cast<size_t>(stat.m_uncomp_size));
    if (stat.m_uncomp_size == 0)
        return true;

    if (!mz_zip_reader_extract_to_mem(archive, it->second, outBuffer.data(), outBuffer.size(), 0))
    {
        LOG_ERROR("[VFS] Failed to extract file '{}'", virtualPath);
        outBuffer.clear();
        return false;
    }

    return true;
}

bool VFS::ReadTextFile(const std::string& virtualPath, std::string& outText) const
{
    std::vector<uint8_t> buffer;
    if (!ReadFile(virtualPath, buffer))
        return false;

    outText.assign(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return true;
}

std::vector<std::string> VFS::ListFiles(const std::string& directory) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::vector<std::string> results;

    std::string normDir = NormalizePath(directory);
    if (!normDir.empty() && normDir.back() != '/')
        normDir += '/';

    for (const auto& path : m_OriginalFilePaths)
    {
        std::string norm = NormalizePath(path);
        if (normDir.empty() || norm.rfind(normDir, 0) == 0)
        {
            results.push_back(path);
        }
    }

    return results;
}

}
