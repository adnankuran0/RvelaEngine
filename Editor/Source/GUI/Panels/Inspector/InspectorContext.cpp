#include "InspectorContext.h"

namespace rv {

static Scene* s_CurrentScene = nullptr;
static entt::entity s_CurrentEntity = entt::null;
static TransformClipboardData s_TransformClipboard;

void InspectorContext::SetScene(Scene* scene)
{
	s_CurrentScene = scene;
}

Scene* InspectorContext::GetScene()
{
	return s_CurrentScene;
}

void InspectorContext::SetCurrentEntity(entt::entity entity)
{
	s_CurrentEntity = entity;
}

entt::entity InspectorContext::GetCurrentEntity()
{
	return s_CurrentEntity;
}

TransformClipboardData& InspectorContext::GetTransformClipboard()
{
	return s_TransformClipboard;
}

} // namespace rv
