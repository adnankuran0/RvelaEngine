#pragma once
#include <cstdint>

class Entity {
public:
    explicit Entity(uint32_t id) : m_ID(id) {}

    uint32_t getID() const { return m_ID; }

    bool operator==(const Entity& other) const { return m_ID == other.m_ID; }
    bool operator!=(const Entity& other) const { return !(*this == other); }

private:
    uint32_t m_ID;
};