#pragma once
#include <string>
#include <chrono>
#include <iostream>

#define PROFILE_SCOPE(name) ScopedTimer timer##__LINE__(name);

class ScopedTimer {
public:
    ScopedTimer(const std::string& name) : m_Name(name), m_Start(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - m_Start).count();
        std::cout << m_Name << ": " << duration << "ms" << std::endl;
    }

private:
    std::string m_Name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
};