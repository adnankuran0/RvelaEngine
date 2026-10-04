#include "Viewport.h"
#include <iostream>
#include "ImGui/imgui.h"
#include "ImGui/ImGuizmo.h"
#include "Core/Engine.h"
#include <glm/gtx/matrix_decompose.hpp>
#include "Renderer/RenderLayer.h"
#include "Renderer/DebugRenderer.h"
#include "Input/Input.h"
#include "Renderer/EditorCamera.h"
#include "EditorSettings.h"

using namespace rv;

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

void Viewport::DrawGizmos(Engine* engine, ImVec2& displayPos, ImVec2& displaySize, entt::entity selectedEntity, const std::vector<entt::entity>& selectedEntities)
{
    bool canDrawGizmo = selectedEntity != entt::null &&
        engine->GetActiveScene().GetRegistry().valid(selectedEntity) &&
        engine->GetActiveScene().GetRegistry().any_of<TransformComponent>(selectedEntity) &&
        engine->GetActiveScene().GetState() == SceneState::EDIT;
    if (!canDrawGizmo)
    {
        m_WasGizmoUsing = false;
        m_InitialTransforms.clear();
        return;
    }

    ImGuizmo::BeginFrame();
    bool isOrtho = engine->GetCamera() && (engine->GetCamera()->ProjectionType == Camera::Projection::Orthographic);
    ImGuizmo::SetOrthographic(isOrtho);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(displayPos.x, displayPos.y, displaySize.x, displaySize.y);

    auto& scene = engine->GetActiveScene();
    auto& reg = scene.GetRegistry();
    auto& tc = reg.get<TransformComponent>(selectedEntity);

    auto isAncestorSelected = [&](entt::entity candidate) -> bool {
        entt::entity cur = candidate;
        while (cur != entt::null && reg.valid(cur) && scene.HasComponent<SceneTreeComponent>(cur))
        {
            entt::entity parent = scene.GetComponent<SceneTreeComponent>(cur).parent;
            if (parent == entt::null || parent == scene.GetRootEntity()) break;
            if (std::find(selectedEntities.begin(), selectedEntities.end(), parent) != selectedEntities.end())
                return true;
            cur = parent;
        }
        return false;
    };

    bool isMultiSelect = selectedEntities.size() > 1;


    if (!m_WasGizmoUsing)
    {
        if (isMultiSelect)
        {
            glm::vec3 groupCenter(0.0f);
            int validCount = 0;
            for (auto e : selectedEntities)
            {
                if (e != entt::null && reg.valid(e) && reg.any_of<TransformComponent>(e) && !isAncestorSelected(e))
                {
                    groupCenter += reg.get<TransformComponent>(e).GetWorldPosition();
                    validCount++;
                }
            }

            if (validCount > 0)
                groupCenter /= static_cast<float>(validCount);
            else
                groupCenter = tc.GetWorldPosition();

            if (m_CurrentGizmoMode == ImGuizmo::LOCAL)
                m_ActiveGizmoMatrix = glm::translate(glm::mat4(1.0f), groupCenter) * glm::mat4_cast(tc.GetWorldRotation());
            else
                m_ActiveGizmoMatrix = glm::translate(glm::mat4(1.0f), groupCenter);
        }
        else
        {
            m_ActiveGizmoMatrix = tc.GetWorldMatrix();
        }
    }

    glm::mat4 preManipulateMatrix = m_ActiveGizmoMatrix;

    glm::mat4 view = engine->GetCamera()->GetViewMatrix();
    glm::mat4 projection = engine->GetCamera()->GetProjectionMatrix();

    if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered())
    {
        if (!ImGui::GetIO().WantTextInput && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGuizmo::IsUsing())
        {
            if (ImGui::IsKeyPressed(ImGuiKey_T))
                m_CurrentGizmoMode = (m_CurrentGizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

            if (ImGui::IsKeyPressed(ImGuiKey_Q)) m_CurrentGizmoOperation = static_cast<ImGuizmo::OPERATION>(-1);
            if (ImGui::IsKeyPressed(ImGuiKey_W)) m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) m_CurrentGizmoOperation = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) m_CurrentGizmoOperation = ImGuizmo::SCALE;
        }
    }

    ImGuizmo::GetStyle().RotationLineThickness = 3.0f;
    ImGuizmo::GetStyle().ScaleLineCircleSize = 5.0f;
    ImGuizmo::GetStyle().ScaleLineThickness = 3.0f;
    ImGuizmo::GetStyle().TranslationLineArrowSize = 5.0f;
    ImGuizmo::GetStyle().TranslationLineThickness = 3.0f;

    bool isCtrlDown = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool useSnap = m_EnableSnap ? !isCtrlDown : isCtrlDown;

    float snapTranslate[3] = { m_snapTranslate, m_snapTranslate, m_snapTranslate };
    float snapScale[3] = { m_snapScale, m_snapScale, m_snapScale };
    float snapRotate[3] = { m_snapRotate, m_snapRotate, m_snapRotate };

    if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE)
    {
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
            m_CurrentGizmoOperation, m_CurrentGizmoMode,
            glm::value_ptr(m_ActiveGizmoMatrix), nullptr,
            useSnap ? snapTranslate : nullptr);
    }
    else if (m_CurrentGizmoOperation == ImGuizmo::ROTATE)
    {
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
            m_CurrentGizmoOperation, m_CurrentGizmoMode,
            glm::value_ptr(m_ActiveGizmoMatrix), nullptr,
            useSnap ? snapRotate : nullptr);
    }
    else if (m_CurrentGizmoOperation == ImGuizmo::SCALE)
    {
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
            m_CurrentGizmoOperation, m_CurrentGizmoMode,
            glm::value_ptr(m_ActiveGizmoMatrix), nullptr,
            useSnap ? snapScale : nullptr);
    }

    if (ImGuizmo::IsUsing())
    {
        if (!m_WasGizmoUsing)
        {
            m_InitialTransforms.clear();
            m_InitialGizmoMatrix = preManipulateMatrix;
            glm::vec3 sk; glm::vec4 p;
            glm::decompose(m_InitialGizmoMatrix, m_InitialGizmoScale, m_InitialGizmoRot, m_InitialGizmoPos, sk, p);

            for (auto e : selectedEntities)
            {
                if (e != entt::null && reg.valid(e) && reg.any_of<TransformComponent>(e))
                {
                    auto& eTc = reg.get<TransformComponent>(e);
                    InitialTransform initTr;
                    initTr.worldMatrix = eTc.GetWorldMatrix();
                    glm::decompose(initTr.worldMatrix, initTr.worldScale, initTr.worldRot, initTr.worldPos, sk, p);
                    m_InitialTransforms[e] = initTr;
                }
            }
            m_WasGizmoUsing = true;
        }

        auto applyWorldTransform = [&](entt::entity target, const glm::mat4& targetWorld) {
            if (!reg.valid(target) || !reg.any_of<TransformComponent>(target)) return;

            glm::mat4 parentWorld(1.0f);
            if (scene.HasComponent<SceneTreeComponent>(target))
            {
                auto& node = reg.get<SceneTreeComponent>(target);
                if (node.parent != entt::null && reg.valid(node.parent) && node.parent != scene.GetRootEntity())
                {
                    parentWorld = reg.get<TransformComponent>(node.parent).GetWorldMatrix();
                }
            }

            glm::mat4 localM = glm::inverse(parentWorld) * targetWorld;
            glm::vec3 lPos, lScale, skew;
            glm::quat lRot;
            glm::vec4 persp;
            glm::decompose(localM, lScale, lRot, lPos, skew, persp);

            auto& targetTc = reg.get<TransformComponent>(target);
            targetTc.SetPosition(lPos);
            targetTc.SetScale(lScale);
            targetTc.SetRotation(lRot);
            targetTc.SetWorldMatrix(targetWorld);
        };

        glm::vec3 curGizmoPos, curGizmoScale, sk;
        glm::quat curGizmoRot;
        glm::vec4 persp;
        glm::decompose(m_ActiveGizmoMatrix, curGizmoScale, curGizmoRot, curGizmoPos, sk, persp);

        if (!isMultiSelect)
        {
            applyWorldTransform(selectedEntity, m_ActiveGizmoMatrix);
        }
        else
        {
            for (auto e : selectedEntities)
            {
                if (isAncestorSelected(e))
                    continue;

                auto it = m_InitialTransforms.find(e);
                if (it == m_InitialTransforms.end())
                    continue;

                const auto& initTr = it->second;

                if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE)
                {
                    glm::vec3 deltaPos = curGizmoPos - m_InitialGizmoPos;
                    glm::mat4 newWorld = initTr.worldMatrix;
                    newWorld[3] = glm::vec4(initTr.worldPos + deltaPos, 1.0f);
                    applyWorldTransform(e, newWorld);
                }
                else if (m_CurrentGizmoOperation == ImGuizmo::ROTATE)
                {
                    glm::quat deltaRot = curGizmoRot * glm::inverse(m_InitialGizmoRot);

                    if (m_CurrentGizmoMode == ImGuizmo::LOCAL)
                    {
                        // Godot Local space: Each object rotates around its OWN pivot/origin!
                        glm::vec3 newPos = initTr.worldPos;
                        glm::quat newRot = deltaRot * initTr.worldRot;
                        glm::mat4 newWorld = glm::translate(glm::mat4(1.0f), newPos) * glm::mat4_cast(newRot) * glm::scale(glm::mat4(1.0f), initTr.worldScale);
                        applyWorldTransform(e, newWorld);
                    }
                    else
                    {
                        // Godot World space: Objects rotate around the GROUP CENTER pivot!
                        glm::vec3 offset = initTr.worldPos - m_InitialGizmoPos;
                        glm::vec3 newPos = m_InitialGizmoPos + deltaRot * offset;
                        glm::quat newRot = deltaRot * initTr.worldRot;
                        glm::mat4 newWorld = glm::translate(glm::mat4(1.0f), newPos) * glm::mat4_cast(newRot) * glm::scale(glm::mat4(1.0f), initTr.worldScale);
                        applyWorldTransform(e, newWorld);
                    }
                }
                else if (m_CurrentGizmoOperation == ImGuizmo::SCALE)
                {
                    glm::vec3 scaleFactor(1.0f);
                    if (std::abs(m_InitialGizmoScale.x) > 0.0001f) scaleFactor.x = curGizmoScale.x / m_InitialGizmoScale.x;
                    if (std::abs(m_InitialGizmoScale.y) > 0.0001f) scaleFactor.y = curGizmoScale.y / m_InitialGizmoScale.y;
                    if (std::abs(m_InitialGizmoScale.z) > 0.0001f) scaleFactor.z = curGizmoScale.z / m_InitialGizmoScale.z;

                    if (m_CurrentGizmoMode == ImGuizmo::LOCAL)
                    {
                        // Godot Local space: Each object scales in place along its OWN local axes / pivot!
                        glm::vec3 newPos = initTr.worldPos;
                        glm::vec3 newScale = initTr.worldScale * scaleFactor;
                        glm::mat4 newWorld = glm::translate(glm::mat4(1.0f), newPos) * glm::mat4_cast(initTr.worldRot) * glm::scale(glm::mat4(1.0f), newScale);
                        applyWorldTransform(e, newWorld);
                    }
                    else
                    {
                        // Godot World space: Objects scale relative to the GROUP CENTER!
                        glm::vec3 offset = initTr.worldPos - m_InitialGizmoPos;
                        glm::vec3 newPos = m_InitialGizmoPos + offset * scaleFactor;
                        glm::vec3 newScale = initTr.worldScale * scaleFactor;
                        glm::mat4 newWorld = glm::translate(glm::mat4(1.0f), newPos) * glm::mat4_cast(initTr.worldRot) * glm::scale(glm::mat4(1.0f), newScale);
                        applyWorldTransform(e, newWorld);
                    }
                }
            }
        }
    }
    else
    {
        m_WasGizmoUsing = false;
        m_InitialTransforms.clear();
    }
}

