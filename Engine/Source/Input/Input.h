#pragma once

#include "glm/glm.hpp"
#include "KeyCodes.h"
#include "MouseCodes.h"

namespace rv { 

class Input
{
public:

	enum class MouseMode
	{
		VISIBLE,
		HIDDEN,
		CAPTURED
	};

	static void Update() noexcept;

	static bool IsKeyPressed(KeyCode key) noexcept;

	static bool IsKeyJustPressed(KeyCode key) noexcept;

	static bool IsKeyJustReleased(KeyCode key) noexcept;

	static bool IsMouseButtonPressed(MouseCode button) noexcept;

	static bool IsMouseButtonJustPressed(MouseCode button) noexcept;

	static bool IsMouseButtonJustReleased(MouseCode button) noexcept;

	static void SetMouseMode(MouseMode mode) noexcept;

	static glm::vec2 GetMousePosition() noexcept;

	inline float GetMouseX() noexcept {
		return s_LastMousePosition.x;
	}

	inline float GetMouseY() noexcept {
		return s_LastMousePosition.y;
	}

private:
	static glm::vec2 s_LastMousePosition;
	static std::unordered_map<KeyCode, bool> s_PreviousKeyState;
	static std::unordered_map<MouseCode, bool> s_PreviousMouseButtonState;
};

}