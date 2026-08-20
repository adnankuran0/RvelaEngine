#include "rvelapch.h"
#include "Framebuffer.h"
#include "Core/Log.h"

namespace rv {

GLenum Framebuffer::ToGLInternalFormat(FramebufferTextureFormat fmt)
{
    switch (fmt)
    {
    case FramebufferTextureFormat::RGBA16F:         return GL_RGBA16F;
    case FramebufferTextureFormat::RGB16F:          return GL_RGB16F;
    case FramebufferTextureFormat::R16F:            return GL_R16F;
    case FramebufferTextureFormat::R8:              return GL_R8;
    case FramebufferTextureFormat::R32UI:           return GL_R32UI;
    case FramebufferTextureFormat::Depth16:         return GL_DEPTH_COMPONENT16;
    case FramebufferTextureFormat::Depth24:         return GL_DEPTH_COMPONENT24;
    case FramebufferTextureFormat::Depth32F:        return GL_DEPTH_COMPONENT32F;
    case FramebufferTextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
    }
    return GL_RGBA16F;
}

GLenum Framebuffer::ToGLBaseFormat(FramebufferTextureFormat fmt)
{
    switch (fmt)
    {
    case FramebufferTextureFormat::RGBA16F:         return GL_RGBA;
    case FramebufferTextureFormat::RGB16F:          return GL_RGB;
    case FramebufferTextureFormat::R16F:            return GL_RED;
    case FramebufferTextureFormat::R8:              return GL_RED;
    case FramebufferTextureFormat::R32UI:           return GL_RED_INTEGER;
    case FramebufferTextureFormat::Depth16:         return GL_DEPTH_COMPONENT;
    case FramebufferTextureFormat::Depth24:         return GL_DEPTH_COMPONENT;
    case FramebufferTextureFormat::Depth32F:        return GL_DEPTH_COMPONENT;
    case FramebufferTextureFormat::Depth24Stencil8: return GL_DEPTH_STENCIL;
    }
    return GL_RGBA;
}

GLenum Framebuffer::ToGLType(FramebufferTextureFormat fmt)
{
    switch (fmt)
    {
    case FramebufferTextureFormat::R8:    return GL_UNSIGNED_BYTE;
    case FramebufferTextureFormat::R32UI: return GL_UNSIGNED_INT;
    default: return GL_FLOAT;
    }
}

Framebuffer::Framebuffer(const FramebufferDesc& spec)
    : m_Desc(spec)
{
    Invalidate();
}

Framebuffer::~Framebuffer()
{
    Destroy();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
{
    *this = std::move(other);
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this == &other) return *this;
    Destroy();
    m_Desc = other.m_Desc;
    m_ID = other.m_ID;
    m_ColorAttachments = std::move(other.m_ColorAttachments);
    m_DepthAttachment = other.m_DepthAttachment;
    m_DepthIsRenderbuffer = other.m_DepthIsRenderbuffer;
    other.m_ID = 0;
    other.m_DepthAttachment = 0;
    return *this;
}

void Framebuffer::Destroy()
{
    if (m_ID == 0) return;

    for (auto tex : m_ColorAttachments)
        glDeleteTextures(1, &tex);

    if (m_DepthAttachment != 0)
    {
        if (m_DepthIsRenderbuffer)
            glDeleteRenderbuffers(1, &m_DepthAttachment);
        else
            glDeleteTextures(1, &m_DepthAttachment);
    }

    glDeleteFramebuffers(1, &m_ID);

    m_ID = 0;
    m_ColorAttachments.clear();
    m_DepthAttachment = 0;
}

void Framebuffer::Invalidate()
{
    Destroy();

    if (m_Desc.width == 0 || m_Desc.height == 0)
        return;

    glCreateFramebuffers(1, &m_ID);

    bool multisample = m_Desc.samples > 1;
    m_ColorAttachments.resize(m_Desc.colorAttachments.size());

    std::vector<GLenum> drawBuffers;
    drawBuffers.reserve(m_ColorAttachments.size());

    for (size_t i = 0; i < m_Desc.colorAttachments.size(); ++i)
    {
        const auto& attSpec = m_Desc.colorAttachments[i];
        uint32_t& tex = m_ColorAttachments[i];

        GLenum target = multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
        glCreateTextures(target, 1, &tex);

        if (multisample)
        {
            glTextureStorage2DMultisample(tex, m_Desc.samples,
                ToGLInternalFormat(attSpec.format), m_Desc.width, m_Desc.height, GL_TRUE);
        }
        else
        {
            glTextureStorage2D(tex, 1, ToGLInternalFormat(attSpec.format), m_Desc.width, m_Desc.height);
            GLenum filter = (attSpec.filter == FramebufferFilterMode::Linear) ? GL_LINEAR : GL_NEAREST;
            glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, filter);
            glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, filter);
            GLenum wrap = (attSpec.wrap == FramebufferWrapMode::Repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
            glTextureParameteri(tex, GL_TEXTURE_WRAP_S, wrap);
            glTextureParameteri(tex, GL_TEXTURE_WRAP_T, wrap);
        }

        GLenum attachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
        glNamedFramebufferTexture(m_ID, attachment, tex, 0);
        drawBuffers.push_back(attachment);
    }