void Viewport::DrawToolbar(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 toolbarPos = ImVec2(displayPos.x + 10.0f, displayPos.y + 10.0f);
    ImVec2 toolbarSize = ImVec2(350.0f, 32.0f);

    drawList->AddRectFilled(toolbarPos, ImVec2(toolbarPos.x + toolbarSize.x, toolbarPos.y + toolbarSize.y),
        IM_COL32(24, 26, 30, 230), 6.0f);
    drawList->AddRect(toolbarPos, ImVec2(toolbarPos.x + toolbarSize.x, toolbarPos.y + toolbarSize.y),
        IM_COL32(50, 54, 62, 255), 6.0f, 0, 1.0f);

    ImGui::SetCursorScreenPos(ImVec2(toolbarPos.x + 4.0f, toolbarPos.y + 4.0f));

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 0.0f));

    auto ModeButton = [&](const char* label, bool active, const ImVec2& size = ImVec2(24.0f, 24.0f)) -> bool {
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.80f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.50f, 0.85f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.40f, 0.75f, 1.0f));
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.16f, 0.18f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.30f, 0.35f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.22f, 0.25f, 1.0f));
        }

        bool pressed = ImGui::Button(label, size);
        ImGui::PopStyleColor(3);
        return pressed;
        };

    if (ModeButton("P", m_CurrentGizmoOperation == static_cast<ImGuizmo::OPERATION>(-1)))
        m_CurrentGizmoOperation = static_cast<ImGuizmo::OPERATION>(-1);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select (Q)");

    ImGui::SameLine();
    if (ModeButton("T", m_CurrentGizmoOperation == ImGuizmo::TRANSLATE))
        m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate (W)");

    ImGui::SameLine();
    if (ModeButton("R", m_CurrentGizmoOperation == ImGuizmo::ROTATE))
        m_CurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");

    ImGui::SameLine();
    if (ModeButton("S", m_CurrentGizmoOperation == ImGuizmo::SCALE))
        m_CurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    const char* coordLabel = (m_CurrentGizmoMode == ImGuizmo::LOCAL) ? "Local" : "World";
    if (ModeButton(coordLabel, false, ImVec2(48.0f, 24.0f)))
    {
        m_CurrentGizmoMode = (m_CurrentGizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        EditorSettings::Get().UpdateFromViewport(*this);
        EditorSettings::Get().Save();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Transform Space (T)");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (ModeButton("Snap", m_EnableSnap, ImVec2(44.0f, 24.0f)))
    {
        m_EnableSnap = !m_EnableSnap;
        EditorSettings::Get().UpdateFromViewport(*this);
        EditorSettings::Get().Save();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Snapping (Ctrl)");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    EditorCamera* cam = engine->GetEditorCamera();

    if (ModeButton("Cam", false, ImVec2(38.0f, 24.0f)))
        ImGui::OpenPopup("CameraSettingsPopup");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Camera Settings (FOV, Speed, Clips)");

    if (cam && ImGui::BeginPopup("CameraSettingsPopup"))
    {
        ImGui::TextDisabled("Camera Settings");
        ImGui::Separator();

        bool camChanged = false;

        bool isCamOrtho = (cam->ProjectionType == Camera::Projection::Orthographic);
        if (ImGui::Checkbox("Orthographic Projection", &isCamOrtho))
        {
            cam->ProjectionType = isCamOrtho ? Camera::Projection::Orthographic : Camera::Projection::Perspective;
            cam->UpdateFrustum();
            camChanged = true;
        }

        if (cam->ProjectionType == Camera::Projection::Perspective)
        {
            ImGui::SetNextItemWidth(120);
            if (ImGui::SliderFloat("FOV", &cam->FOV, 20.0f, 130.0f, "%.0f deg"))
            {
                cam->UpdateFrustum();
                camChanged = true;
            }
        }
        else
        {
            ImGui::SetNextItemWidth(120);
            if (ImGui::DragFloat("Ortho Size", &cam->OrthoSize, 0.2f, 0.1f, 200.0f, "%.1f"))
            {
                cam->UpdateFrustum();
                camChanged = true;
            }
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Movement & Controls");
        ImGui::Separator();

        ImGui::SetNextItemWidth(120);
        if (ImGui::SliderFloat("Speed", &cam->MovementSpeed, 1.0f, 50.0f, "%.1f")) camChanged = true;

        ImGui::SetNextItemWidth(120);
        if (ImGui::SliderFloat("Sprint Speed", &cam->SprintSpeed, 2.0f, 100.0f, "%.1f")) camChanged = true;

        ImGui::SetNextItemWidth(120);
        if (ImGui::SliderFloat("Sensitivity", &cam->MouseSensitivity, 0.01f, 0.5f, "%.3f")) camChanged = true;

        ImGui::SetNextItemWidth(120);
        if (ImGui::SliderFloat("Smoothness", &cam->positionSmoothness, 1.0f, 50.0f, "%.1f")) camChanged = true;

        ImGui::Spacing();
        ImGui::TextDisabled("Clipping Planes");
        ImGui::Separator();

        ImGui::SetNextItemWidth(120);
        if (ImGui::DragFloat("Near Clip", &cam->NearClip, 0.01f, 0.001f, 10.0f, "%.3f"))
        {
            cam->UpdateFrustum();
            camChanged = true;
        }

        ImGui::SetNextItemWidth(120);
        if (ImGui::DragFloat("Far Clip", &cam->FarClip, 10.0f, 10.0f, 50000.0f, "%.0f"))
        {
            cam->UpdateFrustum();
            camChanged = true;
        }

        if (camChanged)
        {
            EditorSettings::Get().UpdateFromCamera(*cam);
            EditorSettings::Get().Save();
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (ModeButton("View", false, ImVec2(42.0f, 24.0f)))
        ImGui::OpenPopup("ViewportSettingsPopup");

    auto& debugSettings = DebugRenderer::Get().GetSettings();
    if (ImGui::BeginPopup("ViewportSettingsPopup"))
    {
        bool viewChanged = false;

        ImGui::TextDisabled("Debug Visuals");
        ImGui::Separator();
        if (ImGui::Checkbox("Draw Colliders", &debugSettings.drawColliders)) viewChanged = true;
        if (ImGui::Checkbox("Draw Bounding Boxes", &debugSettings.drawBoundingBoxes)) viewChanged = true;

        ImGui::Spacing();
        ImGui::TextDisabled("Snapping Values");
        ImGui::Separator();

        ImGui::SetNextItemWidth(80);
        if (ImGui::DragFloat("Translate (m)", &m_snapTranslate, 0.05f, 0.01f, 100.0f, "%.2f")) viewChanged = true;

        ImGui::SetNextItemWidth(80);
        if (ImGui::DragFloat("Rotate (deg)", &m_snapRotate, 1.0f, 0.1f, 180.0f, "%.1f")) viewChanged = true;

        ImGui::SetNextItemWidth(80);
        if (ImGui::DragFloat("Scale", &m_snapScale, 0.05f, 0.01f, 10.0f, "%.2f")) viewChanged = true;

        if (viewChanged)
        {
            EditorSettings::Get().UpdateFromViewport(*this);
            EditorSettings::Get().Save();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);
}

void Viewport::DrawOverlayStats(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    bool isCtrlDown = ImGui::GetIO().KeyCtrl;
    bool isSnappingActive = m_EnableSnap ? !isCtrlDown : isCtrlDown;
    if (isSnappingActive)
    {
        const char* label = "";
        float val = 0.0f;
        if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE) { label = "Snap: %.2fm"; val = m_snapTranslate; }
        else if (m_CurrentGizmoOperation == ImGuizmo::ROTATE) { label = "Snap: %.1f°"; val = m_snapRotate; }
        else if (m_CurrentGizmoOperation == ImGuizmo::SCALE) { label = "Snap: %.2f";  val = m_snapScale; }

        char buf[64];
        snprintf(buf, sizeof(buf), label, val);
        ImVec2 textPos = ImVec2(displayPos.x + 12.0f, displayPos.y + 48.0f);
        ImVec2 snapTextSize = ImGui::CalcTextSize(buf);
        drawList->AddRectFilled(ImVec2(textPos.x - 4, textPos.y - 2), ImVec2(textPos.x + snapTextSize.x + 6, textPos.y + snapTextSize.y + 4), IM_COL32(18, 20, 24, 200), 4.0f);
        drawList->AddText(textPos, IM_COL32(245, 195, 65, 255), buf);
    }

    char statBuf[128];
    snprintf(statBuf, sizeof(statBuf), "%.0f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImVec2 statTextSize = ImGui::CalcTextSize(statBuf);
    ImVec2 statPos = ImVec2(displayPos.x + displaySize.x - statTextSize.x - 14.0f, displayPos.y + 115.0f);

    drawList->AddRectFilled(ImVec2(statPos.x - 6.0f, statPos.y - 4.0f), ImVec2(statPos.x + statTextSize.x + 6.0f, statPos.y + statTextSize.y + 4.0f),
        IM_COL32(18, 20, 24, 180), 4.0f);
    drawList->AddText(statPos, IM_COL32(160, 165, 175, 255), statBuf);
}

void Viewport::DrawOrientationGizmo(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    EditorCamera* cam = engine->GetEditorCamera();
    if (!cam) return;

    const float gizmoSize = 96.0f;
    const float margin = 10.0f;
    ImVec2 gizmoPos = ImVec2(displayPos.x + displaySize.x - gizmoSize - margin, displayPos.y + margin);

    glm::mat4 viewMatrix = cam->GetViewMatrix();

    ImGuizmo::SetDrawlist();
    ImGuizmo::ViewManipulate(
        glm::value_ptr(viewMatrix),
        5.0f,
        gizmoPos,
        ImVec2(gizmoSize, gizmoSize),
        IM_COL32(24, 26, 30, 210)
    );

    glm::mat4 invView = glm::inverse(viewMatrix);
    glm::vec3 newForward = -glm::vec3(invView[2]);

    if (glm::length(newForward) > 0.001f && glm::dot(cam->Front, newForward) < 0.9999f)
    {
        glm::vec3 focusPoint = cam->Position + cam->Front * 5.0f;
        cam->SetDirection(newForward, &focusPoint);
    }
}

void Viewport::HandleSelection(Engine* engine, ImVec2& displayPos, ImVec2& displaySize, entt::entity& selectedEntity, std::vector<entt::entity>& selectedEntities)
{
    if (engine->GetActiveScene().GetState() != SceneState::EDIT) return;

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool isMouseInViewport = mousePos.x >= displayPos.x && mousePos.x <= (displayPos.x + displaySize.x) &&
        mousePos.y >= displayPos.y && mousePos.y <= (displayPos.y + displaySize.y);

    const float gizmoSize = 96.0f;
    const float margin = 10.0f;
    bool isOverOrientationGizmo = mousePos.x >= (displayPos.x + displaySize.x - gizmoSize - margin) &&
        mousePos.x <= (displayPos.x + displaySize.x - margin) &&
        mousePos.y >= (displayPos.y + margin) &&
        mousePos.y <= (displayPos.y + margin + gizmoSize);

    bool isOverToolbar = mousePos.x >= (displayPos.x + 10.0f) &&
        mousePos.x <= (displayPos.x + 360.0f) &&
        mousePos.y >= (displayPos.y + 10.0f) &&
        mousePos.y <= (displayPos.y + 42.0f);

    if (isMouseInViewport && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() &&
        !isOverOrientationGizmo && !isOverToolbar)
    {
        glm::vec2 mouseViewportPos = { mousePos.x - displayPos.x, mousePos.y - displayPos.y };

        auto& context = engine->GetRenderLayer().GetRenderContext();
        float fboWidth = context.viewportWidth;
        float fboHeight = context.viewportHeight;

        uint32_t fboX = static_cast<uint32_t>(mouseViewportPos.x * fboWidth / displaySize.x);
        uint32_t fboY = static_cast<uint32_t>(fboHeight - ((displaySize.y - mouseViewportPos.y) * fboHeight / displaySize.y));

        uint32_t pickedID = engine->Selection.Pick(fboX, fboY);

        ImGuiIO& io = ImGui::GetIO();
        bool isMultiModifier = io.KeyCtrl || io.KeyShift;

        if (pickedID != 0)
        {
            entt::entity entity = static_cast<entt::entity>(pickedID);
            if (isMultiModifier)
            {
                auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
                if (it != selectedEntities.end())
                {
                    selectedEntities.erase(it);
                    if (selectedEntity == entity)
                    {
                        selectedEntity = selectedEntities.empty() ? entt::null : selectedEntities.back();
                    }
                }
                else
                {
                    selectedEntities.push_back(entity);
                    selectedEntity = entity;
                }
            }
            else
            {
                selectedEntities = { entity };
                selectedEntity = entity;
            }
            engine->GetActiveScene().SetSelectedEntity(selectedEntity);
        }
        else
        {
            if (!isMultiModifier)
            {
                selectedEntities.clear();
                selectedEntity = entt::null;
                engine->GetActiveScene().SetSelectedEntity(entt::null);
            }
        }
    }
}

void Viewport::Draw(Engine* engine, entt::entity& selectedEntity, std::vector<entt::entity>& selectedEntities)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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

        ImTextureID textureID = (ImTextureID)(uintptr_t)engine->GetRenderLayer().GetFinalTexture();

        ImGui::GetWindowDrawList()->AddImage(
            textureID,
            displayPos,
            ImVec2(displayPos.x + displaySize.x, displayPos.y + displaySize.y),
            ImVec2(0, 1),
            ImVec2(1, 0),
            IM_COL32_WHITE
        );

        ImVec2 mousePos = ImGui::GetIO().MousePos;
        if (displaySize.x > 0.0f && displaySize.y > 0.0f)
        {
            float normX = (mousePos.x - displayPos.x) / displaySize.x;
            float normY = (mousePos.y - displayPos.y) / displaySize.y;
            Input::SetViewportMousePos(glm::vec2(normX * 1920.0f, normY * 1080.0f));
        }

        DrawGizmos(engine, displayPos, displaySize, selectedEntity, selectedEntities);
        HandleSelection(engine, displayPos, displaySize, selectedEntity, selectedEntities);

        if(engine->GetActiveScene().GetState() == SceneState::EDIT)
        {
            DrawToolbar(engine, displayPos, displaySize);
            DrawOrientationGizmo(engine, displayPos, displaySize);
        }
        DrawOverlayStats(engine, displayPos, displaySize);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
