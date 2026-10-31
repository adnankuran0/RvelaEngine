#pragma once

#include "Core/Layer.h"
#include <cstdint>

namespace rv {

class Engine;

class RuntimeBlitLayer : public Layer
{
public:
    RuntimeBlitLayer(rv::Engine* engine)
        : Layer("RuntimeBlitLayer"), m_Engine(engine) {}
    
    void OnAttach() override;
    void OnUpdate() override;
    void OnRender() override;

private:
    void UpdateViewportAndMouse();

    struct ViewportMetrics
    {
        int winWidth = 0;
        int winHeight = 0;
        uint32_t targetRenderW = 1920;
        uint32_t targetRenderH = 1080;
        uint32_t srcW = 1920;
        uint32_t srcH = 1080;
        int dstX0 = 0;
        int dstY0 = 0;
        int dstX1 = 0;
        int dstY1 = 0;
    };

    rv::Engine* m_Engine = nullptr;
    ViewportMetrics m_Metrics;
};

}
