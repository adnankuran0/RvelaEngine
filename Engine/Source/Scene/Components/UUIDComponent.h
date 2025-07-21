#pragma once
#include "Scene/Component.h"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

class UUIDComponent : public Component
{
public:
    EntityUUID uuid;

    UUIDComponent() = default;
    UUIDComponent(EntityUUID uuid) :uuid(uuid) {}

    std::string Serialize() const override
    {
        json j;
        j["UUID"] = uuid;

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto UUIDData = j["UUID"];
        uuid = UUIDData;
    }

};