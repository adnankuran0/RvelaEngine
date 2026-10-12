#include "rvelapch.h"
#include "SelectionManager.h"
#include "Renderer/RenderLayer.h"

using namespace rv;

uint32_t SelectionManager::Pick(uint32_t x, uint32_t y)
{
    if (!m_RenderLayer) return 0;

    auto& context = m_RenderLayer->GetRenderContext();
    if (context.viewportWidth == 0 || context.viewportHeight == 0) return 0;
    if (x >= context.viewportWidth || y >= context.viewportHeight) return 0;

    GLuint id = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, m_RenderLayer->GetEntityBuffer()); 
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &id); 

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return id; 
}

void SelectionManager::PickRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t* outBuffer)
{
}


