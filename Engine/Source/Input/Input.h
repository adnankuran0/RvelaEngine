#pragma once

#include "glm/glm.hpp"
#include "KeyCodes.h"
#include "MouseCodes.h"
#include <vector>
#include <array>

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

	static void BeginFrame() noexcept;
	static void Update() noexcept;

	static void OnKeyPressed(int key) noexcept;
	static void OnKeyReleased(int key) noexcept;
	static void OnMouseButtonPressed(int button) noexcept;
	static void OnMouseButtonReleased(int button) noexcept;
	static void OnMouseMoved(float xpos, float ypos) noexcept;
	static void OnMouseScrolled(float xOffset, float yOffset) noexcept;
	static void OnCharTyped(unsigned int codepoint) noexcept;
	static const std::vector<uint32_t>& GetTypedCharacters() noexcept;

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
	static void OnFocusChanged(bool focused) noexcept;

	static glm::vec2 GetMousePosition() noexcept;

	static void SetViewportMousePos(const glm::vec2& pos) noexcept;
	static glm::vec2 GetViewportMousePosition(float viewportWidth = 1920.0f, float viewportHeight = 1080.0f) noexcept;

	static bool IsMouseOverUI() noexcept;
	static void SetMouseOverUI(bool state) noexcept;

	static inline float GetMouseX() noexcept {
		return s_CurrentMousePosition.x;
	}

	static inline float GetMouseY() noexcept {
		return s_CurrentMousePosition.y;
	}

	static glm::vec2 GetMouseDelta() noexcept;
	static float GetMouseDeltaX() noexcept;
	static float GetMouseDeltaY() noexcept;

	static glm::vec2 GetMouseScrollDelta() noexcept;
	static float GetMouseScrollX() noexcept;
	static float GetMouseScrollY() noexcept;

private:
	static glm::vec2 s_CurrentMousePosition;
	static glm::vec2 s_LastMousePosition;
	static glm::vec2 s_MouseDelta;
	static glm::vec2 s_AccumulatedMouseDelta;
	static glm::vec2 s_CurrentScrollDelta;
	static glm::vec2 s_AccumulatedScrollDelta;
	static std::vector<uint32_t> s_TypedCharacters;
	static bool s_FirstMouse;
	static bool s_MouseMovedThisFrame;
	static bool s_GameplayInputEnabled;
	static MouseMode s_RequestedMouseMode;
	static MouseMode s_ActiveMouseMode;
	static void ApplyMouseMode(MouseMode mode) noexcept;
	static std::array<bool, 512> s_CurrentKeyState;
	static std::array<bool, 512> s_PreviousKeyState;
	static std::array<bool, 16> s_CurrentMouseButtonState;
	static std::array<bool, 16> s_PreviousMouseButtonState;
};

}
