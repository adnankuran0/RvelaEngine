#pragma once

#include <vector>
#include <memory>
#include "Event.h"

/**
 * @brief A utility class for dispatching events to registered handlers.
 *
 * The EventDispatcher class provides a mechanism to distribute a collection of
 * events to a specified handler function. It is designed to facilitate decoupled
 * event processing in the game engine, allowing systems to respond to events
 * without direct dependencies.
 */
class EventDispatcher {
public:
    /**
     * @brief Dispatches a collection of events to the specified handler.
     *
     * Iterates through the provided vector of events and invokes the handler
     * function for each valid event. Skips null pointers to prevent crashes.
     *
     * @param events A const reference to a vector of unique_ptr events to be processed.
     * @param handler A function that processes each event, taking a reference to an Event.
     */
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