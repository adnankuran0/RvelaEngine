#pragma once
#include "Event.h"
#include "Events.h"

class EventDispatcher {
public:

    static void Dispatch(std::vector<Event*>& events,const std::function<void(Event&)>& handler) {
        for (auto& event : events) {
            handler(*event);
        }
    }
};

