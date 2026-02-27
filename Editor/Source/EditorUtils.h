#pragma once

namespace rv {

class Engine;

class EditorUtils
{
public:
	static void CreateScene(Engine& engine);
	static void OpenScene(Engine& engine);
	static void SaveScene(Engine& engine);
	static void SaveSceneAs(Engine& engine);


};

}
