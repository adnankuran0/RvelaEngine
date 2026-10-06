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
	static void SetGameplayInputEnabled(bool enabled) noexcept;
	static bool IsGameplayInputEnabled() noexcept;
	static bool IsMouseCaptured() noexcept;

	static glm::vec2 GetMousePosition() noexcept;

	static void SetViewportMousePos(const glm::vec2& pos) noexcept;
	static glm::vec2 GetViewportMousePosition(float viewportWidth = 1920.0f, float viewportHeight = 1080.0f) noexcept;

	static bool IsMouseOverUI() noexcept;
	static void SetMouseOverUI(bool state) noexcept;

	inline float GetMouseX() noexcept {
		return s_LastMousePosition.x;
	}

	inline float GetMouseY() noexcept {
		return s_LastMousePosition.y;
	}

private:
	static glm::vec2 s_LastMousePosition;
	static bool s_GameplayInputEnabled;
	static MouseMode s_RequestedMouseMode;
	static MouseMode s_ActiveMouseMode;
	static void ApplyMouseMode(MouseMode mode) noexcept;
	static std::unordered_map<KeyCode, bool> s_PreviousKeyState;
	static std::unordered_map<MouseCode, bool> s_PreviousMouseButtonState;
};

}
