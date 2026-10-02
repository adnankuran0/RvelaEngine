#include "rvelapch.h"
#include "UILayout.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Scene/Components/UIComponents.h"
#include "Renderer/Camera.h"
#include <algorithm>
#include <cmath>
#include <glm/gtx/norm.hpp>

namespace rv {

glm::vec2 UILayout::RotatePoint(const glm::vec2& point, const glm::vec2& pivot, float rotationDegrees)
{
    if (rotationDegrees == 0.0f) return point;
    float rad = glm::radians(rotationDegrees);
    float cosR = std::cos(rad);
    float sinR = std::sin(rad);

    float dx = point.x - pivot.x;
    float dy = point.y - pivot.y;

    return glm::vec2(
        pivot.x + dx * cosR - dy * sinR,
        pivot.y + dx * sinR + dy * cosR
    );
}

bool UILayout::IsPointInRect(const glm::vec2& point, const glm::vec4& rect, float rotationDegrees)
{
    if (rotationDegrees == 0.0f) {
        return point.x >= rect.x && point.x <= rect.z &&
               point.y >= rect.y && point.y <= rect.w;
    }

    glm::vec2 center = { (rect.x + rect.z) * 0.5f, (rect.y + rect.w) * 0.5f };
    float rad = glm::radians(-rotationDegrees);
    float cosR = std::cos(rad);
    float sinR = std::sin(rad);

    float dx = point.x - center.x;
    float dy = point.y - center.y;
    glm::vec2 unrotatedPoint = {
        center.x + dx * cosR - dy * sinR,
        center.y + dx * sinR + dy * cosR
    };

    return unrotatedPoint.x >= rect.x && unrotatedPoint.x <= rect.z &&
           unrotatedPoint.y >= rect.y && unrotatedPoint.y <= rect.w;
}

glm::mat4 UILayout::CalculateWorldSpaceCanvasMatrix(Scene* scene, entt::entity canvasEntity, const UICanvasComponent& canvas, Camera* camera)
{
    glm::mat4 canvasWorldMatrix(1.0f);
    if (!scene || !scene->HasComponent<TransformComponent>(canvasEntity)) return canvasWorldMatrix;

    auto& tc = scene->GetComponent<TransformComponent>(canvasEntity);
    glm::vec3 canvasPos = tc.GetWorldPosition();
    glm::vec3 scale = tc.GetScale();

    if (canvas.constantScreenSize && camera != nullptr)
    {
        float dist = glm::distance(camera->Position, canvasPos);
        if (dist < 0.001f) dist = 0.001f;
        scale *= (dist * 0.1f * canvas.constantScaleFactor);
    }

    if (canvas.billboardMode == BillboardMode::Spherical && camera != nullptr)
    {
        glm::mat4 view = camera->GetViewMatrix();
        glm::vec3 camRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
        glm::vec3 camUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
        glm::vec3 camForward = -glm::vec3(view[0][2], view[1][2], view[2][2]);

        glm::mat4 billboardRot(1.0f);
        billboardRot[0] = glm::vec4(camRight, 0.0f);
        billboardRot[1] = glm::vec4(camUp, 0.0f);
        billboardRot[2] = glm::vec4(-camForward, 0.0f);
        billboardRot[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        canvasWorldMatrix = glm::translate(glm::mat4(1.0f), canvasPos) * billboardRot * glm::scale(glm::mat4(1.0f), scale);
    }
    else if (canvas.billboardMode == BillboardMode::Cylindrical && camera != nullptr)
    {
        glm::vec3 camPos = camera->Position;
        glm::vec3 dir = camPos - canvasPos;
        dir.y = 0.0f;
        float yaw = 0.0f;
        if (glm::length2(dir) > 0.0001f) {
            dir = glm::normalize(dir);
            yaw = std::atan2(dir.x, dir.z);
        }
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        canvasWorldMatrix = glm::translate(glm::mat4(1.0f), canvasPos) * rotY * glm::scale(glm::mat4(1.0f), scale);
    }
    else
    {
        if (canvas.constantScreenSize && camera != nullptr) {
            canvasWorldMatrix = glm::translate(glm::mat4(1.0f), canvasPos) * glm::mat4_cast(tc.GetWorldRotation()) * glm::scale(glm::mat4(1.0f), scale);
        } else {
            canvasWorldMatrix = tc.GetWorldMatrix();
        }
    }

    return canvasWorldMatrix;
}

void UILayout::UpdateLayoutRecursively(Scene* scene, entt::entity entity, const glm::vec2& parentPos, const glm::vec2& parentSize, const glm::vec2& parentCenter, float parentRotation)
{
    glm::vec2 currentPos = parentPos;
    glm::vec2 currentSize = parentSize;
    glm::vec2 currentCenter = parentCenter;
    float currentRotation = parentRotation;

    if (scene->HasComponent<RectTransformComponent>(entity))
    {
        auto& rect = scene->GetComponent<RectTransformComponent>(entity);
        glm::vec4 unrotatedRect = rect.CalculateScreenRect(parentPos, parentSize);
        float width = unrotatedRect.z - unrotatedRect.x;
        float height = unrotatedRect.w - unrotatedRect.y;
        glm::vec2 unrotatedCenter = { (unrotatedRect.x + unrotatedRect.z) * 0.5f, (unrotatedRect.y + unrotatedRect.w) * 0.5f };

        currentCenter = RotatePoint(unrotatedCenter, parentCenter, parentRotation);
        currentRotation = parentRotation + rect.rotation;

        rect.computedScreenRect = glm::vec4(
            currentCenter.x - width * 0.5f,
            currentCenter.y - height * 0.5f,
            currentCenter.x + width * 0.5f,
            currentCenter.y + height * 0.5f
        );
        rect.computedRotation = currentRotation;

        currentPos = { unrotatedRect.x, unrotatedRect.y };
        currentSize = { width, height };
    }

    if (scene->HasComponent<SceneTreeComponent>(entity))
    {
        const auto& children = scene->GetComponent<SceneTreeComponent>(entity).children;
        for (auto child : children)
        {
            UpdateLayoutRecursively(scene, child, currentPos, currentSize, currentCenter, currentRotation);
        }
    }
}

void UILayout::UpdateCanvasLayouts(Scene* scene, float viewportWidth, float viewportHeight)
{
    if (!scene) return;

    glm::vec2 viewportSize = { viewportWidth, viewportHeight };

    auto canvasView = scene->GetRegistry().view<UICanvasComponent>();
    if (canvasView.empty())
    {
        auto rectView = scene->GetRegistry().view<RectTransformComponent>();
        for (auto entity : rectView)
        {
            entt::entity parent = scene->GetParent(entity);
            if (parent == entt::null || !scene->HasComponent<RectTransformComponent>(parent))
            {
                UpdateLayoutRecursively(scene, entity, { 0.0f, 0.0f }, viewportSize, { viewportSize.x * 0.5f, viewportSize.y * 0.5f }, 0.0f);
            }
        }
        return;
    }

    for (auto canvasEntity : canvasView)
    {
        auto& canvas = canvasView.get<UICanvasComponent>(canvasEntity);

        glm::vec2 canvasPos = { 0.0f, 0.0f };
        glm::vec2 canvasSize = viewportSize;
        glm::vec2 canvasCenter = { viewportSize.x * 0.5f, viewportSize.y * 0.5f };
        float canvasRotation = 0.0f;

        if (canvas.mode == CanvasMode::WorldSpace)
        {
            float canvasW = 400.0f;
            float canvasH = 200.0f;
            if (scene->HasComponent<RectTransformComponent>(canvasEntity))
            {
                auto& canvasRect = scene->GetComponent<RectTransformComponent>(canvasEntity);
                canvasW = canvasRect.size.x > 0.0f ? canvasRect.size.x : 400.0f;
                canvasH = canvasRect.size.y > 0.0f ? canvasRect.size.y : 200.0f;
                canvasRect.computedScreenRect = glm::vec4(-canvasW * 0.5f, -canvasH * 0.5f, canvasW * 0.5f, canvasH * 0.5f);
                canvasRect.computedRotation = canvasRect.rotation;
                canvasRotation = canvasRect.rotation;
            }
            canvasPos = { -canvasW * 0.5f, -canvasH * 0.5f };
            canvasSize = { canvasW, canvasH };
            canvasCenter = { 0.0f, 0.0f };
        }
        else if (scene->HasComponent<RectTransformComponent>(canvasEntity))
        {
            auto& canvasRect = scene->GetComponent<RectTransformComponent>(canvasEntity);
            canvasRect.computedScreenRect = glm::vec4(0.0f, 0.0f, viewportSize.x, viewportSize.y);
            canvasRect.computedRotation = canvasRect.rotation;
            canvasPos = { canvasRect.computedScreenRect.x, canvasRect.computedScreenRect.y };
            canvasSize = { canvasRect.computedScreenRect.z - canvasRect.computedScreenRect.x, canvasRect.computedScreenRect.w - canvasRect.computedScreenRect.y };
            canvasCenter = { (canvasRect.computedScreenRect.x + canvasRect.computedScreenRect.z) * 0.5f, (canvasRect.computedScreenRect.y + canvasRect.computedScreenRect.w) * 0.5f };
            canvasRotation = canvasRect.rotation;
        }

        if (scene->HasComponent<SceneTreeComponent>(canvasEntity))
        {
            const auto& children = scene->GetComponent<SceneTreeComponent>(canvasEntity).children;
            for (auto child : children)
            {
                UpdateLayoutRecursively(scene, child, canvasPos, canvasSize, canvasCenter, canvasRotation);
            }
        }
    }
}

}
