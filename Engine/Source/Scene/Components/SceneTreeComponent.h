#pragma once
#include "entt/entt.h"
#include "../nlohmann/json.hpp"
#include "Scene/EntityUUID.h"

namespace rv {

using json = nlohmann::json;

class SceneTreeComponent 
{
public:
    std::vector<EntityUUID> childrenUUIDs;
    std::vector<entt::entity> children;
    EntityUUID parentUUID = 0;
    entt::entity parent = entt::null;

    std::string Serialize() const;
    void Deserialize(const json& j);
};

}