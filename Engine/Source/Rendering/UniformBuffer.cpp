#include "rvelapch.h"

#include "UniformBuffer.h"

UniformBuffer::UniformBuffer(GLsizeiptr size, unsigned int bindingPoint)
{
	glGenBuffers(1, &m_ID);
	glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_STATIC_DRAW);
	glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, m_ID, 0, size);
}

void UniformBuffer::Bind()
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
}

void UniformBuffer::Unbind()
{
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::SetData(GLintptr offset, GLsizeiptr size, const void* data)
{
	glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
}

UniformBuffer::~UniformBuffer()
{
	glDeleteBuffers(1, &m_ID);
}