#include "rvelapch.h"
#include "SceneTreeComponent.h"

namespace rv {

std::string SceneTreeComponent::Serialize() const
{
    json j;
    j["parentUUID"] = parentUUID;
    j["childrenUUIDs"] = childrenUUIDs;

    return j.dump(4);
}

void SceneTreeComponent::Deserialize(const json& j)
{
    parentUUID = j.at("parentUUID").get<EntityUUID>();
    childrenUUIDs.clear();
    for (auto& id : j["childrenUUIDs"])
    {
        childrenUUIDs.push_back(id.get<EntityUUID>());
    }
}

}