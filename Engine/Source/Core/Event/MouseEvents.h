#pragma once
#include "Event.h"
#include "../Input/MouseCodes.h"
#include "glm/glm.hpp"

class MouseButtonPressedEvent : public Event {
public:
    MouseButtonPressedEvent (MouseCode mousecode) : m_MouseCode(mousecode) {}

    EventType GetEventType() const override { return EventType::MouseButtonPressed; }
    const char* GetName() const override { return "MouseButtonPressed"; }
    MouseCode GetMouseCode() const { return m_MouseCode; }



    static EventType GetStaticType() { return EventType::MouseButtonPressed; }

private:
    MouseCode m_MouseCode;
};


class MouseButtonReleasedEvent : public Event {
public:
    MouseButtonReleasedEvent(MouseCode mousecode) : m_MouseCode(mousecode) {}

    EventType GetEventType() const override { return EventType::MouseButtonReleased; }
    const char* GetName() const override { return "MouseButtonReleased"; }
    MouseCode GetMouseCode() const { return m_MouseCode; }



    static EventType GetStaticType() { return EventType::MouseButtonReleased; }

private:
    MouseCode m_MouseCode;
};


class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(float x, float y)
        : m_X(x), m_Y(y) {}

    EventType GetEventType() const override { return EventType::MouseMoved; }
    const char* GetName() const override { return "MouseMoved"; }
    double GetX() const { return m_X; }
    double GetY() const { return m_Y; }
    glm::vec2 GetPos() const { return glm::vec2(m_X, m_Y); }

    static EventType GetStaticType() { return EventType::MouseMoved; }

private:
    double m_X, m_Y;
};

class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float xOffset, float yOffset)
        : m_XOffset(xOffset), m_YOffset(yOffset) {}

    EventType GetEventType() const override { return EventType::MouseScrolled; }
    const char* GetName() const override { return "MouseScrolled"; }
    float GetXOffset() const { return m_XOffset; }
    float GetOffset() const { return m_YOffset; }

    static EventType GetStaticType() { return EventType::MouseScrolled; }

private:
    float m_XOffset, m_YOffset;
};