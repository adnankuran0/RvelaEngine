#pragma once
#include "Event.h"

class WindowResizedEvent : public Event {
public:
    WindowResizedEvent(int width, int height)
        : m_Width(width), m_Height(height) {}

    EventType GetEventType() const override { return EventType::WindowResized; }
    const char* GetName() const override { return "WindowResized"; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    static EventType GetStaticType() { return EventType::WindowResized; }

private:
    int m_Width, m_Height;
};