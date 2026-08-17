#pragma once
#include <cstdint>

namespace rv {

enum class ShadingMode : uint8_t
{
    Lit = 0,
    Unshaded
};

enum class TransparencyMode : uint8_t
{
    Opaque = 0,
    Alpha,
    AlphaScissor,
    DepthPrepass
};

enum class BlendMode : uint8_t
{
    Mix = 0,
    Add,
    Subtract,
    Multiply
};

enum class CullMode : uint8_t
{
    Back = 0,
    Front,
    Disabled
};

}