#include "Core/Engine.h"


class Editor
{
public:
	Editor();
	void Run();

	Engine* GetEngine();
private:
	Engine* m_Engine;

};