#pragma once

#include "glm/glm.hpp"
#include "KeyCodes.h"
#include "MouseCodes.h"

class Input
{
public:
	
	static bool IsKeyPressed(KeyCode key) noexcept;

	static bool IsMouseButtonPressed(MouseCode button) noexcept;

	static glm::vec2 GetMousePosition() noexcept;

	inline float GetMouseX() noexcept {
		return s_LastMousePosition.x;
	}

	inline float GetMouseY() noexcept {
		return s_LastMousePosition.y;
	}

private:
	static glm::vec2 s_LastMousePosition;
};