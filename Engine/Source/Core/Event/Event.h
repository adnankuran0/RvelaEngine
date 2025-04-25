#pragma once

#include "Events.h"

// Forward declaration to minimize dependencies
enum class EventType;

/**
 * @brief Abstract base class for all events in the game engine.
 *
 * The Event class defines the interface for all event types, providing a mechanism
 * to identify and process events in a type-safe and extensible manner. Derived
 * classes must implement the pure virtual functions to specify their event type
 * and name.
 */
class Event {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~Event() = default;

    /**
     * @brief Retrieves the type of the event.
     *
     * @return EventType The unique type identifier for this event.
     */
    virtual EventType GetEventType() const = 0;

    /**
     * @brief Retrieves the name of the event for debugging or logging purposes.
     *
     * @return const char* A null-terminated string representing the event's name.
     */
    virtual const char* GetName() const = 0;
};