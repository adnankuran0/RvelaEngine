#include "rvelapch.h"
#include "../Engine.h"
#include "Input.h"


// Cache for the last known mouse position to avoid redundant GLFW calls
glm::vec2 Input::s_LastMousePosition = { 0.0f, 0.0f };

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
