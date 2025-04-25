#include "rvelapch.h"
#include "EventManager.h"
#include "EventDispatcher.h"

// Initialize the static event queue
std::vector<std::unique_ptr<Event>> EventManager::m_EventQueue;

/**
 * @brief Retrieves the current event queue.
 *
 * Provides read-only access to the event queue to prevent external modifications.
 *
 * @return const std::vector<std::unique_ptr<Event>>& A const reference to the event queue.
 */
const std::vector<std::unique_ptr<Event>>& EventManager::GetEventQueue() noexcept {
    return m_EventQueue;
}

/**
 * @brief Adds a new event to the queue.
 *
 * Takes ownership of the provided event and adds it to the queue. Ignores null pointers.
 *
 * @param event A pointer to the event to be added. Ownership is transferred.
 */
void EventManager::PushEvent(Event* event) noexcept {
    if (event) {
        m_EventQueue.emplace_back(event);
    }
}

/**
 * @brief Clears all events from the queue.
 *
 * Releases all events in the queue, freeing their memory.
 */
void EventManager::ClearEvents() noexcept {
    m_EventQueue.clear();
}

/**
 * @brief Dispatches all events in the queue to the specified handler.
 *
 * Iterates through the event queue and invokes the handler for each event using
 * EventDispatcher. Clears the queue after dispatching to prevent re-processing.
 *
 * @param handler A function that processes each event, taking a reference to an Event.
 */
void EventManager::DispatchEvents(const std::function<void(Event&)>& handler) noexcept {
    if (!handler || m_EventQueue.empty()) {
        return;
    }

    EventDispatcher::Dispatch(m_EventQueue, handler);
    ClearEvents();
}