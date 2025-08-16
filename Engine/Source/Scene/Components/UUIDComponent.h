#pragma once
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

struct UUIDComponent 
{
public:
    EntityUUID uuid;

    UUIDComponent() = default;
    UUIDComponent(EntityUUID uuid) :uuid(uuid) {}

    std::string Serialize() const
    {
        json j;
        j["UUID"] = uuid;

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr)
    {
        json j = json::parse(jsonStr);
        auto UUIDData = j["UUID"];
        uuid = UUIDData;
    }

};