#pragma once
#include "Event.h"
#include <vector>
#include <filesystem>

namespace rv { 

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

class FileDroppedEvent : public Event 
{
public:
    explicit FileDroppedEvent(int pathCount, const char* paths[], float screenX = -1.0f, float screenY = -1.0f) noexcept
        : m_ScreenX(screenX), m_ScreenY(screenY) {
        m_Paths.reserve(pathCount);
        for (int i = 0; i < pathCount; i++)
            m_Paths.emplace_back(paths[i]);
    }

    inline EventType GetEventType() const noexcept override { return EventType::FileDropped; }
    inline const char* GetName() const noexcept override { return EVENT_NAME; }
    inline static EventType GetStaticType() noexcept { return EventType::FileDropped; }

    const std::vector<std::filesystem::path>& GetPaths() const noexcept { return m_Paths; }
    size_t GetPathCount() const noexcept { return m_Paths.size(); }
    float GetScreenX() const noexcept { return m_ScreenX; }
    float GetScreenY() const noexcept { return m_ScreenY; }

private:
    std::vector<std::filesystem::path> m_Paths;
    float m_ScreenX = -1.0f;
    float m_ScreenY = -1.0f;
    static constexpr const char* EVENT_NAME = "FileDropped";
};

class WindowFocusEvent : public Event
{
public:
    explicit WindowFocusEvent(bool focused) noexcept
        : m_Focused(focused) {}

    inline EventType GetEventType() const noexcept override { return EventType::WindowFocus; }
    inline const char* GetName() const noexcept override { return EVENT_NAME; }
    inline static EventType GetStaticType() noexcept { return EventType::WindowFocus; }

    inline bool IsFocused() const noexcept { return m_Focused; }

private:
    bool m_Focused;
    static constexpr const char* EVENT_NAME = "WindowFocus";
};

}
