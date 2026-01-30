#include "rvelapch.h"
#include "TestScript.h"
#include "Scene/Entity.h"
#include "Input/Input.h"

void TestScript::OnUpdate(float dt)
{
	TransformComponent& tc = entity->GetComponent<TransformComponent>();
	auto pos = tc.GetPosition();
	velocity.y -= 10.0f * dt;

	if (Input::IsKeyJustPressed(KeyCode::Space))
		velocity.y = 10.0f;

	pos += velocity * dt;

	tc.SetPosition(pos);
}
