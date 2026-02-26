#include "rvelapch.h"
#include "Sampler.h"
#include "GLAD/gl.h"

using namespace rv;

Sampler::Sampler(const SamplerDesc& desc)
{
    Init(desc);
}

void Sampler::Init(const SamplerDesc& desc)
{
    m_Decs = desc;

    if (m_ID)
        glDeleteSamplers(1, &m_ID);

	glGenSamplers(1, &m_ID);

	GLenum minFilter = GL_LINEAR;
    switch (desc.minFilter)
    {
    case MinFilter::NEAREST: minFilter = GL_NEAREST; break;
    case MinFilter::LINEAR: minFilter = GL_LINEAR; break;
    case MinFilter::NEAREST_MIPMAP_LINEAR: minFilter = GL_NEAREST_MIPMAP_LINEAR; break;
    case MinFilter::LINEAR_MIPMAP_LINEAR: minFilter = GL_LINEAR_MIPMAP_LINEAR; break;
    }
    glSamplerParameteri(m_ID, GL_TEXTURE_MIN_FILTER, minFilter);
    glSamplerParameteri(m_ID, GL_TEXTURE_MAG_FILTER, desc.magFilter == MagFilter::LINEAR ? GL_LINEAR : GL_NEAREST);

    GLenum wrap = GL_REPEAT;
    switch (desc.wrap)
    {
    case Wrap::REPEAT: wrap = GL_REPEAT; break;
    case Wrap::MIRRORED_REPEAT: wrap = GL_MIRRORED_REPEAT; break;
    case Wrap::CLAMP_TO_EDGE: wrap = GL_CLAMP_TO_EDGE; break;
    case Wrap::CLAMP_TO_BORDER: wrap = GL_CLAMP_TO_BORDER; break;
    case Wrap::MIRRORED_CLAMP_TO_EDGE: wrap = GL_MIRROR_CLAMP_TO_EDGE; break;
    }
    glSamplerParameteri(m_ID, GL_TEXTURE_WRAP_S, wrap);
    glSamplerParameteri(m_ID, GL_TEXTURE_WRAP_T, wrap);
    glSamplerParameteri(m_ID, GL_TEXTURE_WRAP_R, wrap);

    if (desc.anisotropy > 1.0f)
    {
        float maxAniso = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);

        float clamped = std::min(desc.anisotropy, maxAniso);
        glSamplerParameterf(m_ID, GL_TEXTURE_MAX_ANISOTROPY, clamped);
    }
}

Sampler::~Sampler()
{
    glDeleteSamplers(1, &m_ID);
}

void Sampler::Bind(unsigned int slot)
{
    glBindSampler(slot, m_ID);
}

void Sampler::Unbind(unsigned int slot)
{
    glBindSampler(slot, 0);
}