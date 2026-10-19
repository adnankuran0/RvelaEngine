#pragma once
#include <cstdint>
#include <unordered_map>

namespace rv
{

enum class TextureFilter : uint8_t
{
    Default = 0,
    Nearest = 1,
    Linear = 2,
    NearestMipmap = 3,
    LinearMipmap = 4
};

enum class TextureWrap : uint8_t
{
    Default = 0,
    Repeat = 1,
    MirroredRepeat = 2,
    ClampToEdge = 3,
    ClampToBorder = 4,
    MirroredClampToEdge = 5
};

enum class TextureAnisotropy : uint8_t
{
    Default = 0,
    Off = 1,
    Aniso2x = 2,
    Aniso4x = 3,
    Aniso8x = 4,
    Aniso16x = 5
};

enum class MinFilter : uint8_t
{
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR
};

enum class MagFilter : uint8_t
{
    NEAREST,
    LINEAR
};

enum class Wrap : uint8_t
{
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER,
    MIRRORED_CLAMP_TO_EDGE
};

struct ResolvedSamplerDesc
{
    MinFilter minFilter = MinFilter::LINEAR_MIPMAP_LINEAR;
    MagFilter magFilter = MagFilter::LINEAR;
    Wrap wrap = Wrap::REPEAT;
    float anisotropy = 8.0f;

    bool operator==(const ResolvedSamplerDesc& o) const
    {
        return minFilter == o.minFilter &&
               magFilter == o.magFilter &&
               wrap == o.wrap &&
               anisotropy == o.anisotropy;
    }
};

struct SamplerDesc
{
    TextureFilter filter = TextureFilter::Default;
    TextureWrap repeat = TextureWrap::Default;
    TextureAnisotropy aniso = TextureAnisotropy::Default;

    MinFilter minFilter = MinFilter::LINEAR_MIPMAP_LINEAR;
    MagFilter magFilter = MagFilter::LINEAR;
    Wrap wrap = Wrap::REPEAT;
    float anisotropy = 8.0f;

    ResolvedSamplerDesc Resolve() const;
};

class SamplerCache
{
public:
    static SamplerCache& Get()
    {
        static SamplerCache instance;
        return instance;
    }

    unsigned int GetOrCreate(const ResolvedSamplerDesc& desc);
    void Clear();

private:
    SamplerCache() = default;
    ~SamplerCache();

    struct Hash
    {
        size_t operator()(const ResolvedSamplerDesc& d) const
        {
            size_t h1 = std::hash<uint8_t>{}(static_cast<uint8_t>(d.minFilter));
            size_t h2 = std::hash<uint8_t>{}(static_cast<uint8_t>(d.magFilter));
            size_t h3 = std::hash<uint8_t>{}(static_cast<uint8_t>(d.wrap));
            size_t h4 = std::hash<uint32_t>{}(static_cast<uint32_t>(d.anisotropy * 100.0f));
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };

    std::unordered_map<ResolvedSamplerDesc, unsigned int, Hash> m_Samplers;
};

class Sampler
{
public: 
    Sampler() = default;
    explicit Sampler(const SamplerDesc& desc);
    ~Sampler() = default;

    Sampler(const Sampler& other) = default;
    Sampler& operator=(const Sampler& other) = default;
    Sampler(Sampler&& other) noexcept = default;
    Sampler& operator=(Sampler&& other) noexcept = default;

    void Init(const SamplerDesc& desc);
    void Bind(unsigned int slot) const;
    void Unbind(unsigned int slot) const;
    unsigned int GetID() const;
    SamplerDesc& GetDesc() { return m_Desc; }
    const SamplerDesc& GetDesc() const { return m_Desc; }

private:
    SamplerDesc m_Desc;
};

}