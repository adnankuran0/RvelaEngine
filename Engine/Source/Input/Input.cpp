#include "rvelapch.h"
#include "Core/Engine.h"
#include "Input.h"

using namespace rv;

// Cache for the last known mouse position to avoid redundant GLFW calls
glm::vec2 Input::s_LastMousePosition = { 0.0f, 0.0f };
bool Input::s_GameplayInputEnabled = true;
Input::MouseMode Input::s_RequestedMouseMode = Input::MouseMode::VISIBLE;
Input::MouseMode Input::s_ActiveMouseMode = Input::MouseMode::VISIBLE;
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

void rv::Input::SetMouseMode(MouseMode mode) noexcept
{
	s_RequestedMouseMode = mode;
	ApplyMouseMode(s_GameplayInputEnabled ? mode : MouseMode::VISIBLE);
}

void Input::SetGameplayInputEnabled(bool enabled) noexcept
{
	if (s_GameplayInputEnabled == enabled)
		return;

	s_GameplayInputEnabled = enabled;
	ApplyMouseMode(enabled ? s_RequestedMouseMode : MouseMode::VISIBLE);
}

bool Input::IsGameplayInputEnabled() noexcept
{
	return s_GameplayInputEnabled;
}

bool Input::IsMouseCaptured() noexcept
{
	return s_ActiveMouseMode == MouseMode::CAPTURED;
}

void Input::ApplyMouseMode(MouseMode mode) noexcept
{
    int glfwMode;
    switch (mode)
    {
    case rv::Input::MouseMode::VISIBLE:
        glfwMode = GLFW_CURSOR_NORMAL;
        break;
    case rv::Input::MouseMode::HIDDEN:
        glfwMode = GLFW_CURSOR_HIDDEN;
        break;
    case rv::Input::MouseMode::CAPTURED:
        glfwMode = GLFW_CURSOR_DISABLED;
        break;
    default:
        glfwMode = GLFW_CURSOR_NORMAL;
        break;
    }

    GLFWwindow* window = Engine::Get()->GetWindow().GetGLFWWindow();
    if (window)
		glfwSetInputMode(window, GLFW_CURSOR, glfwMode);
	s_ActiveMouseMode = mode;
}

// Viewport relative mouse position storage
static glm::vec2 s_ViewportMousePos = { -1.0f, -1.0f };
static bool s_ViewportMousePosValid = false;

void Input::SetViewportMousePos(const glm::vec2& pos) noexcept
{
    s_ViewportMousePos = pos;
    s_ViewportMousePosValid = true;
}

glm::vec2 Input::GetViewportMousePosition(float viewportWidth, float viewportHeight) noexcept
{
    if (s_ViewportMousePosValid)
    {
        return s_ViewportMousePos;
    }

    auto* window = Engine::Get()->GetWindow().GetGLFWWindow();
    if (!window) {
        return { 0.0f, 0.0f };
    }

    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    WindowSize winSize = Engine::Get()->GetWindow().GetSize();
    float winW = winSize.width > 0 ? (float)winSize.width : viewportWidth;
    float winH = winSize.height > 0 ? (float)winSize.height : viewportHeight;

    return glm::vec2((float)xPos / winW * viewportWidth, (float)yPos / winH * viewportHeight);
}

static bool s_IsMouseOverUI = false;

void Input::SetMouseOverUI(bool state) noexcept
{
    s_IsMouseOverUI = state;
}

bool Input::IsMouseOverUI() noexcept
{
	return s_GameplayInputEnabled && s_IsMouseOverUI;
}

glm::vec2 Input::GetMousePosition() noexcept {
	if (!s_GameplayInputEnabled)
		return s_LastMousePosition;

    auto* window = Engine::Get()->GetWindow().GetGLFWWindow();
    if (!window) {
        return { 0.0f, 0.0f };
    }

    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    s_LastMousePosition = { static_cast<float>(xPos), static_cast<float>(yPos) };
    return s_LastMousePosition;
}
