#include "rvelapch.h"
#include "UUIDComponent.h"

std::string UUIDComponent::Serialize() const
{
    json j;
    j["UUID"] = uuid;

    return j.dump(4);
}
void UUIDComponent::Deserialize(const std::string& jsonStr)
{
    json j = json::parse(jsonStr);
    auto UUIDData = j["UUID"];
    uuid = UUIDData;
}