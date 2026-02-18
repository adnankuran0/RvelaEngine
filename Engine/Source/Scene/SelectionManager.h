#pragma once
#include <cstdint>

namespace rv {

class RenderLayer;

class SelectionManager
{
public:
	SelectionManager() : m_RenderLayer(nullptr) {} 
	SelectionManager(RenderLayer* renderLayer) : m_RenderLayer(renderLayer) {}
	uint32_t Pick(uint32_t x, uint32_t y);
	void PickRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t* outBuffer);
private:
	RenderLayer* m_RenderLayer;
};

}