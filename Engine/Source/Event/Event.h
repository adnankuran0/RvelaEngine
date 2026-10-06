#pragma once

#include "Events.h"

namespace rv { 

enum class EventType;

class Event {
public:
    virtual ~Event() = default;

    // Set by the first layer that owns the event to stop propagation.
    bool Handled = false;

    virtual EventType GetEventType() const = 0;

    virtual const char* GetName() const = 0;
};

}
