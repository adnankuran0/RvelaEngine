#pragma once
#include "uuid_v4.h"

class AssetUUID
{
public:
    AssetUUID() {
        m_UUID = UUIDv4::UUIDGenerator<std::mt19937_64>{}.getUUID();
    }

    explicit AssetUUID(const UUIDv4::UUID& uuid) : m_UUID(uuid) {}

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

    bool IsValid() const {
        return m_UUID != UUIDv4::UUID{};
    }

    const UUIDv4::UUID& Raw() const {
        return m_UUID;
    }

private:
    UUIDv4::UUID m_UUID;
};