#pragma once

#include "Event.h"
#include "Input/MouseCodes.h"
#include "glm/glm.hpp"

class MouseButtonEvent : public Event {
public:
    
    MouseButtonEvent(MouseCode mouseCode) noexcept : m_MouseCode(mouseCode) {}

    inline MouseCode GetMouseCode() const noexcept { return m_MouseCode; }

protected:
    MouseCode m_MouseCode; ///< The mouse button code.
};

class MouseButtonPressedEvent : public MouseButtonEvent {
public:
    
    inline explicit MouseButtonPressedEvent(MouseCode mouseCode) noexcept : MouseButtonEvent(mouseCode) {}

    inline EventType GetEventType() const noexcept override { return EventType::MouseButtonPressed; }

    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    inline static EventType GetStaticType() noexcept { return EventType::MouseButtonPressed; }

private:
    static constexpr const char* EVENT_NAME = "MouseButtonPressed"; ///< Static event name for debugging.
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
public:

    inline explicit MouseButtonReleasedEvent(MouseCode mouseCode) noexcept : MouseButtonEvent(mouseCode) {}

    inline EventType GetEventType() const noexcept override { return EventType::MouseButtonReleased; }

    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    inline static EventType GetStaticType() noexcept { return EventType::MouseButtonReleased; }

private:
    static constexpr const char* EVENT_NAME = "MouseButtonReleased"; ///< Static event name for debugging.
};

class MouseMovedEvent : public Event {
public:

    inline MouseMovedEvent(float x, float y) noexcept : m_X(x), m_Y(y) {}

    inline EventType GetEventType() const noexcept override { return EventType::MouseMoved; }

    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    inline float GetX() const noexcept { return m_X; }

    inline float GetY() const noexcept { return m_Y; }

    inline glm::vec2 GetPos() const noexcept { return glm::vec2(m_X, m_Y); }

    inline static EventType GetStaticType() noexcept { return EventType::MouseMoved; }

private:
    float m_X, m_Y; ///< The x and y coordinates of the cursor position.
    static constexpr const char* EVENT_NAME = "MouseMoved"; ///< Static event name for debugging.
};

class MouseScrolledEvent : public Event {
public:

    inline MouseScrolledEvent(float xOffset, float yOffset) noexcept : m_XOffset(xOffset), m_YOffset(yOffset) {}

    inline EventType GetEventType() const noexcept override { return EventType::MouseScrolled; }

    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    inline float GetXOffset() const noexcept { return m_XOffset; }

    inline float GetYOffset() const noexcept { return m_YOffset; }

    inline static EventType GetStaticType() noexcept { return EventType::MouseScrolled; }

private:
    float m_XOffset, m_YOffset; 
    static constexpr const char* EVENT_NAME = "MouseScrolled"; 
};