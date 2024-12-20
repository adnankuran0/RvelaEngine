#pragma once

enum class EventType {
    None = 0,
    KeyPressed,
    KeyReleased,
    MouseMoved,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseScrolled,
    WindowResized
};