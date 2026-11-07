#include "rvelapch.h"
#include "SelectionManager.h"
#include "Renderer/RenderLayer.h"
#include <unordered_set>

using namespace rv;

uint32_t SelectionManager::Pick(uint32_t x, uint32_t y)
{
    if (!m_RenderLayer) return 0;

    auto& context = m_RenderLayer->GetRenderContext();
    if (context.viewportWidth == 0 || context.viewportHeight == 0) return 0;
    if (x >= context.viewportWidth || y >= context.viewportHeight) return 0;

    GLuint id = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, m_RenderLayer->GetEntityBuffer()); 
    glReadBuffer(GL_COLOR_ATTACHMENT3);

    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &id); 

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return id; 
}

void SelectionManager::PickRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t* outBuffer)
{
    if (!m_RenderLayer || !outBuffer || w == 0 || h == 0) return;

    auto& context = m_RenderLayer->GetRenderContext();
    if (context.viewportWidth == 0 || context.viewportHeight == 0) return;
    if (x >= context.viewportWidth || y >= context.viewportHeight) return;

    if (x + w > context.viewportWidth) w = context.viewportWidth - x;
    if (y + h > context.viewportHeight) h = context.viewportHeight - y;

    glBindFramebuffer(GL_FRAMEBUFFER, m_RenderLayer->GetEntityBuffer());
    glReadBuffer(GL_COLOR_ATTACHMENT3);

    glReadPixels(x, y, w, h, GL_RED_INTEGER, GL_UNSIGNED_INT, outBuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::vector<uint32_t> SelectionManager::PickRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    if (!m_RenderLayer || w == 0 || h == 0) return {};

    auto& context = m_RenderLayer->GetRenderContext();
    if (context.viewportWidth == 0 || context.viewportHeight == 0) return {};
    if (x >= context.viewportWidth || y >= context.viewportHeight) return {};

    if (x + w > context.viewportWidth) w = context.viewportWidth - x;
    if (y + h > context.viewportHeight) h = context.viewportHeight - y;

    std::vector<uint32_t> buffer(w * h);
    PickRect(x, y, w, h, buffer.data());

    std::unordered_set<uint32_t> uniqueIDs;
    for (uint32_t id : buffer)
    {
        if (id != 0)
            uniqueIDs.insert(id);
    }

    return std::vector<uint32_t>(uniqueIDs.begin(), uniqueIDs.end());
}
