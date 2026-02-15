#pragma once
#include <string>
#include <chrono>
#include <iostream>
#include "Core/Log.h"

#define PROFILE_SCOPE(name) ScopedTimer timer##__LINE__(name);

namespace rv {

class ScopedTimer {
public:
    ScopedTimer(const std::string& name) : m_Name(name), m_Start(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - m_Start).count();
        LOG_INFO("{} : {} ms", m_Name , duration);
    }

private:
    std::string m_Name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};

}