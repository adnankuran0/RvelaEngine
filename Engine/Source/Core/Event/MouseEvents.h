#pragma once

#include "Event.h"
#include "../Input/MouseCodes.h"
#include "glm/glm.hpp"

/**
 * @brief Base class for mouse button-related events.
 *
 * The MouseButtonEvent class provides common functionality for mouse button events,
 * such as storing and retrieving the associated mouse button code. Derived classes
 * specify the specific event type (e.g., button pressed or released).
 */
class MouseButtonEvent : public Event {
public:
    /**
     * @brief Constructs a MouseButtonEvent with the specified mouse button code.
     *
     * @param mouseCode The mouse button code representing the button.
     */
    MouseButtonEvent(MouseCode mouseCode) noexcept : m_MouseCode(mouseCode) {}

    /**
     * @brief Retrieves the mouse button code associated with the event.
     *
     * @return MouseCode The mouse button code.
     */
    inline MouseCode GetMouseCode() const noexcept { return m_MouseCode; }

protected:
    MouseCode m_MouseCode; ///< The mouse button code.
};

/**
 * @brief Event triggered when a mouse button is pressed.
 *
 * The MouseButtonPressedEvent class represents a mouse button press event, providing
 * access to the mouse button code and event type.
 */
class MouseButtonPressedEvent : public MouseButtonEvent {
public:
    /**
     * @brief Constructs a MouseButtonPressedEvent with the specified mouse button code.
     *
     * @param mouseCode The mouse button code representing the pressed button.
     */
    inline explicit MouseButtonPressedEvent(MouseCode mouseCode) noexcept : MouseButtonEvent(mouseCode) {}

    /**
     * @brief Retrieves the type of the event.
     *
     * @return EventType The event type (MouseButtonPressed).
     */
    inline EventType GetEventType() const noexcept override { return EventType::MouseButtonPressed; }

    /**
     * @brief Retrieves the name of the event for debugging or logging.
     *
     * @return const char* The name of the event ("MouseButtonPressed").
     */
    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    /**
     * @brief Retrieves the static event type.
     *
     * @return EventType The static event type (MouseButtonPressed).
     */
    inline static EventType GetStaticType() noexcept { return EventType::MouseButtonPressed; }

private:
    static constexpr const char* EVENT_NAME = "MouseButtonPressed"; ///< Static event name for debugging.
};

/**
 * @brief Event triggered when a mouse button is released.
 *
 * The MouseButtonReleasedEvent class represents a mouse button release event, providing
 * access to the mouse button code and event type.
 */
class MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    /**
     * @brief Constructs a MouseButtonReleasedEvent with the specified mouse button code.
     *
     * @param mouseCode The mouse button code representing the released button.
     */
    inline explicit MouseButtonReleasedEvent(MouseCode mouseCode) noexcept : MouseButtonEvent(mouseCode) {}

    /**
     * @brief Retrieves the type of the event.
     *
     * @return EventType The event type (MouseButtonReleased).
     */
    inline EventType GetEventType() const noexcept override { return EventType::MouseButtonReleased; }

    /**
     * @brief Retrieves the name of the event for debugging or logging.
     *
     * @return const char* The name of the event ("MouseButtonReleased").
     */
    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    /**
     * @brief Retrieves the static event type.
     *
     * @return EventType The static event type (MouseButtonReleased).
     */
    inline static EventType GetStaticType() noexcept { return EventType::MouseButtonReleased; }

private:
    static constexpr const char* EVENT_NAME = "MouseButtonReleased"; ///< Static event name for debugging.
};

/**
 * @brief Event triggered when the mouse cursor moves.
 *
 * The MouseMovedEvent class represents a mouse cursor movement event, providing
 * access to the cursor's position in 2D space.
 */
class MouseMovedEvent : public Event {
public:
    /**
     * @brief Constructs a MouseMovedEvent with the specified cursor position.
     *
     * @param x The x-coordinate of the cursor position.
     * @param y The y-coordinate of the cursor position.
     */
    inline MouseMovedEvent(float x, float y) noexcept : m_X(x), m_Y(y) {}

    /**
     * @brief Retrieves the type of the event.
     *
     * @return EventType The event type (MouseMoved).
     */
    inline EventType GetEventType() const noexcept override { return EventType::MouseMoved; }

    /**
     * @brief Retrieves the name of the event for debugging or logging.
     *
     * @return const char* The name of the event ("MouseMoved").
     */
    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    /**
     * @brief Retrieves the x-coordinate of the cursor position.
     *
     * @return float The x-coordinate.
     */
    inline float GetX() const noexcept { return m_X; }

    /**
     * @brief Retrieves the y-coordinate of the cursor position.
     *
     * @return float The y-coordinate.
     */
    inline float GetY() const noexcept { return m_Y; }

    /**
     * @brief Retrieves the cursor position as a 2D vector.
     *
     * @return glm::vec2 The cursor position (x, y).
     */
    inline glm::vec2 GetPos() const noexcept { return glm::vec2(m_X, m_Y); }

    /**
     * @brief Retrieves the static event type.
     *
     * @return EventType The static event type (MouseMoved).
     */
    inline static EventType GetStaticType() noexcept { return EventType::MouseMoved; }

private:
    float m_X, m_Y; ///< The x and y coordinates of the cursor position.
    static constexpr const char* EVENT_NAME = "MouseMoved"; ///< Static event name for debugging.
};

/**
 * @brief Event triggered when the mouse wheel is scrolled.
 *
 * The MouseScrolledEvent class represents a mouse wheel scroll event, providing
 * access to the scroll offsets along the x and y axes.
 */
class MouseScrolledEvent : public Event {
public:
    /**
     * @brief Constructs a MouseScrolledEvent with the specified scroll offsets.
     *
     * @param xOffset The scroll offset along the x-axis.
     * @param yOffset The scroll offset along the y-axis.
     */
    inline MouseScrolledEvent(float xOffset, float yOffset) noexcept : m_XOffset(xOffset), m_YOffset(yOffset) {}

    /**
     * @brief Retrieves the type of the event.
     *
     * @return EventType The event type (MouseScrolled).
     */
    inline EventType GetEventType() const noexcept override { return EventType::MouseScrolled; }

    /**
     * @brief Retrieves the name of the event for debugging or logging.
     *
     * @return const char* The name of the event ("MouseScrolled").
     */
    inline const char* GetName() const noexcept override { return EVENT_NAME; }

    /**
     * @brief Retrieves the scroll offset along the x-axis.
     *
     * @return float The x-axis scroll offset.
     */
    inline float GetXOffset() const noexcept { return m_XOffset; }

    /**
     * @brief Retrieves the scroll offset along the y-axis.
     *
     * @return float The y-axis scroll offset.
     */
    inline float GetYOffset() const noexcept { return m_YOffset; }

    /**
     * @brief Retrieves the static event type.
     *
     * @return EventType The static event type (MouseScrolled).
     */
    inline static EventType GetStaticType() noexcept { return EventType::MouseScrolled; }

private:
    float m_XOffset, m_YOffset; ///< The scroll offsets along the x and y axes.
    static constexpr const char* EVENT_NAME = "MouseScrolled"; ///< Static event name for debugging.
};