    if (drawBuffers.empty())
    {
        glNamedFramebufferDrawBuffer(m_ID, GL_NONE);
        glNamedFramebufferReadBuffer(m_ID, GL_NONE);
    }
    else
        glNamedFramebufferDrawBuffers(m_ID, static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());

    if (m_Desc.hasDepth)
    {
        bool useRBO = multisample || m_Desc.depthAsRenderbuffer;
        m_DepthIsRenderbuffer = useRBO;

        GLenum attachPoint = (m_Desc.depthAttachment.format == FramebufferTextureFormat::Depth24Stencil8)
            ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;

        if (useRBO)
        {
            glCreateRenderbuffers(1, &m_DepthAttachment);
            if (multisample)
                glNamedRenderbufferStorageMultisample(m_DepthAttachment, m_Desc.samples,
                    ToGLInternalFormat(m_Desc.depthAttachment.format), m_Desc.width, m_Desc.height);
            else
                glNamedRenderbufferStorage(m_DepthAttachment,
                    ToGLInternalFormat(m_Desc.depthAttachment.format), m_Desc.width, m_Desc.height);

            glNamedFramebufferRenderbuffer(m_ID, attachPoint, GL_RENDERBUFFER, m_DepthAttachment);
        }
        else
        {
            glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthAttachment);
            glTextureStorage2D(m_DepthAttachment, 1, ToGLInternalFormat(m_Desc.depthAttachment.format),
                m_Desc.width, m_Desc.height);

            GLenum filter = (m_Desc.depthAttachment.filter == FramebufferFilterMode::Linear) ? GL_LINEAR : GL_NEAREST;
            glTextureParameteri(m_DepthAttachment, GL_TEXTURE_MIN_FILTER, filter);
            glTextureParameteri(m_DepthAttachment, GL_TEXTURE_MAG_FILTER, filter);

            if (m_Desc.depthAttachment.wrap == FramebufferWrapMode::ClampToBorder)
            {
                glTextureParameteri(m_DepthAttachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTextureParameteri(m_DepthAttachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTextureParameterfv(m_DepthAttachment, GL_TEXTURE_BORDER_COLOR, borderColor);
            }
            else
            {
                glTextureParameteri(m_DepthAttachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTextureParameteri(m_DepthAttachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            if (m_Desc.depthAttachment.compareMode)
            {
                glTextureParameteri(m_DepthAttachment, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTextureParameteri(m_DepthAttachment, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            }

            glNamedFramebufferTexture(m_ID, attachPoint, m_DepthAttachment, 0);
        }
    }

    GLenum status = glCheckNamedFramebufferStatus(m_ID, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        LOG_ERROR("Framebuffer incomplete! status=0x{:X}", (unsigned)status);
}

void Framebuffer::Resize(uint32_t width, uint32_t height)
{
    if (width == m_Desc.width && height == m_Desc.height)
        return;

    m_Desc.width = width;
    m_Desc.height = height;
    Invalidate();
}

void Framebuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
}

void Framebuffer::BindViewport() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
    glViewport(0, 0, m_Desc.width, m_Desc.height);
}

void Framebuffer::BindDefault()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}