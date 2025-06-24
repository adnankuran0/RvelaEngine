#pragma once

#include "Event.h"
#include "Input/KeyCodes.h"

class KeyEvent : public Event {
public:
    KeyEvent(KeyCode keyCode) noexcept : m_KeyCode(keyCode) {}

    inline KeyCode GetKeyCode() const noexcept { return m_KeyCode; }

protected:
    KeyCode m_KeyCode;
};

class KeyPressedEvent : public KeyEvent {
public:
    inline explicit KeyPressedEvent(KeyCode keyCode) noexcept : KeyEvent(keyCode) {}

    inline EventType GetEventType() const noexcept override { return EventType::KeyPressed; }

    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    inline static EventType GetStaticType() noexcept { return EventType::KeyPressed; }

private:
    static constexpr const char* EVENT_NAME = "KeyPressed"; 
};

class KeyReleasedEvent : public KeyEvent {
public:
    inline explicit KeyReleasedEvent(KeyCode keyCode) noexcept : KeyEvent(keyCode) {}

    inline EventType GetEventType() const noexcept override { return EventType::KeyReleased; }

    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    inline static EventType GetStaticType() noexcept { return EventType::KeyReleased; }

private:
    static constexpr const char* EVENT_NAME = "KeyReleased";
};