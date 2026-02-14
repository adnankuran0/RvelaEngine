class Engine;

class Editor
{
public:
	Editor();
	void Run();

	Engine* GetEngine();
private:
	Engine* m_Engine;

};