#pragma once
#include <iostream>

class Entity; // forward declaration

class ScriptableEntity
{
public:
    virtual ~ScriptableEntity() = default;

    void SetEntity(Entity* entity) { this->entity = entity; }

    virtual void OnCreate() {}
    virtual void OnUpdate(float dt);
    virtual void OnDestroy() {}

    Entity* entity;
};
