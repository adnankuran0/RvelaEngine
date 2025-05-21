#pragma once
#include "Scene/Component.h"
#include "entt/entt.h"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;
using UUID = uint64_t;

class SceneTreeComponent : public Component {
public:
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;

    UUID parentUUID = 0;
    std::vector<UUID> childrenUUIDs;

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
            childrenUUIDs.push_back(id.get<UUID>());
        }
    }

};