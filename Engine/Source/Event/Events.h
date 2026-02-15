#pragma once

namespace rv {

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

}