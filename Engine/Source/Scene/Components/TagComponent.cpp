#include "rvelapch.h"
#include "TagComponent.h"

using namespace rv;

json TagComponent::Serialize() const
{
    json j;
    j["tag"] = tag;
    j["isActive"] = isActive;
    return j;
}

void TagComponent::Deserialize(const json& j)
{
    if (j.is_string())
    {
        tag = j.get<std::string>();
        isActive = true;
        return;
    }
    tag = j.value("tag", "Entity");
    isActive = j.value("isActive", true);
}