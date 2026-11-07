#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <type_traits>
#include <cstdint>
#include "Event.h"

namespace rv { 

class EventManager {
public:

    template<typename T, typename... Args>
    static void Push(Args&&... args) {
        static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");
        void* ptr = Allocate(sizeof(T), alignof(T));
        if (ptr) {
            T* event = new (ptr) T(std::forward<Args>(args)...);
            m_EventQueue.push_back(event);
            if constexpr (!std::is_trivially_destructible_v<T>) {
                m_Destructors.push_back({ [](void* p) { static_cast<T*>(p)->~T(); }, ptr });
            }
        }
        else {
            PushEvent(new T(std::forward<Args>(args)...));
        }
    }

    static const std::vector<Event*>& GetEventQueue() noexcept;

    static void PushEvent(Event* event) noexcept;

    static void ClearEvents() noexcept;

    static void DispatchEvents(const std::function<void(Event&)>& handler) noexcept;

private:
    static void* Allocate(size_t size, size_t alignment) noexcept;

    struct DestructorEntry {
        void (*fn)(void*);
        void* ptr;
    };

    static constexpr size_t BUFFER_SIZE = 8192;
    static alignas(alignof(std::max_align_t)) uint8_t m_Buffer[BUFFER_SIZE];
    static size_t m_BufferOffset;
    static std::vector<Event*> m_EventQueue;
    static std::vector<DestructorEntry> m_Destructors;
    static std::vector<std::unique_ptr<Event>> m_HeapEvents;
};

}
