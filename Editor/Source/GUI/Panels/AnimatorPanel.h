#pragma once
#include "EditorPanel.h"
#include "GUI/AnimationSequenceAdapter.h"
#include <entt/entt.h>
#include <memory>

namespace rv {

class Engine;

class AnimatorPanel : public EditorPanel
{
public:
    AnimatorPanel() : EditorPanel("Animator", true) {}

    void Draw(Engine* engine);

private:
    SequencerContext m_SeqContext;
    int m_CurrentFrame = 0;
    int m_FirstFrame = 0;
    bool m_Expanded = true;
};

}