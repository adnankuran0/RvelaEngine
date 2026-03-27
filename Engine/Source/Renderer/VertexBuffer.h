#pragma once


namespace rv {

class VertexBuffer
{
private:
	unsigned int ID;
public:
	VertexBuffer() = default;
	VertexBuffer(const void* data, size_t size);
	~VertexBuffer();

	void Init(const void* data, size_t size);
	void Init();
	void Bind() const;
	void Unbind() const;
	void Destroy() const;
	void Data(const void* data, unsigned int size, bool dynamicDraw = false) const;
	unsigned int getID();
};


}