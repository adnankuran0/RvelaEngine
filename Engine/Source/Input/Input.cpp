#include "rvelapch.h"
#include "Core/Engine.h"
#include "Input.h"
#include "Event/EventManager.h"
#include "Event/MouseEvents.h"

using namespace rv;

glm::vec2 Input::s_CurrentMousePosition = { 0.0f, 0.0f };
glm::vec2 Input::s_LastMousePosition = { 0.0f, 0.0f };
glm::vec2 Input::s_MouseDelta = { 0.0f, 0.0f };
glm::vec2 Input::s_AccumulatedMouseDelta = { 0.0f, 0.0f };
glm::vec2 Input::s_CurrentScrollDelta = { 0.0f, 0.0f };
glm::vec2 Input::s_AccumulatedScrollDelta = { 0.0f, 0.0f };
std::vector<uint32_t> Input::s_TypedCharacters;
bool Input::s_FirstMouse = true;
bool Input::s_MouseMovedThisFrame = false;
bool Input::s_GameplayInputEnabled = true;
Input::MouseMode Input::s_RequestedMouseMode = Input::MouseMode::VISIBLE;
Input::MouseMode Input::s_ActiveMouseMode = Input::MouseMode::VISIBLE;

std::array<bool, 512> Input::s_CurrentKeyState = {};
std::array<bool, 512> Input::s_PreviousKeyState = {};
std::array<bool, 16> Input::s_CurrentMouseButtonState = {};
std::array<bool, 16> Input::s_PreviousMouseButtonState = {};

void Input::BeginFrame() noexcept
{
    s_CurrentScrollDelta = s_AccumulatedScrollDelta;
    s_AccumulatedScrollDelta = { 0.0f, 0.0f };
    s_TypedCharacters.clear();

    if (s_FirstMouse)
    {
        GLFWwindow* window = Engine::Get() ? Engine::Get()->GetWindow().GetGLFWWindow() : nullptr;
        if (window)
        {
            double xPos = 0.0, yPos = 0.0;
            glfwGetCursorPos(window, &xPos, &yPos);
            s_CurrentMousePosition = { static_cast<float>(xPos), static_cast<float>(yPos) };
            s_LastMousePosition = s_CurrentMousePosition;
            s_AccumulatedMouseDelta = { 0.0f, 0.0f };
            s_MouseDelta = { 0.0f, 0.0f };
            s_FirstMouse = false;
        }
    }
    else
    {
        s_MouseDelta = s_GameplayInputEnabled ? s_AccumulatedMouseDelta : glm::vec2(0.0f);
        s_AccumulatedMouseDelta = { 0.0f, 0.0f };
        s_LastMousePosition = s_CurrentMousePosition;
    }

    if (s_MouseMovedThisFrame)
    {
        EventManager::Push<MouseMovedEvent>(s_CurrentMousePosition.x, s_CurrentMousePosition.y);
        s_MouseMovedThisFrame = false;
    }
}

void Input::OnMouseMoved(float xpos, float ypos) noexcept
{
    glm::vec2 currentPos = { xpos, ypos };
    if (s_FirstMouse)
    {
        s_CurrentMousePosition = currentPos;
        s_LastMousePosition = currentPos;
        s_AccumulatedMouseDelta = { 0.0f, 0.0f };
        s_MouseDelta = { 0.0f, 0.0f };
        s_FirstMouse = false;
    }
    else
    {
        s_AccumulatedMouseDelta += (currentPos - s_CurrentMousePosition);
        s_CurrentMousePosition = currentPos;
    }
    s_MouseMovedThisFrame = true;
}

void Input::OnKeyPressed(int key) noexcept
{
    if (key >= 0 && key < 512)
        s_CurrentKeyState[key] = true;
}

void Input::OnKeyReleased(int key) noexcept
{
    if (key >= 0 && key < 512)
        s_CurrentKeyState[key] = false;
}

void Input::OnMouseButtonPressed(int button) noexcept
{
    if (button >= 0 && button < 16)
        s_CurrentMouseButtonState[button] = true;
}

void Input::OnMouseButtonReleased(int button) noexcept
{
    if (button >= 0 && button < 16)
        s_CurrentMouseButtonState[button] = false;
}

void Input::OnMouseScrolled(float xOffset, float yOffset) noexcept
{
    s_AccumulatedScrollDelta += glm::vec2(xOffset, yOffset);
}

void Input::OnCharTyped(unsigned int codepoint) noexcept
{
    if (s_GameplayInputEnabled)
    {
        s_TypedCharacters.push_back(codepoint);
    }
}

