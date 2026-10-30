#pragma once
#include "BufferLayout.h"
#include <utility>

namespace rv { 

class VertexArray
{
public:
	VertexArray() = default;
	~VertexArray() { Destroy(); }

	VertexArray(const VertexArray&) = delete;
	VertexArray& operator=(const VertexArray&) = delete;

	VertexArray(VertexArray&& o) noexcept : ID(std::exchange(o.ID, 0)) {}
	VertexArray& operator=(VertexArray&& o) noexcept
	{
		if (this != &o)
		{
			Destroy();
			ID = std::exchange(o.ID, 0);
		}
		return *this;
	}

	void Init();
	void Bind() const;
	void Unbind() const;
	void Destroy();
	unsigned int getID() const;
	void SetBufferLayout(const BufferLayout& layout);
private:
	unsigned int ID = 0;
};

}
