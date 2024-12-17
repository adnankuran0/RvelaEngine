#include "EventManager.h"

void EventManager::Subscribe(const std::string& eventName, Callback callback) {
    listeners[eventName].push_back(callback);
}


void EventManager::Dispatch(const std::string& eventName) {
    if (listeners.find(eventName) != listeners.end()) {
        for (const auto& callback : listeners[eventName]) {
            callback();
        }
    }
}