#pragma once
#include "../nlohmann/json.hpp"
#include "Scene/EntityUUID.h"

using json = nlohmann::json;

struct UUIDComponent 
{
public:
    EntityUUID uuid;

    UUIDComponent() = default;
    UUIDComponent(EntityUUID uuid) :uuid(uuid) {}

    std::string Serialize() const;
    void Deserialize(const std::string& jsonStr);
   
};