#include "rvelapch.h"
#include "ScriptableEntity.h"
#include "Scene/Entity.h"

void ScriptableEntity::OnUpdate(float dt)
{
	TransformComponent& tc = entity->GetComponent<TransformComponent>();
	auto pos = tc.GetPosition();
	pos.x += 10.0f * dt;
	tc.SetPosition(pos);
}