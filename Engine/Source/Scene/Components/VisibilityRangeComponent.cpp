#include "rvelapch.h"
#include "VisibilityRangeComponent.h"

namespace rv {

json VisibilityRangeComponent::Serialize() const
{
    json j;
    j["begin"] = begin;
    j["beginMargin"] = beginMargin;
    j["end"] = end;
    j["endMargin"] = endMargin;
    j["fade"] = fade;
    return j;
}

void VisibilityRangeComponent::Deserialize(const json& j)
{
    if (j.contains("begin"))
        begin = j["begin"].get<float>();
    if (j.contains("beginMargin"))
        beginMargin = j["beginMargin"].get<float>();
    if (j.contains("end"))
        end = j["end"].get<float>();
    if (j.contains("endMargin"))
        endMargin = j["endMargin"].get<float>();
    if (j.contains("fade"))
        fade = j["fade"].get<bool>();
}

} // namespace rv
