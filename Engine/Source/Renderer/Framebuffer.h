#pragma once
#include <cstdint>

class Framebuffer {
public:
    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;
    virtual void Resize(int width, int height) = 0;
    virtual unsigned int GetHandle() const = 0;
    virtual ~Framebuffer() = default;
};