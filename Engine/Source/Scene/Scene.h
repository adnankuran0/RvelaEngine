#pragma once

#include <entt/entt.h>
#include <string>
#include "Components.h"
#include "Core/RvelaMath.h"

class Entity;


class Scene {
public:
    Scene();

    Entity CreateEntity(const std::string& name);
    void DestroyEntity(entt::entity entity);

    template<typename Component, typename... Args>
    void AddComponent(entt::entity entity, Args&&... args) {
        m_Registry.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template<typename Component>
    Component& GetComponent(entt::entity entity) {
        return m_Registry.get<Component>(entity);
    }

    template<typename Component>
    bool HasComponent(entt::entity entity) {
        return m_Registry.any_of<Component>(entity);
    }

    template<typename Component>
    void RemoveComponent(entt::entity entity) {
        m_Registry.remove<Component>(entity);
    }

    void SetParent(entt::entity child, entt::entity parent) {
        if (!HasComponent<SceneTreeComponent>(child))
            AddComponent<SceneTreeComponent>(child);
        if (parent != entt::null && !HasComponent<SceneTreeComponent>(parent))
            AddComponent<SceneTreeComponent>(parent);

        auto& childNode = GetComponent<SceneTreeComponent>(child);

        glm::mat4 childWorldMatrix;
        if (HasComponent<WorldTransformComponent>(child)) {
            auto& childWorldTransform = GetComponent<WorldTransformComponent>(child);
            childWorldMatrix = childWorldTransform.GetMatrix();
        }
        else {
            auto& childTransform = GetComponent<TransformComponent>(child);
            childWorldMatrix = childTransform.GetMatrix();
        }

        if (childNode.parent != entt::null) {
            auto& oldParentNode = GetComponent<SceneTreeComponent>(childNode.parent);
            oldParentNode.children.erase(
                std::remove(oldParentNode.children.begin(), oldParentNode.children.end(), child),
                oldParentNode.children.end()
            );
        }

        childNode.parent = parent;

        if (parent != entt::null) {
            auto& parentNode = GetComponent<SceneTreeComponent>(parent);
            parentNode.children.push_back(child);

            auto& parentWorldTransform = GetComponent<WorldTransformComponent>(parent);
            glm::mat4 parentWorldMatrix = parentWorldTransform.GetMatrix();
            glm::mat4 parentInverseMatrix = glm::inverse(parentWorldMatrix);

            glm::mat4 newLocalMatrix = parentInverseMatrix * childWorldMatrix;

            glm::vec3 newScale, newRotation, newPosition;
            DecomposeToEulerAngles(newLocalMatrix, newScale, newRotation, newPosition);

            auto& childTransform = GetComponent<TransformComponent>(child);
            childTransform.scale = newScale;
            childTransform.rotation = newRotation;
            childTransform.position = newPosition;
        }
        else {
            glm::vec3 scale, rotation, position;
            DecomposeToEulerAngles(childWorldMatrix, scale, rotation, position);

            auto& childTransform = GetComponent<TransformComponent>(child);
            childTransform.scale = scale;
            childTransform.rotation = rotation;
            childTransform.position = position;
        }
    }

    void RemoveParent(entt::entity child) {
        if (!HasComponent<SceneTreeComponent>(child))
            return;

        auto& childNode = GetComponent<SceneTreeComponent>(child);

        glm::mat4 childWorldMatrix;
        if (HasComponent<WorldTransformComponent>(child)) {
            auto& childWorldTransform = GetComponent<WorldTransformComponent>(child);
            childWorldMatrix = childWorldTransform.GetMatrix();
        }
        else {
            auto& childTransform = GetComponent<TransformComponent>(child);
            childWorldMatrix = childTransform.GetMatrix();
        }

        if (childNode.parent != entt::null) {
            auto& parentNode = GetComponent<SceneTreeComponent>(childNode.parent);
            parentNode.children.erase(
                std::remove(parentNode.children.begin(), parentNode.children.end(), child),
                parentNode.children.end()
            );
        }

        childNode.parent = entt::null;

        glm::vec3 scale, rotation, position;
        DecomposeToEulerAngles(childWorldMatrix, scale, rotation, position);

        auto& childTransform = GetComponent<TransformComponent>(child);
        childTransform.scale = scale;
        childTransform.rotation = rotation;
        childTransform.position = position;
    }

    const std::vector<entt::entity>& GetChildren(entt::entity entity) {
        return GetComponent<SceneTreeComponent>(entity).children;
    }

    entt::entity GetParent(entt::entity entity) {
        if (HasComponent<SceneTreeComponent>(entity))
            return GetComponent<SceneTreeComponent>(entity).parent;
        return entt::null;
    }

    void UpdateHierarchy();



    void Update();

    entt::registry& GetRegistry();

private:
    entt::registry m_Registry;
    friend class Entity;
};
