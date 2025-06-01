#pragma once
#include "Framebuffer.h"

class DirectionalShadowFramebuffer : public Framebuffer {
public:
    DirectionalShadowFramebuffer(unsigned int width, unsigned int height);
    void Bind() const override;
    void Unbind() const override;
    void Resize(int width, int height) override;
    unsigned int GetHandle() const override;
    unsigned int GetDepthTexture() const;

private:
    unsigned int fbo = 0;
    unsigned int depthMap = 0;
    unsigned int width, height;
};