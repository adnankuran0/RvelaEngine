#pragma once
#include "EditorPanel.h"
#include <unordered_map>
#include <string>

namespace rv {

class Engine;

class EnvironmentPanel : public EditorPanel
{
public:
    EnvironmentPanel() : EditorPanel("Environment", true) {}
    ~EnvironmentPanel() override = default;

    void Draw(Engine* engine);
};

} // namespace rv