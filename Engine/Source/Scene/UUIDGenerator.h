#pragma once
#include <cstdint>

using UUID = uint64_t;

class UUIDGenerator {
public:
    static uint64_t Generate() {
        return ++m_CurrentID;
    }

    static void RegisterExternalUUID(uint64_t uuid) {
        if (uuid > m_CurrentID)
            m_CurrentID = uuid;
    }

private:
    inline static uint64_t m_CurrentID = 0;
};