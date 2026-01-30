#pragma once
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

struct TagComponent
{
public:
    std::string tag;
    TagComponent() = default;
    TagComponent(const std::string& tag) : tag(tag) {}

    json Serialize() const;
    void Deserialize(const json& j);

};