#include "Viewport.h"
#include <iostream>
#include "ImGui/imgui.h"
#include "ImGui/ImGuizmo.h"
#include "Core/Engine.h"
#include <glm/gtx/matrix_decompose.hpp>

namespace rv {

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



        ImTextureID textureID = (ImTextureID)(intptr_t)engine->GetFinalTexture(); //TODO: make this dynamic

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

        bool canDrawGizmo = selectedEntity != entt::null && engine->GetActiveScene().GetRegistry().any_of<TransformComponent>(selectedEntity) &&
            engine->GetActiveScene().GetState() == SceneState::EDIT;

        if (canDrawGizmo) {
            auto& tc = engine->GetActiveScene().GetRegistry().get<TransformComponent>(selectedEntity);

            glm::mat4 transform = tc.GetWorldMatrix();

            glm::mat4 view = engine->GetCamera()->GetViewMatrix();
            glm::mat4 projection = engine->GetCamera()->GetProjectionMatrix();

            static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
            static ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;
            if (ImGui::IsKeyPressed(ImGuiKey_T)) {
                currentGizmoMode = (currentGizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            }
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

                if (currentGizmoMode == ImGuizmo::LOCAL) {
                    // Local mode için de parent transform'unu hesaba kat
                    glm::mat4 parentWorld(1.0f);
                    auto& scene = engine->GetActiveScene();
                    auto& reg = scene.GetRegistry();
                    auto& node = reg.get<SceneTreeComponent>(selectedEntity);
                    if (node.parent != entt::null && reg.valid(node.parent)) {
                        parentWorld = reg.get<TransformComponent>(node.parent).GetWorldMatrix();
                    }
                    glm::mat4 localM = glm::inverse(parentWorld) * transform;
                    glm::vec3 lPos, lScale, skew;
                    glm::quat lRot;
                    glm::vec4 persp;
                    glm::decompose(localM, lScale, lRot, lPos, skew, persp);

                    tc.SetPosition(lPos);
                    tc.SetScale(lScale);
                    tc.SetRotation(lRot);
                }
                else if (currentGizmoMode == ImGuizmo::WORLD) {
                    glm::mat4 parentWorld(1.0f);
                    auto& scene = engine->GetActiveScene();
                    auto& reg = scene.GetRegistry();
                    auto& node = reg.get<SceneTreeComponent>(selectedEntity);
                    if (node.parent != entt::null && reg.valid(node.parent)) {
                        parentWorld = reg.get<TransformComponent>(node.parent).GetWorldMatrix();
                    }
                    glm::mat4 localM = glm::inverse(parentWorld) * transform;
                    glm::vec3 lPos, lScale, skew;
                    glm::quat lRot;
                    glm::vec4 persp;
                    glm::decompose(localM, lScale, lRot, lPos, skew, persp);
                    tc.SetPosition(lPos);
                    tc.SetScale(lScale);
                    tc.SetRotation(lRot);
                }
            }
        }



        // selecting
        
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
            glm::mat4 projection = engine->GetCamera()->GetProjectionMatrix();

            glm::vec3 rayOrigin = engine->GetCamera()->GetPosition();
            glm::vec3 rayDirection = math::ScreenPosToWorldRay(mouseViewportPos, viewportSizeF, projection, view);

            entt::entity closest = entt::null;
            float closestDist = std::numeric_limits<float>::max();

            auto& registry = engine->GetActiveScene().GetRegistry();
            registry.view<TransformComponent,
                MeshRendererComponent,
                MeshComponent>().each([&](entt::entity entity,
                    TransformComponent& tc,
                    MeshRendererComponent& mrc,
                    MeshComponent& mc) {

                        AABB& box = mrc.worldAABB;
                        //Frustum culling
                        if (!engine->GetCamera()->Intersects(box)) return;

                        Ref<MeshAsset> mesh = mc.GetMesh();
                        glm::mat4 modelMatrix = tc.GetWorldMatrix();

                        for (size_t i = 0; i < mesh->GetTriangleCount(); ++i)
                        {
                            glm::vec3 v0, v1, v2;
                            mesh->GetTriangle(i, v0, v1, v2);

                            v0 = modelMatrix * glm::vec4(v0, 1.0f);
                            v1 = modelMatrix * glm::vec4(v1, 1.0f);
                            v2 = modelMatrix * glm::vec4(v2, 1.0f);

                            float t;
                            if (math::RayIntersectsTriangle(rayOrigin, rayDirection, v0, v1, v2, t))
                            {
                                if (t < closestDist)
                                {
                                    closestDist = t;
                                    selectedEntity = entity;
                                    engine->GetActiveScene().SetSelectedEntity(selectedEntity);
                                }
                            }
                        }
                    });


        }
        
    }
    ImGui::End();
    ImGui::PopStyleVar();
}



}