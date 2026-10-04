#pragma once
#include <utility>

namespace rv {

	class VertexBuffer
	{
	private:
		unsigned int ID = 0;
	public:
		VertexBuffer() = default;
		VertexBuffer(const void* data, size_t size);
		~VertexBuffer();

		VertexBuffer(const VertexBuffer&) = delete;
		VertexBuffer& operator=(const VertexBuffer&) = delete;
		VertexBuffer(VertexBuffer&& o) noexcept : ID(std::exchange(o.ID, 0)) {}
		VertexBuffer& operator=(VertexBuffer&& o) noexcept
		{
			if (this != &o)
			{
				Destroy();
				ID = std::exchange(o.ID, 0);
			}
			return *this;
		}

	void Init(const void* data, size_t size);
	void Init();
	void Bind() const;
	void Unbind() const;
	void Destroy();
	void Data(const void* data, unsigned int size, bool dynamicDraw = false) const;
	unsigned int getID() const;
};


}