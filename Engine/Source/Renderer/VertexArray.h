#pragma once
#include <GL/glew.h>
#include "BufferLayout.h"
#include <iostream>
class VertexArray
{
public:
	VertexArray();
	~VertexArray();
	void Bind() const;
	void Unbind() const;
	unsigned int getID();
	void SetBufferLayout(const BufferLayout* layout);
private:
	unsigned int ID;
};