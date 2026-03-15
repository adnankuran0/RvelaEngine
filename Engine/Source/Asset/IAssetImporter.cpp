#include "IAssetImporter.h"
#include <fstream>

uint64_t rv::IAssetImporter::HashFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return 0;

    // FNV-1a 64bit
    uint64_t hash = 14695981039346656037ULL;
    constexpr uint64_t prime = 1099511628211ULL;

    char byte;
    while (file.get(byte))
    {
        hash ^= static_cast<uint64_t>(static_cast<uint8_t>(byte));
        hash *= prime;
    }
    return hash;
}