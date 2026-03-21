#include "rvelapch.h"
#include "EventManager.h"
#include <functional>
#include "Events.h"
#include "MouseEvents.h"
#include "WindowEvents.h"
#include "KeyEvents.h"

using namespace rv;

std::vector<std::unique_ptr<Event>> EventManager::m_EventQueue;

const std::vector<std::unique_ptr<Event>>& EventManager::GetEventQueue() noexcept {
    return m_EventQueue;
}

void EventManager::PushEvent(Event* event) noexcept {
    if (event) {
        m_EventQueue.emplace_back(event);
    }
}

void EventManager::ClearEvents() noexcept {
    m_EventQueue.clear();
}

void EventManager::DispatchEvents(const std::function<void(Event&)>& handler) noexcept {
    if (!handler || m_EventQueue.empty()) {
        return;
    }

    Dispatch(m_EventQueue, handler);
}
