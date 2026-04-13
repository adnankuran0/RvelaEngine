#include "rvelapch.h"
#include "SelectionManager.h"
#include "Renderer/RenderLayer.h"

using namespace rv;

uint32_t SelectionManager::Pick(uint32_t x, uint32_t y)
{
    if (!m_RenderLayer) return 0;

    GLuint id = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, m_RenderLayer->GetEntityBuffer()); 
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    auto& context = m_RenderLayer->GetRenderContext();

    glReadPixels(x, context.viewportHeight - y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &id); 

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return id; 
}

void SelectionManager::PickRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t* outBuffer)
{
}


