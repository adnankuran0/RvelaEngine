#pragma once
#include "Scene/ScriptableEntity.h"

struct ScriptComponent {
    ScriptableEntity* instance = nullptr;

    ScriptableEntity* (*InstantiateScript)() = nullptr;
    void (*DestroyScript)(ScriptComponent*) = nullptr;
};