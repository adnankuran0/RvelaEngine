#pragma once
#include "GLAD/gl.h"
#include "BufferLayout.h"

class VertexArray
{
public:
	VertexArray();
	void Bind() const;
	void Unbind() const;
	void Destroy() const;
	unsigned int getID();
	void SetBufferLayout(const BufferLayout& layout);
private:
	unsigned int ID;
};