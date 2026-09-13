#pragma once

#include "Core/Layer.h"
#include "Renderer/EditorCamera.h"
#include "Asset/AssetImportPipeline.h"

#include <Renderer/RenderPipeline.h>

namespace rv {

class Engine;

class RuntimeBlitLayer : public Layer
{
public:
    RuntimeBlitLayer(rv::Engine* engine) : m_Engine(engine) {}
    
    void OnRender() override;


private:
    rv::Engine* m_Engine;
};

}