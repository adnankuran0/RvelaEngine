#pragma once
#include <GL/glew.h>

class VertexBuffer
{
private:
	unsigned int ID;
public:
	VertexBuffer() = default;
	VertexBuffer(const void* data, unsigned int size);
	~VertexBuffer();

	void Init(const void* data, unsigned int size);
	void Bind() const;
	void Unbind() const;
	void Data(const void* data, unsigned int size) const;
	unsigned int getID();
};
