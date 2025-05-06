#pragma once

#include "Event.h"

class WindowResizedEvent : public Event {
public:
    
    inline explicit WindowResizedEvent(unsigned int width, unsigned int height) noexcept
        : m_WindowWidth(width > 0 ? width : 1), m_WindowHeight(height > 0 ? height : 1) {
    }
    
    inline EventType GetEventType() const noexcept override { return EventType::WindowResized; }

    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    inline unsigned int GetWidth() const noexcept { return m_WindowWidth; }

    inline unsigned int GetHeight() const noexcept { return m_WindowHeight; }
    
    inline static EventType GetStaticType() noexcept { return EventType::WindowResized; }

private:
    unsigned int m_WindowWidth;  
    unsigned int m_WindowHeight; 
    static constexpr const char* EVENT_NAME = "WindowResized"; 
};