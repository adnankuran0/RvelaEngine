#pragma once
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

struct TagComponent
{
public:
    std::string tag;
    TagComponent() = default;
    TagComponent(const std::string& tag) : tag(tag) {}

    std::string Serialize() const 
    {
        json j;
        j["tag"] = tag;

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) 
    {
        json j = json::parse(jsonStr);
        auto tagData = j["tag"];
        tag = tagData;
    }

};