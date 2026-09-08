#pragma once
#include <cstdint>

namespace rv {

constexpr uint32_t MAGIC_ANIM_LIB = 0x52414E4C;
constexpr uint16_t ANIM_LIB_VERSION = 1;

#pragma pack(push, 1)
    struct AnimationLibraryHeader
    {
        uint32_t magic = MAGIC_ANIM_LIB;
        uint16_t version = ANIM_LIB_VERSION;
        uint32_t clipCount = 0;
        uint8_t  reserved[6] = {};
    };
#pragma pack(pop)

}