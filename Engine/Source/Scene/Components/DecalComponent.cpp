#include "rvelapch.h"
#include "DecalComponent.h"

namespace rv {

json DecalComponent::Serialize() const
{
    json j;
    j["textureUUID"] = textureUUID.ToString();
    j["color"] = { color.r, color.g, color.b, color.a };
    j["opacity"] = opacity;
    j["angleCutoff"] = angleCutoff;
    j["renderOrder"] = renderOrder;
    j["lit"] = lit;
    j["receiveShadows"] = receiveShadows;
    j["upperFade"] = upperFade;
    j["lowerFade"] = lowerFade;
    j["distanceFade"] = distanceFade;
    j["fadeStart"] = fadeStart;
    j["fadeEnd"] = fadeEnd;
    return j;
}

void DecalComponent::Deserialize(const json& j)
{
    if (j.contains("textureUUID"))
        textureUUID = AssetUUID::FromString(j["textureUUID"].get<std::string>());
    if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 4)
        color = glm::vec4(j["color"][0], j["color"][1], j["color"][2], j["color"][3]);
    opacity = j.value("opacity", 1.0f);
    angleCutoff = j.value("angleCutoff", 60.0f);
    renderOrder = j.value("renderOrder", 0);
    lit = j.value("lit", true);
    receiveShadows = j.value("receiveShadows", true);
    upperFade = j.value("upperFade", 0.0f);
    lowerFade = j.value("lowerFade", 0.0f);
    distanceFade = j.value("distanceFade", false);
    fadeStart = j.value("fadeStart", 40.0f);
    fadeEnd = j.value("fadeEnd", 50.0f);
}

}
