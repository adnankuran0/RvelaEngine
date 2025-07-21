#pragma once
#include "Scene/Component.h"
#include "entt/entt.h"
#include "../nlohmann/json.hpp"
#include "Scene/EntityUUID.h"

using json = nlohmann::json;

class SceneTreeComponent : public Component {
public:
    std::vector<EntityUUID> childrenUUIDs;
    std::vector<entt::entity> children;
    EntityUUID parentUUID = 0;
    entt::entity parent = entt::null;


    std::string Serialize() const override
    {
        json j;
        j["parentUUID"] = parentUUID;
        j["childrenUUIDs"] = childrenUUIDs;

        return j.dump(4);
    }

    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        parentUUID = j["parentUUID"];
        childrenUUIDs.clear();
        for (auto& id : j["childrenUUIDs"])
        {
            childrenUUIDs.push_back(id.get<EntityUUID>());
        }
    }

};