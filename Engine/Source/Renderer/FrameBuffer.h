#pragma once
#include <vector>
#include <cstdint>
#include "GLAD/gl.h"
namespace rv {

enum class FramebufferTextureFormat
{
    RGBA16F,
    RGB16F,
    R16F,
    R8,
    R32UI,
    Depth16,
    Depth24,
    Depth32F,
    Depth24Stencil8,
};

enum class FramebufferFilterMode { Nearest, Linear };
enum class FramebufferWrapMode { ClampToEdge, ClampToBorder, Repeat };

struct FramebufferAttachmentDesc
{
    FramebufferTextureFormat format;
    FramebufferFilterMode filter = FramebufferFilterMode::Nearest;
    FramebufferWrapMode wrap = FramebufferWrapMode::ClampToEdge;
    bool compareMode = false;
};

struct FramebufferDesc
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t samples = 1;
    std::vector<FramebufferAttachmentDesc> colorAttachments;
    bool hasDepth = false;
    FramebufferAttachmentDesc depthAttachment{ FramebufferTextureFormat::Depth32F };
    bool depthAsRenderbuffer = false;
    bool depthIsSampledTexture = true;
};

class Framebuffer
{
public:
    Framebuffer() = default;
    explicit Framebuffer(const FramebufferDesc& spec);
    ~Framebuffer();
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;
    void Resize(uint32_t width, uint32_t height);
    void Bind() const;
    void BindViewport() const;
    static void BindDefault();
    uint32_t GetColorAttachment(size_t index = 0) const { return m_ColorAttachments.at(index); }
    uint32_t GetDepthAttachment() const { return m_DepthAttachment; }
    uint32_t GetID() const { return m_ID; }
    uint32_t GetWidth() const { return m_Desc.width; }
    uint32_t GetHeight() const { return m_Desc.height; }
private:
    void Invalidate();
    void Destroy();
    static GLenum ToGLInternalFormat(FramebufferTextureFormat fmt);
    static GLenum ToGLBaseFormat(FramebufferTextureFormat fmt);
    static GLenum ToGLType(FramebufferTextureFormat fmt);
private:
    FramebufferDesc m_Desc;
    uint32_t m_ID = 0;
    std::vector<uint32_t> m_ColorAttachments;
    uint32_t m_DepthAttachment = 0;
    bool m_DepthIsRenderbuffer = false;
};
}