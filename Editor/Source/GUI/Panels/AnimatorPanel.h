#pragma once

#include "GUI/AnimationSequenceAdapter.h"
#include <entt/entt.h>
#include <memory>

namespace rv {

class Engine;

class AnimatorPanel
{
public:
    void Draw(Engine* engine, entt::entity& selectedEntity);

private:
    SequencerContext m_SeqContext;
    int m_CurrentFrame = 0;
    int m_FirstFrame = 0;
    bool m_Expanded = true;
};

}