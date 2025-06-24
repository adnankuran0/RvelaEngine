#pragma once
#include <utility>
#include <type_traits>

class RefCounted {
public:
    void AddRef() { ++refCount; }
    void ReleaseRef() {
        if (--refCount == 0 && weakCount == 0)
            delete this;
    }

    void AddWeak() { ++weakCount; }
    void ReleaseWeak() {
        if (--weakCount == 0 && refCount == 0)
            delete this;
    }

    int GetRefCount() const { return refCount; }
    int GetWeakCount() const { return weakCount; }

protected:
    virtual ~RefCounted() = default;

private:
    int refCount = 0;
    int weakCount = 0;
};

template<typename T>
class Ref;

template<typename T>
class WeakRef;

template<typename T>
class Ref {
public:
    Ref() : ptr(nullptr) {}

    Ref(T* raw) : ptr(raw) {
        static_assert(std::is_base_of<RefCounted, T>::value);
        if (ptr) ptr->AddRef();
    }

    Ref(const Ref& other) : ptr(other.ptr) {
        if (ptr) ptr->AddRef();
    }

    Ref& operator=(const Ref& other) {
        if (this == &other)
            return *this;
        if (ptr) ptr->ReleaseRef();
        ptr = other.ptr;
        if (ptr) ptr->AddRef();
        return *this;
    }

    ~Ref() {
        if (ptr) ptr->ReleaseRef();
    }

    T* operator->() const { return ptr; }
    T& operator*() const { return *ptr; }
    T* Get() const { return ptr; }

    operator bool() const { return ptr != nullptr; }

private:
    friend class WeakRef<T>;
    T* ptr;
};

template<typename T>
class WeakRef {
public:
    WeakRef() : ptr(nullptr) {}

    WeakRef(const Ref<T>& ref) : ptr(ref.ptr) {
        if (ptr) ptr->AddWeak();
    }

    WeakRef(const WeakRef& other) : ptr(other.ptr) {
        if (ptr) ptr->AddWeak();
    }

    WeakRef& operator=(const WeakRef& other) {
        if (this == &other)
            return *this;
        if (ptr) ptr->ReleaseWeak();
        ptr = other.ptr;
        if (ptr) ptr->AddWeak();
        return *this;
    }

    ~WeakRef() {
        if (ptr) ptr->ReleaseWeak();
    }

    Ref<T> Lock() const {
        if (ptr && ptr->GetRefCount() > 0)
            return Ref<T>(ptr);
        return Ref<T>();
    }

    operator bool() const {
        return ptr && ptr->GetRefCount() > 0;
    }

private:
    T* ptr;
};

template<typename T, typename... Args>
Ref<T> CreateRef(Args&&... args) {
    static_assert(std::is_base_of<RefCounted, T>::value, "T must inherit RefCounted");
    return Ref<T>(new T(std::forward<Args>(args)...));
}

