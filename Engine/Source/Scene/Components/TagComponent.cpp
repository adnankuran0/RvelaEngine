#include "rvelapch.h"
#include "TagComponent.h"

namespace rv {

json TagComponent::Serialize() const
{
    json j;
    j["tag"] = tag;
    return j;
}
void TagComponent::Deserialize(const json& j)
{
    tag = j.at("tag").get<std::string>();
}

}