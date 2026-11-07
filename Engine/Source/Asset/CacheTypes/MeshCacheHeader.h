#pragma once
#include <cstdint>

namespace rv {

constexpr uint32_t MAGIC_MESH = 0x524D5348; // "RMSH"
constexpr uint16_t MESH_CACHE_VERSION_1 = 1;
constexpr uint16_t MESH_CACHE_VERSION = 2;

#pragma pack(push, 1)
    struct MeshLODHeader
    {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        float screenRatio = 0.0f;
        float lodError = 0.0f;
    };

    struct MeshCacheHeader
    {
        uint32_t magic = MAGIC_MESH;
        uint16_t version = MESH_CACHE_VERSION;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint16_t lodCount = 1;
        uint8_t reserved[4] = {}; // alignment (total struct size = 20 bytes)
    };
#pragma pack(pop)

}
