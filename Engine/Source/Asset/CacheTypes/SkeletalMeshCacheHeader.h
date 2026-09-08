#pragma once
#include <cstdint>

namespace rv {

constexpr uint32_t MAGIC_SKMESH = 0x524D534B; // "RMSK"
constexpr uint16_t SKMESH_CACHE_VERSION = 1;

#pragma pack(push, 1)
    struct SkeletalMeshCacheHeader
    {
        uint32_t magic = MAGIC_SKMESH;
        uint16_t version = SKMESH_CACHE_VERSION;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint8_t reserved[6] = {};
    };
#pragma pack(pop)

}