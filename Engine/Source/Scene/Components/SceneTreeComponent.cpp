#include "rvelapch.h"
#include "SceneTreeComponent.h"

std::string SceneTreeComponent::Serialize() const
{
    json j;
    j["parentUUID"] = parentUUID;
    j["childrenUUIDs"] = childrenUUIDs;

    return j.dump(4);
}

void SceneTreeComponent::Deserialize(const std::string& jsonStr)
{
    json j = json::parse(jsonStr);
    parentUUID = j["parentUUID"];
    childrenUUIDs.clear();
    for (auto& id : j["childrenUUIDs"])
    {
        childrenUUIDs.push_back(id.get<EntityUUID>());
    }
}