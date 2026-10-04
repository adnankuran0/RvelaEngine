#pragma once
#include <utility>

namespace rv {


class ElementBuffer
{
private:
	unsigned int ID = 0;
public:
	ElementBuffer() = default;
	ElementBuffer(const void* data, size_t size);
	~ElementBuffer() { Destroy(); }

	ElementBuffer(const ElementBuffer&) = delete;
	ElementBuffer& operator=(const ElementBuffer&) = delete;
	ElementBuffer(ElementBuffer&& o) noexcept : ID(std::exchange(o.ID, 0)) {}
	ElementBuffer& operator=(ElementBuffer&& o) noexcept
	{
		if (this != &o) { Destroy(); ID = std::exchange(o.ID, 0); }
		return *this;
	}

	unsigned int getID() const;
	void Init(const void* data, size_t size);
	void Bind() const;
	void Unbind() const;
	void Destroy();
};

}