#pragma once
#include <memory>

template<typename T>
class Ref {
public:
    Ref() = default;

    Ref(std::shared_ptr<T> ptr) : ptr(std::move(ptr)) {}

    template<typename U>
    Ref(const Ref<U>& other) {
        ptr = std::dynamic_pointer_cast<T>(other.GetShared());
    }

    Ref(const Ref& other) = default;
    Ref& operator=(const Ref& other) = default;

    T* operator->() const { return ptr.get(); }
    T& operator*() const { return *ptr; }
    T* Get() const { return ptr.get(); }

    std::shared_ptr<T> GetShared() const { return ptr; }

    operator bool() const { return ptr != nullptr; }

private:
    std::shared_ptr<T> ptr;

    template<typename U>
    friend class WeakRef;
};

template<typename T>
class WeakRef {
public:
    WeakRef() = default;

    WeakRef(const Ref<T>& ref) : weakPtr(ref.ptr) {}

    template<typename U>
    WeakRef(const Ref<U>& ref) {
        weakPtr = std::dynamic_pointer_cast<T>(ref.GetShared());
    }

    WeakRef(const WeakRef& other) = default;
    WeakRef& operator=(const WeakRef& other) = default;

    T* operator->() const { return Lock().Get(); }
    T& operator*() const { return *Lock(); }
    T* Get() const { return Lock().Get(); }

    Ref<T> Lock() const {
        return Ref<T>(weakPtr.lock());
    }

    operator bool() const {
        return !weakPtr.expired();
    }

private:
    std::weak_ptr<T> weakPtr;
};

// Helper
template<typename T, typename... Args>
Ref<T> CreateRef(Args&&... args) {
    return Ref<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

template<typename T, typename U>
Ref<T> DynamicCast(const Ref<U>& other) {
    return Ref<T>(std::dynamic_pointer_cast<T>(other.GetShared()));
}
