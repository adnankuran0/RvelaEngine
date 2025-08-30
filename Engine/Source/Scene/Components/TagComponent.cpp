#include "rvelapch.h"
#include "TagComponent.h"

std::string TagComponent::Serialize() const
{
    json j;
    j["tag"] = tag;

    return j.dump(4);
}
void TagComponent::Deserialize(const std::string& jsonStr)
{
    json j = json::parse(jsonStr);
    auto tagData = j["tag"];
    tag = tagData;
}