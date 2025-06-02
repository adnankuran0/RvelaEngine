#include "Viewport.h"
#include <iostream>
#include "ImGui/ImGuizmo.h"


void Viewport::Draw(Engine* engine, entt::entity& selectedEntity)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");
    {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        const float textureWidth = 1920.0f;
        const float textureHeight = 1080.0f;
        const float textureAspect = textureWidth / textureHeight;

        ImVec2 displaySize;
        if (viewportSize.x / viewportSize.y > textureAspect) {
            displaySize.y = viewportSize.y;
            displaySize.x = viewportSize.y * textureAspect;
        }
        else {
            displaySize.x = viewportSize.x;
            displaySize.y = viewportSize.x / textureAspect;
        }

        ImVec2 displayPos = ImGui::GetCursorScreenPos();
        displayPos.x += (viewportSize.x - displaySize.x) * 0.5f;
        displayPos.y += (viewportSize.y - displaySize.y) * 0.5f;

        

        ImTextureID textureID = (ImTextureID)(intptr_t)26; //TODO: make this dynamic

        ImGui::GetWindowDrawList()->AddImage(
            textureID,
            displayPos,
            ImVec2(displayPos.x + displaySize.x, displayPos.y + displaySize.y),
            ImVec2(0, 1), // UV min (OpenGL texture coordinates)
            ImVec2(1, 0), // UV max (flip vertically)
            IM_COL32_WHITE
        );

        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Viewport: %.0fx%.0f | Texture: 1920x1080 | Display: %.0fx%.0f | FPS: %.1f",
            viewportSize.x, viewportSize.y, displaySize.x, displaySize.y, ImGui::GetIO().Framerate);

        ///////////////////////////////// GIZMO
        ImGuizmo::BeginFrame();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();


        ImGuizmo::SetRect(displayPos.x, displayPos.y, displaySize.x, displaySize.y);

        

        // Eğer entity seçiliyse ve transform component varsa
        if (selectedEntity != entt::null && engine->GetScene()->GetRegistry().any_of<TransformComponent>(selectedEntity)) {
            auto& tc = engine->GetScene()->GetRegistry().get<TransformComponent>(selectedEntity);

            glm::mat4 transform = tc.GetWorldMatrix();

            glm::mat4 view = engine->GetCamera()->GetViewMatrix(); // Kamera sistemi nasıl çalışıyorsa oradan al
            glm::mat4 projection = engine->GetCamera()->projection;

            static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
            static ImGuizmo::MODE currentGizmoMode = ImGuizmo::LOCAL;

            // Kısayollarla değiştirmek istersen
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                if (ImGui::IsKeyPressed(ImGuiKey_W)) currentGizmoOperation = ImGuizmo::TRANSLATE;
                if (ImGui::IsKeyPressed(ImGuiKey_E)) currentGizmoOperation = ImGuizmo::ROTATE;
                if (ImGui::IsKeyPressed(ImGuiKey_R)) currentGizmoOperation = ImGuizmo::SCALE;
            }
            
            ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                currentGizmoOperation, currentGizmoMode,
                glm::value_ptr(transform));

            if (ImGuizmo::IsUsing()) {
                glm::vec3 translation, rotation, scale;
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform),
                    glm::value_ptr(translation),
                    glm::value_ptr(rotation),
                    glm::value_ptr(scale));
                 
                tc.SetPosition(translation);
                tc.SetScale(scale);
                tc.SetEulerRotation(glm::radians(rotation));
            }
        }

        // selecing
        ImVec2 mousePos = ImGui::GetIO().MousePos;

        bool isMouseInViewport =
            mousePos.x >= displayPos.x && mousePos.x <= (displayPos.x + displaySize.x) &&
            mousePos.y >= displayPos.y && mousePos.y <= (displayPos.y + displaySize.y);

        if (isMouseInViewport && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing())
        {

            glm::vec2 mouseViewportPos = {
                mousePos.x - displayPos.x,
                mousePos.y - displayPos.y
            };

            glm::vec2 viewportSizeF = { displaySize.x, displaySize.y };
            glm::mat4 view = engine->GetCamera()->GetViewMatrix();
            glm::mat4 projection = engine->GetCamera()->projection;

            glm::vec3 rayOrigin = engine->GetCamera()->Position;
            glm::vec3 rayDirection = ScreenPosToWorldRay(mouseViewportPos, viewportSizeF, projection, view);

            entt::entity closest = entt::null;
            float closestDist = std::numeric_limits<float>::max();

            auto& registry = engine->GetScene()->GetRegistry();
            registry.view<MeshRendererComponent>().each([&](entt::entity entity, MeshRendererComponent& mrc) {
                BoundingBox box = mrc.worldAABB;

                float t;
                if (RayIntersectsAABB(rayOrigin, rayDirection, box, t)) {
                    glm::vec3 hitPoint = rayOrigin + rayDirection * t;

                    glm::mat4 view = engine->GetCamera()->GetViewMatrix();
                    glm::mat4 projection = engine->GetCamera()->projection;
                    glm::vec4 viewport = glm::vec4(0, 0, displaySize.x, displaySize.y);

                    glm::vec3 screenPos = glm::project(hitPoint, view, projection, viewport);

                    glm::vec2 screenHitPos(screenPos.x, displaySize.y - screenPos.y); // ImGui y-up
                    glm::vec2 mouseViewportPos = {
                        mousePos.x - displayPos.x,
                        mousePos.y - displayPos.y
                    };

                    float distance2D = glm::distance(mouseViewportPos, screenHitPos);

                    if (distance2D < closestDist) {
                        closestDist = distance2D;
                        closest = entity;
                    }
                }
                });

            selectedEntity = closest;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

bool Viewport::RayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
    const BoundingBox& box, float& t)
{
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::max();

    for (int i = 0; i < 3; ++i)
    {
        if (abs(rayDir[i]) < 1e-8f)
        {
            if (rayOrigin[i] < box.min[i] || rayOrigin[i] > box.max[i])
                return false;
        }
        else
        {
            float invD = 1.0f / rayDir[i];
            float t0 = (box.min[i] - rayOrigin[i]) * invD;
            float t1 = (box.max[i] - rayOrigin[i]) * invD;

            if (t0 > t1) std::swap(t0, t1);

            tmin = std::max(tmin, t0);
            tmax = std::min(tmax, t1);

            if (tmax < tmin)
                return false;
        }
    }

    t = tmin;
    return true;
}

glm::vec3 Viewport::ScreenPosToWorldRay(const glm::vec2& mousePos, const glm::vec2& viewportSize,
    const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
{
    float x = (2.0f * mousePos.x) / viewportSize.x - 1.0f;
    float y = 1.0f - (2.0f * mousePos.y) / viewportSize.y;

    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec4 rayWorld = glm::inverse(viewMatrix) * rayEye;
    return glm::normalize(glm::vec3(rayWorld));
}
