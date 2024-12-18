#pragma once
#include "Event.h"
#include "../Input/KeyCodes.h"

class KeyPressedEvent : public Event {
public:
    KeyPressedEvent(KeyCode keycode) : m_Keycode(keycode) {}

    EventType GetEventType() const override { return EventType::KeyPressed; }
    const char* GetName() const override { return "KeyPressed"; }
    KeyCode GetKeycode() const { return m_Keycode; }
    


    static EventType GetStaticType() { return EventType::KeyPressed; }

private:
    KeyCode m_Keycode;
};