#include "rvelapch.h"
#include "TagComponent.h"

using namespace rv;

json TagComponent::Serialize() const
{
    json j;
    j["tag"] = tag;
    return j;
}

void TagComponent::Deserialize(const json& j)
{
    if (j.is_string())
    {
        tag = j.get<std::string>();
        return;
    }
    tag = j.value("tag", "Entity");
}