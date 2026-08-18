#pragma once
#include "GLAD/gl.h"

namespace rv {

class UniformBuffer
{
public:
    UniformBuffer(GLsizeiptr size, unsigned int bindingPoint);
    ~UniformBuffer();

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    UniformBuffer(UniformBuffer&& other) noexcept;
    UniformBuffer& operator=(UniformBuffer&& other) noexcept;

    void SetData(GLintptr offset, GLsizeiptr size, const void* data);
    void Bind() const;
    void Unbind() const;

    inline unsigned int GetID() const { return m_ID; }
    inline unsigned int GetBinding() const { return m_Binding; }

private:
    unsigned int m_ID = 0;
    unsigned int m_Binding = 0;
};

}