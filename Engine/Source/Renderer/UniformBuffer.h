#pragma once
#include "GLAD/gl.h"
#include "GLFW/glfw3.h"

class UniformBuffer
{
public:
	UniformBuffer(GLsizeiptr size, unsigned int bindingPoint);
	~UniformBuffer();
	void SetData(GLintptr offset, GLsizeiptr size, const void* data);
	void Bind();
	void Unbind();
private:
	unsigned int m_ID;

};