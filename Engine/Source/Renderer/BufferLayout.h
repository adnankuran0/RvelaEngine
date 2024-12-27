#pragma once
#include<vector>
#include"GL/glew.h"

struct BufferElement
{
	unsigned int type;
	unsigned int count;
	unsigned char normalized;
	unsigned int offset;
	unsigned int divisor;
};

class BufferLayout
{
public:
	BufferLayout() : stride(0) { strides.push_back(0); };

	std::vector<BufferElement> elements;

	unsigned int stride;
	std::vector<unsigned int> strides;

	std::vector<unsigned int> bufferStartIndex;
	std::vector<unsigned int> buffers;

	void BindVertexBuffer(unsigned int ID);
	void AddVertexBuffer(unsigned int newVBO);

	template<typename T>
	void Push(unsigned int count)
	{
		elements.push_back({ GL_FLOAT,count,GL_FALSE,strides[m_StrideIndex],0 });

		strides[m_StrideIndex] += count * sizeof(float);
	}

	template<typename T>
	void Push(unsigned int count, unsigned int divisor)
	{
		elements.push_back({ GL_FLOAT,count,GL_FALSE,strides[m_StrideIndex],divisor });

		strides[m_StrideIndex] += count * sizeof(float);
	}

private:
	unsigned int m_StrideIndex = 0;

};