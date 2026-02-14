#pragma once
#include "BufferLayout.h"

class VertexArray
{
public:
	void Init();
	void Bind() const;
	void Unbind() const;
	void Destroy() const;
	unsigned int getID();
	void SetBufferLayout(const BufferLayout& layout);
private:
	unsigned int ID = 0;
};