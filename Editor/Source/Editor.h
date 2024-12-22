#include "Core/Engine.h"
#include "ImGuiLayer.h"

class Editor
{
public:
	Editor();
	void Run();

	Engine* GetEngine();
private:
	Engine* s_Engine;
	ImGuiLayer* m_ImGuiLayer;

};