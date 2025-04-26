#pragma once

#include "glm/glm.hpp"
#include "KeyCodes.h"
#include "MouseCodes.h"

/**
 * @brief A utility class for handling user input from keyboard and mouse devices.
 *
 * The Input class provides a static interface to query the state of keyboard keys
 * and mouse buttons, as well as retrieve the mouse cursor position. It is designed
 * to facilitate input handling in the game engine without requiring instantiation.
 */
class Input
{
public:
	
	static bool IsKeyPressed(KeyCode key) noexcept;

	static bool IsMouseButtonPressed(MouseCode button) noexcept;

	static glm::vec2 GetMousePosition() noexcept;

	/**
	 * @brief Retrieves the x-coordinate of the current mouse cursor position.
	 *
	 * Uses the cached mouse position to avoid redundant GLFW calls.
	 *
	 * @return float The x-coordinate of the mouse cursor.
	 */
	inline float GetMouseX() noexcept {
		return s_LastMousePosition.x;
	}

	/**
	 * @brief Retrieves the y-coordinate of the current mouse cursor position.
	 *
	 * Uses the cached mouse position to avoid redundant GLFW calls.
	 *
	 * @return float The y-coordinate of the mouse cursor.
	 */
	inline float GetMouseY() noexcept {
		return s_LastMousePosition.y;
	}

private:
	static glm::vec2 s_LastMousePosition;
};