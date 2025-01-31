#include "rvelapch.h"
#include "BufferLayout.h"

void BufferLayout::AddVertexBuffer(unsigned int newVBO)
{
	stride = 0;
	strides.push_back(0);
	m_StrideIndex += 1;
	bufferStartIndex.push_back(elements.size());
	buffers.push_back(newVBO);
}

void BufferLayout::BindVertexBuffer(unsigned int ID)
{
	buffers.push_back(ID);
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}