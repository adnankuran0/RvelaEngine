#include "rvelapch.h"
#include "VertexBuffer.h"

namespace rv { 

VertexBuffer::VertexBuffer(const void* data, size_t size)
{
	Init(data, size);
}
VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &ID);
}
unsigned int VertexBuffer::getID()
{
	return ID;
}

void VertexBuffer::Destroy() const
{
	glDeleteBuffers(1, &ID);
}

void VertexBuffer::Bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}

void VertexBuffer::Init(const void* data, size_t size)
{
	ID = 0;
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ARRAY_BUFFER, ID);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

void VertexBuffer::Unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::Data(const void* data, unsigned int size) const
{
	glBindBuffer(GL_ARRAY_BUFFER, ID);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

}


