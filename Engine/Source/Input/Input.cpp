#include "rvelapch.h"
#include "Core/Engine.h"
#include "Input.h"

using namespace rv;

// Cache for the last known mouse position to avoid redundant GLFW calls
glm::vec2 Input::s_LastMousePosition = { 0.0f, 0.0f };
// Cache previosly pressed keys and buttons
std::unordered_map<KeyCode, bool> Input::s_PreviousKeyState;
std::unordered_map<MouseCode, bool> Input::s_PreviousMouseButtonState;

void Input::Update() noexcept
{
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        KeyCode code = static_cast<KeyCode>(key);
        s_PreviousKeyState[code] = IsKeyPressed(code);
    }

    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        MouseCode code = static_cast<MouseCode>(button);
        s_PreviousMouseButtonState[code] = IsMouseButtonPressed(code);
    }
}

bool Input::IsKeyPressed(KeyCode key) noexcept {
    GLFWwindow* window = Engine::Get()->GetWindow().GetGLFWWindow();
    if (!window) {
        return false;
    }

    return glfwGetKey(window, static_cast<int>(key)) == GLFW_PRESS;
}

bool Input::IsKeyJustPressed(KeyCode key) noexcept
{
    bool isPressed = IsKeyPressed(key);
    bool wasPressed = s_PreviousKeyState[key];
    return isPressed && !wasPressed;
}

bool Input::IsKeyJustReleased(KeyCode key) noexcept
{
    bool isPressed = IsKeyPressed(key);
    bool wasPressed = s_PreviousKeyState[key];
    return !isPressed && wasPressed;
}

bool Input::IsMouseButtonPressed(MouseCode button) noexcept {
    auto* window = Engine::Get()->GetWindow().GetGLFWWindow();
    if (!window) {
        return false;
    }

    return glfwGetMouseButton(window, static_cast<int>(button)) == GLFW_PRESS;
}

bool Input::IsMouseButtonJustPressed(MouseCode button) noexcept
{
    bool isPressed = IsMouseButtonPressed(button);
    bool wasPressed = s_PreviousMouseButtonState[button];
    return isPressed && !wasPressed;
}

bool Input::IsMouseButtonJustReleased(MouseCode button) noexcept
{
    bool isPressed = IsMouseButtonPressed(button);
    bool wasPressed = s_PreviousMouseButtonState[button];
    return !isPressed && wasPressed;
}

glm::vec2 Input::GetMousePosition() noexcept {
    auto* window = Engine::Get()->GetWindow().GetGLFWWindow();
    if (!window) {
        return { 0.0f, 0.0f };
    }

    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    s_LastMousePosition = { static_cast<float>(xPos), static_cast<float>(yPos) };
    return s_LastMousePosition;
}
