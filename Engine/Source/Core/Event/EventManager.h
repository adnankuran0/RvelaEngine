#pragma once

#include <vector>
#include <functional>
#include <memory>
#include "Event.h"
#include "Events.h"
#include "MouseEvents.h"
#include "WindowEvents.h"
#include "KeyEvents.h"

/**
 * @brief Manages the event queue and dispatches events to registered handlers.
 *
 * The EventManager class provides a centralized mechanism for collecting, storing,
 * and dispatching events in the game engine. It maintains a queue of events and
 * allows systems to push new events, clear the queue, and dispatch events to
 * handlers. All operations are static, making this a singleton-like utility class.
 */
class EventManager {
public:
    /**
     * @brief Retrieves the current event queue.
     *
     * Provides read-only access to the event queue to prevent external modifications.
     *
     * @return const std::vector<std::unique_ptr<Event>>& A const reference to the event queue.
     */
    static const std::vector<std::unique_ptr<Event>>& GetEventQueue() noexcept;

    /**
     * @brief Adds a new event to the queue.
     *
     * Takes ownership of the provided event and adds it to the queue. Ignores null pointers.
     *
     * @param event A pointer to the event to be added. Ownership is transferred.
     */
    static void PushEvent(Event* event) noexcept;

    /**
     * @brief Clears all events from the queue.
     *
     * Releases all events in the queue, freeing their memory.
     */
    static void ClearEvents() noexcept;

    /**
     * @brief Dispatches all events in the queue to the specified handler.
     *
     * Iterates through the event queue and invokes the handler for each event.
     * Clears the queue after dispatching to prevent re-processing.
     *
     * @param handler A function that processes each event, taking a reference to an Event.
     */
    static void DispatchEvents(const std::function<void(Event&)>& handler) noexcept;

private:
    static std::vector<std::unique_ptr<Event>> m_EventQueue; ///< The queue storing events.
};