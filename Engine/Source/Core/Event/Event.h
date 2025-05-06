#pragma once

#include "Events.h"

// Forward declaration to minimize dependencies
enum class EventType;

class Event {
public:
    virtual ~Event() = default;

    virtual EventType GetEventType() const = 0;

    /**
     * @brief Retrieves the name of the event for debugging or logging purposes.
     */
    virtual const char* GetName() const = 0;
};