#pragma once
#include <cstdint>
#include <random>

using UUID = uint64_t;

class UUIDGenerator {
public:
    static uint64_t Generate() {
        return ++m_CurrentID;
    }

    static UUID GeneratePersistent() {
        static std::mt19937_64 rng(std::random_device{}());
        static std::uniform_int_distribution<UUID> dist;
        return dist(rng);
    }

    static void RegisterExternalUUID(uint64_t uuid) {
        if (uuid > m_CurrentID)
            m_CurrentID = uuid;
    }

private:
    inline static uint64_t m_CurrentID = 0;
};
