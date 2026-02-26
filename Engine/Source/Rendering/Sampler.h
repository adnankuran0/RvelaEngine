#pragma once

namespace rv
{

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

struct SamplerDesc
{
    MinFilter minFilter = MinFilter::LINEAR_MIPMAP_LINEAR;
    MagFilter magFilter = MagFilter::LINEAR;
    Wrap wrap = Wrap::REPEAT;
    float anisotropy = 8.0f; 
};

class Sampler
{
public: 
	Sampler(const SamplerDesc& desc);
	Sampler() = default;
	void Init(const SamplerDesc& desc);
	~Sampler();
	void Bind(unsigned int slot);
	void Unbind(unsigned int slot);
	unsigned int GetID() { return m_ID; }
    SamplerDesc& GetDesc() { return m_Decs; }
private:
    SamplerDesc m_Decs;
	unsigned int m_ID;
};

}