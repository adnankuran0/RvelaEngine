#include "Scene.h"
#include "../Core/Time.h"
#include <iostream>

Scene::Scene() : registry() {}

entt::entity Scene::createEntity(const std::string& name) {
    entt::entity entity = registry.create();
    registry.emplace<Name>(entity, name);
    registry.emplace<Transform>(entity, Transform{ 0.0f, 0.0f, 0.0f });
    return entity;
}

void Scene::destroyEntity(entt::entity entity) {
    registry.destroy(entity);
}

void Scene::update() {
    registry.view<Transform>().each([](auto entity, Transform& transform) {
        transform.x += 0.1f * Time::getDeltaTime();
        transform.y += 0.1f * Time::getDeltaTime();
        transform.z += 0.1f * Time::getDeltaTime();

        std::cout << "Entity updated: Position = ("
            << transform.x << ", " << transform.y << ", " << transform.z
            << ")\n";
        });
}

entt::registry& Scene::getRegistry()
{
    return registry;
}
