#pragma once

#include "ScriptableEntity.h"

class TestScript : public ScriptableEntity
{
public:
	void OnUpdate(float dt) override;
private:
	glm::vec3 velocity;

};