#pragma once
#include <nlohmann/json.hpp>

namespace rv {

using json = nlohmann::json;

struct VisibilityRangeComponent
{
public:
    float begin = 0.0f;
    float beginMargin = 0.0f;
    float end = 0.0f;
    float endMargin = 0.0f;
    bool fade = false;

    VisibilityRangeComponent() = default;
    VisibilityRangeComponent(const VisibilityRangeComponent&) = default;
    VisibilityRangeComponent& operator=(const VisibilityRangeComponent&) = default;

    json Serialize() const;
    void Deserialize(const json& j);
};

}
