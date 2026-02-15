#pragma once

#include "Events.h"

namespace rv { 

enum class EventType;

class Event {
public:
    virtual ~Event() = default;

    virtual EventType GetEventType() const = 0;

    virtual const char* GetName() const = 0;
};

}