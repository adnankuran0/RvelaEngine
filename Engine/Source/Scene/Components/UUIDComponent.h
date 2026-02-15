#pragma once
#include "../nlohmann/json.hpp"
#include "Scene/EntityUUID.h"

namespace rv {

using json = nlohmann::json;

struct UUIDComponent 
{
public:
    EntityUUID uuid;

    UUIDComponent() = default;
    UUIDComponent(EntityUUID uuid) :uuid(uuid) {}

    json Serialize() const;
    void Deserialize(const json& j);
};

}