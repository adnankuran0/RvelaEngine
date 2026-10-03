#pragma once
#include "../nlohmann/json.hpp"

namespace rv {

using json = nlohmann::json;

struct TagComponent
{
public:
    std::string tag;
    bool isActive = true;

    TagComponent() = default;
    TagComponent(const std::string& tag, bool isActive = true) : tag(tag), isActive(isActive) {}

    json Serialize() const;
    void Deserialize(const json& j);

};

}