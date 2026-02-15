#pragma once 

namespace rv
{
	class Engine;
}

class Editor
{
public:
	Editor();
	void Run();

	rv::Engine* GetEngine();
private:
	rv::Engine* m_Engine;

};