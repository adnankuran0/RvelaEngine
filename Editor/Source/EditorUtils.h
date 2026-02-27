#pragma once

namespace rv {

class Engine;

class EditorUtils
{
public:
	static void CreateScene(Engine& engine);
	static void OpenScene(Engine& engine);
	static bool SaveScene(Engine& engine);
	static bool SaveSceneAs(Engine& engine);


};

}
