#include "rvelapch.h"
#include "VertexArray.h"
#include "GLAD/gl.h"

namespace rv {

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
	unsigned int currentBuffer = 0;
	size_t totalBufferCount = layout.bufferStartIndex.size();
	glBindBuffer(GL_ARRAY_BUFFER, layout.buffers[0]);

	for (unsigned int i = 0; i < layout.elements.size(); i++)
	{
		//checking if we changed the vertex buffer
		if (currentBuffer < totalBufferCount && i == layout.bufferStartIndex[currentBuffer])
		{
			currentBuffer += 1;
			glBindBuffer(GL_ARRAY_BUFFER, layout.buffers[currentBuffer]);
		}

		glEnableVertexAttribArray(i);
		glVertexAttribPointer(
			i,
			layout.elements[i].count,
			GL_FLOAT,
			GL_FALSE,
			layout.strides[currentBuffer],
			reinterpret_cast<void*>(static_cast<uintptr_t>(layout.elements[i].offset))
		);
		if (layout.elements[i].divisor != 0)
			glVertexAttribDivisor(i, layout.elements[i].divisor);
	}

}



unsigned int VertexArray::getID()
{
	return ID;
}

}