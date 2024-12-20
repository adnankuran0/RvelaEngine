#include "Core/Engine.h"

class Editor
{
public:
	Editor();
	void Run();

	Editor* GetInstance();
	Engine* GetEngine();
private:
	Editor* s_Instance;
	Engine* s_Engine;

};