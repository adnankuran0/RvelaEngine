#pragma once
#include <cstdint>
#include <random>

namespace rv {

using EntityUUID = uint64_t;

class EntityUUIDGenerator {
public:
    static uint64_t Generate() {
        return ++m_CurrentID;
    }

    static EntityUUID GeneratePersistent() {
        static std::mt19937_64 rng(std::random_device{}());
        static std::uniform_int_distribution<EntityUUID> dist;
        return dist(rng);
    }

    static void RegisterExternalUUID(uint64_t uuid) {
        if (uuid > m_CurrentID)
            m_CurrentID = uuid;
    }

private:
    inline static uint64_t m_CurrentID = 0;
};

}