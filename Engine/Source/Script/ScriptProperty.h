#pragma once
#include "Asset/AssetUUID.h"
#include "Scene/EntityUUID.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace rv {

enum class ScriptPropertyType
{
    Float,
    Int,
    Bool,
    String,
    Vec2,
    Vec3,
    Vec4,
    Color,
    AssetHandle,
    Entity
};

struct ScriptPropertyDef
{
    std::string name;
    ScriptPropertyType type = ScriptPropertyType::Float;

    // Default values
    float floatVal = 0.0f;
    int intVal = 0;
    bool boolVal = false;
    std::string stringVal;
    glm::vec2 vec2Val{ 0.0f };
    glm::vec3 vec3Val{ 0.0f };
    glm::vec4 vec4Val{ 0.0f };
    AssetUUID assetVal{};
    EntityHandle entityVal{};

    // Metadata
    float minVal = 0.0f;
    float maxVal = 0.0f;
    float step = 0.1f;
    bool hasRange = false;
};

}
