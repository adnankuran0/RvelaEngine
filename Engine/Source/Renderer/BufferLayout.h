#pragma once
#include "rvelapch.h"
#include "GLAD/gl.h"
#include <vector>
#include <cstdint>

namespace rv {

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
	BufferLayout() : stride(0) { strides.push_back(0); }

	std::vector<BufferElement> elements;

	unsigned int stride;
	std::vector<unsigned int> strides;

	std::vector<unsigned int> bufferStartIndex;
	std::vector<unsigned int> buffers;

	void BindVertexBuffer(unsigned int ID);
	void AddVertexBuffer(unsigned int newVBO);

	template<typename T>
	void Push(unsigned int count, unsigned int divisor = 0)
	{
		static_assert(sizeof(T) == 0, "Unsupported type for BufferLayout::Push!");
	}

private:
	unsigned int m_StrideIndex = 0;
};

template<>
inline void BufferLayout::Push<float>(unsigned int count, unsigned int divisor)
{
	elements.push_back({ GL_FLOAT, count, GL_FALSE, strides[m_StrideIndex], divisor });
	strides[m_StrideIndex] += count * sizeof(float);
}

template<>
inline void BufferLayout::Push<uint32_t>(unsigned int count, unsigned int divisor)
{
	elements.push_back({ GL_UNSIGNED_INT, count, GL_FALSE, strides[m_StrideIndex], divisor });
	strides[m_StrideIndex] += count * sizeof(uint32_t);
}

template<>
inline void BufferLayout::Push<int32_t>(unsigned int count, unsigned int divisor)
{
	elements.push_back({ GL_INT, count, GL_FALSE, strides[m_StrideIndex], divisor });
	strides[m_StrideIndex] += count * sizeof(int32_t);
}

}