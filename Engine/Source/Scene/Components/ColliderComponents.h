#pragma once

namespace rv {

struct BoxColliderComponent
{
	glm::vec3 Size = { 1.0f, 1.0f, 1.0f };
	glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };
};

struct SphereColliderComponent {
    float Radius = 0.5f;
    glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };
};

struct CapsuleColliderComponent {
    float Radius = 0.5f;
    float HalfHeight = 1.0f;
    glm::vec3 Offset = { 0.0f, 0.0f, 0.0f };
};

}