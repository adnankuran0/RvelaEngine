#pragma once
#include "GL/glew.h"

class ElementBuffer
{
private:
	unsigned int ID;
public:
	unsigned int getID() const;
	ElementBuffer( const void* data, size_t size);
	~ElementBuffer();
	void Bind() const;
	void Unbind() const;



};