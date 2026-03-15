#pragma once
#include "uuid_v4/uuid_v4.h"
#include <string>
#include <random>

namespace rv {

class AssetUUID
{
public:
    AssetUUID() {
        m_UUID = UUIDv4::UUIDGenerator<std::mt19937_64>{}.getUUID();
    }

    explicit AssetUUID(const UUIDv4::UUID& uuid) : m_UUID(uuid) {}

    inline static AssetUUID Invalid() {
        static const UUIDv4::UUID emptyUUID{}; 
        return AssetUUID(emptyUUID);
    }

    std::string ToString() const {
        return m_UUID.str();
    }

    static AssetUUID FromString(const std::string& str) {
        UUIDv4::UUID parsed;
        parsed.fromStr(str.c_str());
        return AssetUUID(parsed);
    }

    bool operator==(const AssetUUID& other) const {
        return m_UUID == other.m_UUID;
    }

    bool operator!=(const AssetUUID& other) const {
        return !(*this == other);
    }

    inline bool IsValid() const {
        static const UUIDv4::UUID emptyUUID{};
        return !(m_UUID == emptyUUID);
    }

    inline const UUIDv4::UUID& Raw() const {
        return m_UUID;
    }

private:
    UUIDv4::UUID m_UUID;
};

}

namespace std {
    template <>
    struct hash<rv::AssetUUID> {
        size_t operator()(const rv::AssetUUID& uuid) const noexcept {
            return uuid.Raw().hash();
        }
    };
}

