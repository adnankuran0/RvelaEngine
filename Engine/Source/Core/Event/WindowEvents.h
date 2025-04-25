#pragma once

#include "Event.h"

/**
 * @brief Event triggered when the game window is resized.
 *
 * The WindowResizedEvent class represents a window resize event, providing access
 * to the new width and height of the window. This event is typically used to update
 * rendering configurations or UI layouts.
 */
class WindowResizedEvent : public Event {
public:
    /**
     * @brief Constructs a WindowResizedEvent with the specified window dimensions.
     *
     * Ensures that the width and height are positive values to prevent invalid states.
     *
     * @param width The new width of the window (must be positive).
     * @param height The new height of the window (must be positive).
     */
    explicit WindowResizedEvent(unsigned int width, unsigned int height) noexcept
        : m_WindowWidth(width > 0 ? width : 1), m_WindowHeight(height > 0 ? height : 1) {
    }

    /**
     * @brief Retrieves the type of the event.
     *
     * @return EventType The event type (WindowResized).
     */
    EventType GetEventType() const noexcept override { return EventType::WindowResized; }

    /**
     * @brief Retrieves the name of the event for debugging or logging.
     *
     * @return const char* The name of the event ("WindowResized").
     */
    const char* GetName() const noexcept override { return EVENT_NAME; }

    /**
     * @brief Retrieves the new width of the window.
     *
     * @return unsigned int The window width.
     */
    unsigned int GetWidth() const noexcept { return m_WindowWidth; }

    /**
     * @brief Retrieves the new height of the window.
     *
     * @return unsigned int The window height.
     */
    unsigned int GetHeight() const noexcept { return m_WindowHeight; }

    /**
     * @brief Retrieves the static event type.
     *
     * @return EventType The static event type (WindowResized).
     */
    static EventType GetStaticType() noexcept { return EventType::WindowResized; }

private:
    unsigned int m_WindowWidth;  ///< The new width of the window.
    unsigned int m_WindowHeight; ///< The new height of the window.
    static constexpr const char* EVENT_NAME = "WindowResized"; ///< Static event name for debugging.
};