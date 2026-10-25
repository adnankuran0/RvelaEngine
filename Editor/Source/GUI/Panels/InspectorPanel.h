#pragma once
#include "EditorPanel.h"
#include "entt/entt.h"
#include <vector>

namespace rv {

class Engine;

class InspectorPanel : public EditorPanel
{
public:
	InspectorPanel() : EditorPanel("Inspector", true) {}

	void Draw(Engine* engine);
};

}