#pragma once

/**
 * @brief Enumerates the types of events supported by the game engine.
 *
 * The EventType enum class defines unique identifiers for different events that
 * can occur in the game engine, such as keyboard input, mouse actions, or window
 * changes. This enum is used by the event system to dispatch and handle events
 * in a type-safe manner.
 */
enum class EventType {
    /**
     * @brief Default value indicating no event.
     */
    None = 0,

    /**
     * @brief Event triggered when a keyboard key is pressed.
     */
    KeyPressed,

    /**
     * @brief Event triggered when a keyboard key is released.
     */
    KeyReleased,

    /**
     * @brief Event triggered when the mouse cursor moves.
     */
    MouseMoved,

    /**
     * @brief Event triggered when a mouse button is pressed.
     */
    MouseButtonPressed,

    /**
     * @brief Event triggered when a mouse button is released.
     */
    MouseButtonReleased,

    /**
     * @brief Event triggered when the mouse wheel is scrolled.
     */
    MouseScrolled,

    /**
     * @brief Event triggered when the game window is resized.
     */
    WindowResized
};