#pragma once

#include "Scene/Entity.h"
#include <functional>

namespace rv {

    template<typename T>
    struct ComponentHandle
    {
        mutable Entity entity{};

        ComponentHandle() = default;
        ComponentHandle(Entity e) : entity(e) {}

        bool IsValid() const { return entity && entity.HasComponent<T>(); }

        operator bool() const { return IsValid(); }

        T* Get() const
        {
            if (IsValid())
            {
                return &entity.GetComponent<T>();
            }
            return nullptr;
        }

        bool operator==(const ComponentHandle<T>& other) const { return entity == other.entity; }
        bool operator!=(const ComponentHandle<T>& other) const { return !(*this == other); }
        bool operator<(const ComponentHandle<T>& other) const { return entity < other.entity; }
        bool operator<=(const ComponentHandle<T>& other) const { return (*this < other) || (*this == other); }
    };

}

