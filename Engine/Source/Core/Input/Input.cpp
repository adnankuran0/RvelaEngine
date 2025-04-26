#include "rvelapch.h"
#include "../Engine.h"
#include "Input.h"

// Cache for the last known mouse position to avoid redundant GLFW calls
glm::vec2 Input::s_LastMousePosition = { 0.0f, 0.0f };

/**
 * @brief Checks if a specific keyboard key is currently pressed.
 *
 * Queries the state of the specified key using GLFW and returns whether it is pressed.
 *
 * @param key The key code to check.
 * @return bool True if the key is pressed, false otherwise. Returns false if the window is invalid.
 */
bool Input::IsKeyPressed(KeyCode key) noexcept {
    auto* window = Engine::Get();
    if (!window || !window->GetWindow()) {
        return false;
    }

    GLFWwindow* glfwWindow = window->GetWindow()->GetGLFWWindow();
    if (!glfwWindow) {
        return false;
    }

    return glfwGetKey(glfwWindow, static_cast<int>(key)) == GLFW_PRESS;
}

/**
 * @brief Checks if a specific mouse button is currently pressed.
 *
 * Queries the state of the specified mouse button using GLFW and returns whether it is pressed.
 *
 * @param button The mouse button code to check.
 * @return bool True if the button is pressed, false otherwise. Returns false if the window is invalid.
 */
bool Input::IsMouseButtonPressed(MouseCode button) noexcept {
    auto* window = Engine::Get();
    if (!window || !window->GetWindow()) {
        return false;
    }

    GLFWwindow* glfwWindow = window->GetWindow()->GetGLFWWindow();
    if (!glfwWindow) {
        return false;
    }

    return glfwGetMouseButton(glfwWindow, static_cast<int>(button)) == GLFW_PRESS;
}

/**
 * @brief Retrieves the current mouse cursor position as a 2D vector.
 *
 * Queries the mouse cursor position using GLFW and caches the result to avoid redundant calls.
 *
 * @return glm::vec2 The (x, y) coordinates of the mouse cursor. Returns (0, 0) if the window is invalid.
 */
glm::vec2 Input::GetMousePosition() noexcept {
    auto* window = Engine::Get();
    if (!window || !window->GetWindow()) {
        return { 0.0f, 0.0f };
    }

    GLFWwindow* glfwWindow = window->GetWindow()->GetGLFWWindow();
    if (!glfwWindow) {
        return { 0.0f, 0.0f };
    }

    double xPos, yPos;
    glfwGetCursorPos(glfwWindow, &xPos, &yPos);
    s_LastMousePosition = { static_cast<float>(xPos), static_cast<float>(yPos) };
    return s_LastMousePosition;
}
