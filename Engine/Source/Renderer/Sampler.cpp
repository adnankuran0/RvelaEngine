#include "rvelapch.h"
#include "Sampler.h"
#include "GLAD/gl.h"
#include "Core/Project.h"
#include "Utils/ProjectManager.h"
#include <algorithm>

using namespace rv;

ResolvedSamplerDesc SamplerDesc::Resolve() const
{
    ResolvedSamplerDesc res;

    TextureFilter projFilter = TextureFilter::LinearMipmap;
    TextureWrap projWrap = TextureWrap::Repeat;
    TextureAnisotropy projAniso = TextureAnisotropy::Aniso8x;

    auto activeProj = ProjectManager::GetActiveProject();
    if (activeProj)
    {
        projFilter = activeProj->GetSettings().defaultTextureFilter;
        projWrap = activeProj->GetSettings().defaultTextureWrap;
        projAniso = activeProj->GetSettings().defaultAnisotropy;
    }

    TextureFilter effFilter = (filter == TextureFilter::Default) ? projFilter : filter;
    switch (effFilter)
    {
    case TextureFilter::Nearest:
        res.minFilter = MinFilter::NEAREST;
        res.magFilter = MagFilter::NEAREST;
        break;
    case TextureFilter::Linear:
        res.minFilter = MinFilter::LINEAR;
        res.magFilter = MagFilter::LINEAR;
        break;
    case TextureFilter::NearestMipmap:
        res.minFilter = MinFilter::NEAREST_MIPMAP_LINEAR;
        res.magFilter = MagFilter::NEAREST;
        break;
    case TextureFilter::LinearMipmap:
    default:
        res.minFilter = MinFilter::LINEAR_MIPMAP_LINEAR;
        res.magFilter = MagFilter::LINEAR;
        break;
    }

    TextureWrap effWrap = (repeat == TextureWrap::Default) ? projWrap : repeat;
    switch (effWrap)
    {
    case TextureWrap::Repeat:
        res.wrap = Wrap::REPEAT;
        break;
    case TextureWrap::MirroredRepeat:
        res.wrap = Wrap::MIRRORED_REPEAT;
        break;
    case TextureWrap::ClampToEdge:
        res.wrap = Wrap::CLAMP_TO_EDGE;
        break;
    case TextureWrap::ClampToBorder:
        res.wrap = Wrap::CLAMP_TO_BORDER;
        break;
    case TextureWrap::MirroredClampToEdge:
        res.wrap = Wrap::MIRRORED_CLAMP_TO_EDGE;
        break;
    default:
        res.wrap = Wrap::REPEAT;
        break;
    }

    TextureAnisotropy effAniso = (aniso == TextureAnisotropy::Default) ? projAniso : aniso;
    switch (effAniso)
    {
    case TextureAnisotropy::Off:
        res.anisotropy = 1.0f;
        break;
    case TextureAnisotropy::Aniso2x:
        res.anisotropy = 2.0f;
        break;
    case TextureAnisotropy::Aniso4x:
        res.anisotropy = 4.0f;
        break;
    case TextureAnisotropy::Aniso8x:
        res.anisotropy = 8.0f;
        break;
    case TextureAnisotropy::Aniso16x:
        res.anisotropy = 16.0f;
        break;
    default:
        res.anisotropy = 8.0f;
        break;
    }

    if (filter == TextureFilter::Default && minFilter != MinFilter::LINEAR_MIPMAP_LINEAR)
    {
        res.minFilter = minFilter;
        res.magFilter = magFilter;
    }
    if (repeat == TextureWrap::Default && wrap != Wrap::REPEAT)
    {
        res.wrap = wrap;
    }

    return res;
}

SamplerCache::~SamplerCache()
{
    Clear();
}

void SamplerCache::Clear()
{
    for (auto& [desc, id] : m_Samplers)
    {
        if (id)
            glDeleteSamplers(1, &id);
    }
    m_Samplers.clear();
}

unsigned int SamplerCache::GetOrCreate(const ResolvedSamplerDesc& desc)
{
    auto it = m_Samplers.find(desc);
    if (it != m_Samplers.end())
        return it->second;

    unsigned int id = 0;
    glGenSamplers(1, &id);

    GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR;
    switch (desc.minFilter)
    {
    case MinFilter::NEAREST: minFilter = GL_NEAREST; break;
    case MinFilter::LINEAR: minFilter = GL_LINEAR; break;
    case MinFilter::NEAREST_MIPMAP_LINEAR: minFilter = GL_NEAREST_MIPMAP_LINEAR; break;
    case MinFilter::LINEAR_MIPMAP_LINEAR: minFilter = GL_LINEAR_MIPMAP_LINEAR; break;
    }
    glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, minFilter);
    glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, desc.magFilter == MagFilter::LINEAR ? GL_LINEAR : GL_NEAREST);

    GLenum wrap = GL_REPEAT;
    switch (desc.wrap)
    {
    case Wrap::REPEAT: wrap = GL_REPEAT; break;
    case Wrap::MIRRORED_REPEAT: wrap = GL_MIRRORED_REPEAT; break;
    case Wrap::CLAMP_TO_EDGE: wrap = GL_CLAMP_TO_EDGE; break;
    case Wrap::CLAMP_TO_BORDER: wrap = GL_CLAMP_TO_BORDER; break;
    case Wrap::MIRRORED_CLAMP_TO_EDGE: wrap = GL_MIRROR_CLAMP_TO_EDGE; break;
    }
    glSamplerParameteri(id, GL_TEXTURE_WRAP_S, wrap);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_T, wrap);
    glSamplerParameteri(id, GL_TEXTURE_WRAP_R, wrap);

    if (desc.anisotropy > 1.0f)
    {
        float maxAniso = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        float clamped = std::min(desc.anisotropy, maxAniso);
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY, clamped);
    }

    m_Samplers[desc] = id;
    return id;
}

Sampler::Sampler(const SamplerDesc& desc)
    : m_Desc(desc)
{
}

void Sampler::Init(const SamplerDesc& desc)
{
    m_Desc = desc;
}

unsigned int Sampler::GetID() const
{
    ResolvedSamplerDesc resolved = m_Desc.Resolve();
    return SamplerCache::Get().GetOrCreate(resolved);
}

void Sampler::Bind(unsigned int slot) const
{
    glBindSampler(slot, GetID());
}

void Sampler::Unbind(unsigned int slot) const
{
    glBindSampler(slot, 0);
}
