#include "rvelapch.h"
#include "EventManager.h"
#include <functional>
#include "Events.h"
#include "MouseEvents.h"
#include "WindowEvents.h"
#include "KeyEvents.h"

using namespace rv;

alignas(alignof(std::max_align_t)) uint8_t EventManager::m_Buffer[EventManager::BUFFER_SIZE];
size_t EventManager::m_BufferOffset = 0;
std::vector<Event*> EventManager::m_EventQueue;
std::vector<EventManager::DestructorEntry> EventManager::m_Destructors;
std::vector<std::unique_ptr<Event>> EventManager::m_HeapEvents;

void* EventManager::Allocate(size_t size, size_t alignment) noexcept {
    uintptr_t current = reinterpret_cast<uintptr_t>(m_Buffer + m_BufferOffset);
    uintptr_t aligned = (current + alignment - 1) & ~(static_cast<uintptr_t>(alignment) - 1);
    size_t offset = static_cast<size_t>(aligned - reinterpret_cast<uintptr_t>(m_Buffer));
    if (offset + size <= BUFFER_SIZE) {
        m_BufferOffset = offset + size;
        return m_Buffer + offset;
    }
    return nullptr;
}

const std::vector<Event*>& EventManager::GetEventQueue() noexcept {
    return m_EventQueue;
}

void EventManager::PushEvent(Event* event) noexcept {
    if (event) {
        m_HeapEvents.emplace_back(event);
        m_EventQueue.push_back(event);
    }
}

void EventManager::ClearEvents() noexcept {
    for (auto& entry : m_Destructors) {
        entry.fn(entry.ptr);
    }
    m_Destructors.clear();
    m_BufferOffset = 0;
    m_EventQueue.clear();
    m_HeapEvents.clear();
}

void EventManager::DispatchEvents(const std::function<void(Event&)>& handler) noexcept {
    if (!handler || m_EventQueue.empty()) {
        return;
    }

    for (Event* event : m_EventQueue) {
        if (event) {
            handler(*event);
        }
    }
}
