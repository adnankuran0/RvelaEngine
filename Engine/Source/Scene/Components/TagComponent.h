#pragma once
#include "Scene/Component.h"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

class TagComponent : public Component {
public:
    std::string tag;
    TagComponent() = default;
    TagComponent(const std::string& tag) : tag(tag) {}

    std::string Serialize() const override
    {
        json j;
        j["tag"] = tag;

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto tagData = j["tag"];
        tag = tagData;
    }

};