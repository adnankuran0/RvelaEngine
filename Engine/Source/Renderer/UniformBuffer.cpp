#include "rvelapch.h"
#include "UniformBuffer.h"

using namespace rv;

UniformBuffer::UniformBuffer(GLsizeiptr size, unsigned int bindingPoint)
    : m_Binding(bindingPoint)
{
    glCreateBuffers(1, &m_ID);
    glNamedBufferData(m_ID, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, m_ID, 0, size);
}

UniformBuffer::~UniformBuffer()
{
    if (m_ID != 0)
    {
        glDeleteBuffers(1, &m_ID);
        m_ID = 0;
    }
}

UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    : m_ID(other.m_ID), m_Binding(other.m_Binding)
{
    other.m_ID = 0;
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept
{
    if (this != &other)
    {
        if (m_ID != 0)
            glDeleteBuffers(1, &m_ID);

        m_ID = other.m_ID;
        m_Binding = other.m_Binding;
        other.m_ID = 0;
    }
    return *this;
}

void UniformBuffer::SetData(GLintptr offset, GLsizeiptr size, const void* data)
{
    glNamedBufferSubData(m_ID, offset, size, data);
}

void UniformBuffer::Bind() const
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
}

void UniformBuffer::Unbind() const
{
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}