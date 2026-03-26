#pragma once
#include <cstdint>

namespace rv {

constexpr uint32_t MAGIC_MESH = 0x524D5348; // "RMSH"
constexpr uint16_t MESH_CACHE_VERSION = 1;

#pragma pack(push, 1)
    struct MeshCacheHeader
    {
        uint32_t magic = MAGIC_MESH;
        uint16_t version = MESH_CACHE_VERSION;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint8_t reserved[6] = {}; // alignment
    };
#pragma pack(pop)

}