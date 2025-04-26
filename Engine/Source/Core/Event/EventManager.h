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

    static const std::vector<std::unique_ptr<Event>>& GetEventQueue() noexcept;

    static void PushEvent(Event* event) noexcept;

    static void ClearEvents() noexcept;

    static void DispatchEvents(const std::function<void(Event&)>& handler) noexcept;

private:
    static std::vector<std::unique_ptr<Event>> m_EventQueue; ///< The queue storing events.
};