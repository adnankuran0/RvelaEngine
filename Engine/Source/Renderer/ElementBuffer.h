#pragma once
#include "GL/glew.h"

class ElementBuffer
{
private:
	unsigned int ID;
public:
	unsigned int getID() const;
	ElementBuffer( const void* data, size_t size);
	ElementBuffer() = default;
	~ElementBuffer();
	void Init(const void* data, size_t size);
	void Bind() const;
	void Unbind() const;
	void Destroy() const;
};