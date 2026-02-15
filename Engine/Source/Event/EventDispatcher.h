#pragma once

#include <vector>
#include <memory>
#include "Event.h"

namespace rv { 

class EventDispatcher {
public:
    static void Dispatch(const std::vector<std::unique_ptr<Event>>& events,
        const std::function<void(Event&)>& handler) noexcept {
        if (events.empty() || !handler) {
            return;
        }

        for (const auto& event : events) {
            if (event) {
                handler(*event);
            }
        }
    }
};

}