#include "rvelapch.h"
#include "VertexArray.h"
#include "GLAD/gl.h"

using namespace rv;

void VertexArray::Init()
{
	ID = 0;
	glGenVertexArrays(1, &ID);
	glBindVertexArray(ID);
}

void VertexArray::Bind() const
{
	glBindVertexArray(ID);
}

void VertexArray::Unbind() const
{
	glBindVertexArray(0);
}

void VertexArray::Destroy() const
{
	glDeleteVertexArrays(1, &ID);
}

void VertexArray::SetBufferLayout(const BufferLayout& layout)
{
	if (layout.buffers.empty() || layout.elements.empty())
		return;

	unsigned int currentBuffer = 0;
	size_t totalBufferCount = layout.bufferStartIndex.size();
	glBindBuffer(GL_ARRAY_BUFFER, layout.buffers[0]);

	for (unsigned int i = 0; i < layout.elements.size(); i++)
	{
		const auto& element = layout.elements[i];

		if (currentBuffer < totalBufferCount && i == layout.bufferStartIndex[currentBuffer])
		{
			currentBuffer += 1;
			glBindBuffer(GL_ARRAY_BUFFER, layout.buffers[currentBuffer]);
		}

		glEnableVertexAttribArray(i);

		void* offsetPtr = reinterpret_cast<void*>(static_cast<uintptr_t>(element.offset));

		if (element.type == GL_UNSIGNED_INT || element.type == GL_INT ||
			element.type == GL_UNSIGNED_BYTE || element.type == GL_BYTE ||
			element.type == GL_UNSIGNED_SHORT || element.type == GL_SHORT)
		{
			glVertexAttribIPointer(
				i,
				element.count,
				element.type,
				layout.strides[currentBuffer],
				offsetPtr
			);
		}
		else
		{
			glVertexAttribPointer(
				i,
				element.count,
				element.type,
				element.normalized,
				layout.strides[currentBuffer],
				offsetPtr
			);
		}

		if (element.divisor != 0)
			glVertexAttribDivisor(i, element.divisor);
	}
}

unsigned int VertexArray::getID()
{
	return ID;
}