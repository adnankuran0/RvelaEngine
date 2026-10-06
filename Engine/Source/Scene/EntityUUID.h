#pragma once
#include <cstdint>
#include <random>

namespace rv {

using EntityUUID = uint64_t;

class Entity;
class Scene;

// Stable scene reference used by serialized properties. Runtime Entity values
// are resolved from this UUID so references survive entity handle changes.
struct EntityHandle {
    EntityUUID uuid = 0;

    bool IsValid() const { return uuid != 0; }
    explicit operator bool() const { return IsValid(); }
    bool operator==(const EntityHandle&) const = default;
    Entity Resolve(Scene& scene) const;
};

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
