#pragma once
#include <glm/glm.hpp>
#include "json.hpp"
#include "Rendering/Camera.h"

namespace rv {

using json = nlohmann::json;

struct CameraComponent
{
    Camera camera;
    bool isActive = true; 

    json Serialize() const;
    void Deserialize(const json& j);
};

}