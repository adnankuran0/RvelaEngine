#include "Core/Engine.h"
#include "RuntimeBlitLayer.h"

int main()
{
	rv::Engine engine;
	engine.GetSceneManager().LoadScene("C:\\RvelaEngine\\Resources\\Editor\\TestProject\\Assets\\Scenes\\TPS.rscene");
	engine.GetSceneManager().Update();
	engine.GetActiveScene().SetState(rv::SceneState::PLAY);
	engine.PushLayer(new rv::RuntimeBlitLayer(&engine));
	engine.Run();
;}