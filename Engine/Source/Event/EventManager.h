#pragma once

#include <vector>
#include <functional>
#include <memory>
#include "Event.h"
#include "Events.h"
#include "MouseEvents.h"
#include "WindowEvents.h"
#include "KeyEvents.h"

class EventManager {
public:

    static const std::vector<std::unique_ptr<Event>>& GetEventQueue() noexcept;

    static void PushEvent(Event* event) noexcept;

    static void ClearEvents() noexcept;

    static void DispatchEvents(const std::function<void(Event&)>& handler) noexcept;

private:
    static std::vector<std::unique_ptr<Event>> m_EventQueue; ///< The queue storing events.
};