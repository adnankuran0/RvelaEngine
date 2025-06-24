#include "Viewport.h"
#include <iostream>
#include "ImGui/ImGuizmo.h"
#include "Core/Engine.h"


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

            glm::mat4 transform = tc.GetLocalMatrix();

            glm::mat4 view = engine->GetCamera()->GetViewMatrix();
            glm::mat4 projection = engine->GetCamera()->projection;

            static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
            static ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

            if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                if (ImGui::IsKeyPressed(ImGuiKey_W)) currentGizmoOperation = ImGuizmo::TRANSLATE;
                if (ImGui::IsKeyPressed(ImGuiKey_E)) currentGizmoOperation = ImGuizmo::ROTATE;
                if (ImGui::IsKeyPressed(ImGuiKey_R)) currentGizmoOperation = ImGuizmo::SCALE;
            }

            bool useSnap = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

            float snapTranslate[3] = { 1.0f, 1.0f, 1.0f };
            float snapRotate = 15.0f;
            float snapScale[3] = { 1.f, 1.f, 1.f };

            if (currentGizmoOperation == ImGuizmo::TRANSLATE) {
                ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                    currentGizmoOperation, currentGizmoMode,
                    glm::value_ptr(transform), nullptr,
                    useSnap ? snapTranslate : nullptr);
            }
            else if (currentGizmoOperation == ImGuizmo::ROTATE) {
                ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                    currentGizmoOperation, currentGizmoMode,
                    glm::value_ptr(transform), nullptr,
                    useSnap ? &snapRotate : nullptr);
            }
            else if (currentGizmoOperation == ImGuizmo::SCALE) {
                ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                    currentGizmoOperation, currentGizmoMode,
                    glm::value_ptr(transform), nullptr,
                    useSnap ? snapScale : nullptr);
            }

            if (ImGuizmo::IsUsing()) {
                glm::vec3 translation, rotation, scale;
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform),
                    glm::value_ptr(translation),
                    glm::value_ptr(rotation),
                    glm::value_ptr(scale));

                tc.SetPosition(translation);
                tc.SetScale(scale);
                tc.SetEulerRotation(rotation);
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
            registry.view<TransformComponent,
                MeshRendererComponent,
                MeshComponent>().each([&](entt::entity entity, 
                    TransformComponent& tc,
                    MeshRendererComponent& mrc,
                    MeshComponent& mc) {
                
                BoundingBox box = mrc.worldAABB;
                //Frustum culling
                if (!engine->GetCamera()->Intersects(box)) return;

                MeshData& mesh = mc.mesh;
                glm::mat4 modelMatrix = tc.GetWorldMatrix();

                for (size_t i = 0; i < mesh.GetTriangleCount(); ++i)
                {
                    glm::vec3 v0, v1, v2;
                    mesh.GetTriangle(i, v0, v1, v2);

                    v0 = modelMatrix * glm::vec4(v0, 1.0f);
                    v1 = modelMatrix * glm::vec4(v1, 1.0f);
                    v2 = modelMatrix * glm::vec4(v2, 1.0f);

                    float t;
                    if (RayIntersectsTriangle(rayOrigin, rayDirection, v0, v1, v2, t))
                    {
                        if (t < closestDist)
                        {
                            closestDist = t;
                            selectedEntity = entity;
                            engine->GetScene()->SetSelectedEntity(selectedEntity);
                        }
                    }
                }
                });

            
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

bool Viewport::RayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
    float& t)
{
    const float EPSILON = 1e-8;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;

    glm::vec3 h = cross(rayDir, edge2);
    float a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false;

    float f = 1.0f / a;
    glm::vec3 s = rayOrigin - v0;
    float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;

    glm::vec3 q = cross(s, edge1);
    float v = f * dot(rayDir, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * dot(edge2, q);
    return t > EPSILON;
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
