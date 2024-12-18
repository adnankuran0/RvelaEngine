#pragma once
#include <unordered_map>
#include <vector>
#include <functional>
#include <string>
#include "Events.h"

class Event {
public:
    virtual ~Event() = default;
    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
};