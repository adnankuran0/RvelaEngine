#include "ElementBuffer.h"

ElementBuffer::ElementBuffer(const void* data, size_t size)
{
	ID = 0;
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

void ElementBuffer::Bind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}

void ElementBuffer::Unbind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

unsigned int ElementBuffer::getID() const
{
	return ID;
}

ElementBuffer::~ElementBuffer()
{
	glDeleteBuffers(1, &ID);
}