const std::vector<uint32_t>& Input::GetTypedCharacters() noexcept
{
    return s_TypedCharacters;
}

glm::vec2 Input::GetMouseDelta() noexcept
{
    return s_GameplayInputEnabled ? s_MouseDelta : glm::vec2(0.0f);
}

float Input::GetMouseDeltaX() noexcept
{
    return GetMouseDelta().x;
}

float Input::GetMouseDeltaY() noexcept
{
    return GetMouseDelta().y;
}

glm::vec2 Input::GetMouseScrollDelta() noexcept
{
    return s_GameplayInputEnabled ? s_CurrentScrollDelta : glm::vec2(0.0f);
}

float Input::GetMouseScrollX() noexcept
{
    return GetMouseScrollDelta().x;
}

float Input::GetMouseScrollY() noexcept
{
    return GetMouseScrollDelta().y;
}

void Input::Update() noexcept
{
    s_PreviousKeyState = s_CurrentKeyState;
    s_PreviousMouseButtonState = s_CurrentMouseButtonState;
}

bool Input::IsKeyPressed(KeyCode key) noexcept {
    int k = static_cast<int>(key);
    return (k >= 0 && k < 512) ? s_CurrentKeyState[k] : false;
}

bool Input::IsKeyJustPressed(KeyCode key) noexcept
{
    int k = static_cast<int>(key);
    return (k >= 0 && k < 512) ? (s_CurrentKeyState[k] && !s_PreviousKeyState[k]) : false;
}

bool Input::IsKeyJustReleased(KeyCode key) noexcept
{
    int k = static_cast<int>(key);
    return (k >= 0 && k < 512) ? (!s_CurrentKeyState[k] && s_PreviousKeyState[k]) : false;
}

bool Input::IsMouseButtonPressed(MouseCode button) noexcept {
    int b = static_cast<int>(button);
    return (b >= 0 && b < 16) ? s_CurrentMouseButtonState[b] : false;
}

bool Input::IsMouseButtonJustPressed(MouseCode button) noexcept
{
    int b = static_cast<int>(button);
    return (b >= 0 && b < 16) ? (s_CurrentMouseButtonState[b] && !s_PreviousMouseButtonState[b]) : false;
}

bool Input::IsMouseButtonJustReleased(MouseCode button) noexcept
{
    int b = static_cast<int>(button);
    return (b >= 0 && b < 16) ? (!s_CurrentMouseButtonState[b] && s_PreviousMouseButtonState[b]) : false;
}

void rv::Input::SetMouseMode(MouseMode mode) noexcept
{
	s_RequestedMouseMode = mode;
	s_FirstMouse = true;
	ApplyMouseMode(s_GameplayInputEnabled ? mode : MouseMode::VISIBLE);
}

void Input::SetGameplayInputEnabled(bool enabled) noexcept
{
	if (s_GameplayInputEnabled == enabled)
		return;

	s_GameplayInputEnabled = enabled;
	s_FirstMouse = true;
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

    GLFWwindow* window = Engine::Get() ? Engine::Get()->GetWindow().GetGLFWWindow() : nullptr;
    if (window)
		glfwSetInputMode(window, GLFW_CURSOR, glfwMode);
	s_ActiveMouseMode = mode;
}

void Input::OnFocusChanged(bool focused) noexcept
{
    s_CurrentKeyState.fill(false);
    s_PreviousKeyState.fill(false);
    s_CurrentMouseButtonState.fill(false);
    s_PreviousMouseButtonState.fill(false);
    s_MouseDelta = { 0.0f, 0.0f };
    s_AccumulatedMouseDelta = { 0.0f, 0.0f };
    s_CurrentScrollDelta = { 0.0f, 0.0f };
    s_AccumulatedScrollDelta = { 0.0f, 0.0f };
    s_TypedCharacters.clear();
    s_FirstMouse = true;
    s_MouseMovedThisFrame = false;

    if (!focused)
    {
        ApplyMouseMode(MouseMode::VISIBLE);
    }
    else
    {
        ApplyMouseMode(s_GameplayInputEnabled ? s_RequestedMouseMode : MouseMode::VISIBLE);
    }
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

    WindowSize winSize = Engine::Get() ? Engine::Get()->GetWindow().GetSize() : WindowSize{ 0, 0 };
    float winW = winSize.width > 0 ? (float)winSize.width : viewportWidth;
    float winH = winSize.height > 0 ? (float)winSize.height : viewportHeight;

    return glm::vec2(s_CurrentMousePosition.x / winW * viewportWidth, s_CurrentMousePosition.y / winH * viewportHeight);
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
    return s_CurrentMousePosition;
}
