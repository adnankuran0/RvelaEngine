#pragma once
#include <glm/glm.hpp>
#include "json.hpp"

using json = nlohmann::json;

struct CameraComponent
{
    float fov = 75.0f;
    float nearClip = 0.1f;
    float farClip = 100.0f;
    bool isActive = false; 

    json Serialize() const;
    void Deserialize(const json& j);
};
