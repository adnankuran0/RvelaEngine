#pragma once
#include <GL/glew.h>

class VertexBuffer
{
private:
	unsigned int ID;
public:
	VertexBuffer(const void* data, unsigned int size);
	~VertexBuffer();

	void Bind() const;
	void Unbind() const;
	void Data(const void* data, unsigned int size) const;
	unsigned int getID();
};
