#pragma once
#include <Jolt/Jolt.h>
#include "../nlohmann/json.hpp"

namespace rv {

using json = nlohmann::json;

struct BoxColliderComponent
{
	glm::vec3 size = { 0.5f, 0.5f, 0.5f };
	glm::vec3 offset = { 0.0f, 0.0f, 0.0f };

    json Serialize() const;
    void Deserialize(const json& j);
};

struct SphereColliderComponent {
    float radius = 0.5f;
    glm::vec3 offset = { 0.0f, 0.0f, 0.0f };
    
    json Serialize() const;
    void Deserialize(const json& j);;
};

struct CapsuleColliderComponent {
    float radius = 0.5f;
    float halfHeight = 1.0f;
    glm::vec3 offset = { 0.0f, 0.0f, 0.0f };

    json Serialize() const;
    void Deserialize(const json& j);
};

}