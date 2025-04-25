#pragma once

#include "Event.h"
#include "../Input/KeyCodes.h"

/**
 * @brief Base class for keyboard-related events.
 *
 * The KeyEvent class provides common functionality for keyboard events, such as
 * storing and retrieving the associated key code. Derived classes specify the
 * specific event type (e.g., key pressed or released).
 */
class KeyEvent : public Event {
public:
    /**
     * @brief Constructs a KeyEvent with the specified key code.
     *
     * @param keyCode The key code representing the keyboard key.
     */
    explicit KeyEvent(KeyCode keyCode) noexcept : m_KeyCode(keyCode) {}

    /**
     * @brief Retrieves the key code associated with the event.
     *
     * @return KeyCode The key code of the keyboard key.
     */
    KeyCode GetKeyCode() const noexcept { return m_KeyCode; }

protected:
    KeyCode m_KeyCode; ///< The key code representing the keyboard key.
};

/**
 * @brief Event triggered when a keyboard key is pressed.
 *
 * The KeyPressedEvent class represents a keyboard key press event, providing
 * access to the key code and event type.
 */
class KeyPressedEvent : public KeyEvent {
public:
    /**
     * @brief Constructs a KeyPressedEvent with the specified key code.
     *
     * @param keyCode The key code representing the pressed key.
     */
    explicit KeyPressedEvent(KeyCode keyCode) noexcept : KeyEvent(keyCode) {}

    /**
     * @brief Retrieves the type of the event.
     *
     * @return EventType The event type (KeyPressed).
     */
    EventType GetEventType() const noexcept override { return EventType::KeyPressed; }

    /**
     * @brief Retrieves the name of the event for debugging or logging.
     *
     * @return const char* The name of the event ("KeyPressed").
     */
    const char* GetName() const noexcept override { return EVENT_NAME; }

    /**
     * @brief Retrieves the static event type.
     *
     * @return EventType The static event type (KeyPressed).
     */
    static EventType GetStaticType() noexcept { return EventType::KeyPressed; }

private:
    static constexpr const char* EVENT_NAME = "KeyPressed"; ///< Static event name for debugging.
};

/**
 * @brief Event triggered when a keyboard key is released.
 *
 * The KeyReleasedEvent class represents a keyboard key release event, providing
 * access to the key code and event type.
 */
class KeyReleasedEvent : public KeyEvent {
public:
    /**
     * @brief Constructs a KeyReleasedEvent with the specified key code.
     *
     * @param keyCode The key code representing the released key.
     */
    explicit KeyReleasedEvent(KeyCode keyCode) noexcept : KeyEvent(keyCode) {}

    /**
     * @brief Retrieves the type of the event.
     *
     * @return EventType The event type (KeyReleased).
     */
    EventType GetEventType() const noexcept override { return EventType::KeyReleased; }

    /**
     * @brief Retrieves the name of the event for debugging or logging.
     *
     * @return const char* The name of the event ("KeyReleased").
     */
    const char* GetName() const noexcept override { return EVENT_NAME; }

    /**
     * @brief Retrieves the static event type.
     *
     * @return EventType The static event type (KeyReleased).
     */
    static EventType GetStaticType() noexcept { return EventType::KeyReleased; }

private:
    static constexpr const char* EVENT_NAME = "KeyReleased"; ///< Static event name for debugging.
};