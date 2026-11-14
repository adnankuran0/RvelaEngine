#include "Viewport.h"
#include <unordered_set>
#include <iostream>
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/ImGuizmo.h"
#include "Core/Engine.h"
#include "Core/Log.h"
#include <glm/gtx/matrix_decompose.hpp>
#include "Renderer/RenderLayer.h"
#include "Renderer/DebugRenderer.h"
#include "Input/Input.h"
#include "Renderer/EditorCamera.h"
#include "Render/GizmoPass.h"
#include "EditorSettings.h"
#include "EditorSelection.h"
#include "EditorUtils.h"
#include "Scene/Entity.h"
#include "Scene/Components/PrefabComponent.h"
#include "PrefabEditManager.h"
#include "Commands/CommandHistory.h"
#include "Commands/TransformCommand.h"
#include "Commands/RectTransformCommand.h"
#include "Commands/EntityLifecycleCommands.h"
#include "UI/UILayout.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/DirectionalLightComponent.h"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/SpotLightComponent.h"
#include "Scene/Components/AudioEmitterComponent.h"
#include "Scene/Components/ParticleEmitterComponent.h"
#include "Scene/Components/DecalComponent.h"
#include "Scene/Components/ReflectionProbeComponent.h"
#include "Scene/Components/UIComponents.h"
#include "Scene/Components/ColliderComponents.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "Scene/Components/ScriptComponent.h"
#include "Scene/Components/SkeletalMeshComponent.h"
#include "Scene/Components/TagComponent.h"
#include "Scene/SceneSerializer.h"
#include "Commands/ComponentCommands.h"
#include "Script/ScriptEngine.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetMeta.h"
#include "Math/RvelaMath.h"
#include "Physics/PhysicsSystem.h"
#include "Physics/PhysicsWorld.h"
#include "Style.h"

using namespace rv;

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

bool Viewport::ContainsPoint(const glm::vec2& point) const
{
	if (!m_IsOpen)
		return false;
	return point.x >= m_PanelPosition.x && point.x <= m_PanelPosition.x + m_PanelSize.x &&
		point.y >= m_PanelPosition.y && point.y <= m_PanelPosition.y + m_PanelSize.y;
}

void Viewport::DrawGizmos(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    entt::entity selectedEntity = EditorSelection::Get().GetPrimary();
    const auto& selectedEntities = EditorSelection::Get().GetSelectedEntities();

    auto& scene = engine->GetActiveScene();
    auto& reg = scene.GetRegistry();

    bool hasTransform = selectedEntity != entt::null &&
        reg.valid(selectedEntity) &&
        (reg.any_of<TransformComponent>(selectedEntity) || reg.any_of<RectTransformComponent>(selectedEntity));

    bool canDrawGizmo = m_DrawGizmos &&
        hasTransform &&
        scene.GetState() == SceneState::EDIT;
    if (!canDrawGizmo)
    {
        m_WasGizmoUsing = false;
        m_WasUIGizmo = false;
        m_InitialTransforms.clear();
        m_InitialRectTransforms.clear();
        return;
    }

    bool isUI = false;
    if (reg.any_of<RectTransformComponent>(selectedEntity))
    {
        isUI = true;
        if (reg.any_of<UICanvasComponent>(selectedEntity))
        {
            const auto& canvas = reg.get<UICanvasComponent>(selectedEntity);
            if (canvas.mode == CanvasMode::WorldSpace)
                isUI = false;
        }
    }

    if (isUI)
    {
        if (m_WasGizmoUsing && !m_WasUIGizmo)
        {
            m_WasGizmoUsing = false;
            m_InitialTransforms.clear();
        }
        DrawUIGizmos(engine, displayPos, displaySize);
        return;
    }
    else
    {
        if (m_WasGizmoUsing && m_WasUIGizmo)
        {
            m_WasGizmoUsing = false;
            m_WasUIGizmo = false;
            m_InitialRectTransforms.clear();
        }
    }

    ImGuizmo::BeginFrame();
    bool isOrtho = engine->GetCamera() && (engine->GetCamera()->ProjectionType == Camera::Projection::Orthographic);
    ImGuizmo::SetOrthographic(isOrtho);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(displayPos.x, displayPos.y, displaySize.x, displaySize.y);

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
                    initTr.localPos = eTc.GetPosition();
                    initTr.localRot = eTc.GetRotation();
                    initTr.localScale = eTc.GetScale();
                    if (reg.any_of<UUIDComponent>(e))
                        initTr.uuid = reg.get<UUIDComponent>(e).uuid;
                    m_InitialTransforms[e] = initTr;
                }
            }
            m_WasGizmoUsing = true;
            engine->GetActiveScene().SetDirty(true);
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
            targetTc.SetDirty(true);
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
                        glm::vec3 newPos = initTr.worldPos;
                        glm::quat newRot = deltaRot * initTr.worldRot;
                        glm::mat4 newWorld = glm::translate(glm::mat4(1.0f), newPos) * glm::mat4_cast(newRot) * glm::scale(glm::mat4(1.0f), initTr.worldScale);
                        applyWorldTransform(e, newWorld);
                    }
                    else
                    {
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
                        glm::vec3 newPos = initTr.worldPos;
                        glm::vec3 newScale = initTr.worldScale * scaleFactor;
                        glm::mat4 newWorld = glm::translate(glm::mat4(1.0f), newPos) * glm::mat4_cast(initTr.worldRot) * glm::scale(glm::mat4(1.0f), newScale);
                        applyWorldTransform(e, newWorld);
                    }
                    else
                    {
                        glm::vec3 offset = initTr.worldPos - m_InitialGizmoPos;
                        glm::vec3 newPos = m_InitialGizmoPos + offset * scaleFactor;
                        glm::vec3 newScale = initTr.worldScale * scaleFactor;
                        glm::mat4 newWorld = glm::translate(glm::mat4(1.0f), newPos) * glm::mat4_cast(initTr.worldRot) * glm::scale(glm::mat4(1.0f), newScale);
                        applyWorldTransform(e, newWorld);
                    }
                }
            }
        }

        scene.GetTransformSystem().Update();
    }
    else
    {
        if (m_WasGizmoUsing)
        {
            Scene& activeScene = engine->GetActiveScene();
            auto& currentReg = activeScene.GetRegistry();

            std::vector<EntityTransformRecord> beforeRecords;
            std::vector<EntityTransformRecord> afterRecords;

            for (const auto& [entityHandle, initTr] : m_InitialTransforms)
            {
                if (initTr.uuid != 0 && currentReg.valid(entityHandle) && currentReg.any_of<TransformComponent>(entityHandle))
                {
                    auto& curTc = currentReg.get<TransformComponent>(entityHandle);

                    bool changed = (curTc.GetPosition() != initTr.localPos) ||
                                   (curTc.GetRotation() != initTr.localRot) ||
                                   (curTc.GetScale() != initTr.localScale);

                    if (changed)
                    {
                        beforeRecords.push_back({ initTr.uuid, entityHandle, initTr.localPos, initTr.localRot, initTr.localScale });
                        afterRecords.push_back({ initTr.uuid, entityHandle, curTc.GetPosition(), curTc.GetRotation(), curTc.GetScale() });
                    }
                }
            }

            if (!beforeRecords.empty())
            {
                auto cmd = std::make_unique<TransformCommand>(&activeScene, beforeRecords, afterRecords);
                CommandHistory::Get().Execute(std::move(cmd));
            }

            activeScene.SetDirty(true);
            activeScene.GetTransformSystem().Update();
        }
        m_WasGizmoUsing = false;
        m_WasUIGizmo = false;
        m_InitialTransforms.clear();
        m_InitialRectTransforms.clear();
    }
}

void Viewport::DrawUIGizmos(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    entt::entity selectedEntity = EditorSelection::Get().GetPrimary();
    const auto& selectedEntities = EditorSelection::Get().GetSelectedEntities();

    auto& scene = engine->GetActiveScene();
    auto& reg = scene.GetRegistry();

    float targetW = (engine && engine->GetRenderLayer().GetTargetWidth() > 0)
        ? static_cast<float>(engine->GetRenderLayer().GetTargetWidth())
        : displaySize.x;
    float targetH = (engine && engine->GetRenderLayer().GetTargetHeight() > 0)
        ? static_cast<float>(engine->GetRenderLayer().GetTargetHeight())
        : displaySize.y;

    if (targetW <= 0.0f) targetW = 1920.0f;
    if (targetH <= 0.0f) targetH = 1080.0f;

    UILayout::UpdateCanvasLayouts(&scene, targetW, targetH);

    auto uiToScreen = [&](const glm::vec2& p) -> ImVec2 {
        float normX = p.x / targetW;
        float normY = 1.0f - (p.y / targetH);
        return ImVec2(displayPos.x + normX * displaySize.x, displayPos.y + normY * displaySize.y);
    };

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for (auto e : selectedEntities)
    {
        if (e != entt::null && reg.valid(e) && reg.any_of<RectTransformComponent>(e))
        {
            const auto& r = reg.get<RectTransformComponent>(e);
            float w = r.computedScreenRect.z - r.computedScreenRect.x;
            float h = r.computedScreenRect.w - r.computedScreenRect.y;
            float hw = w * 0.5f;
            float hh = h * 0.5f;
            glm::vec2 c((r.computedScreenRect.x + r.computedScreenRect.z) * 0.5f,
                        (r.computedScreenRect.y + r.computedScreenRect.w) * 0.5f);

            float rad = glm::radians(r.computedRotation);
            float cosR = std::cos(rad);
            float sinR = std::sin(rad);

            glm::vec2 rel0 = { -hw, -hh };
            glm::vec2 rel1 = {  hw, -hh };
            glm::vec2 rel2 = {  hw,  hh };
            glm::vec2 rel3 = { -hw,  hh };

            glm::vec2 p0 = c + glm::vec2(rel0.x * cosR - rel0.y * sinR, rel0.x * sinR + rel0.y * cosR);
            glm::vec2 p1 = c + glm::vec2(rel1.x * cosR - rel1.y * sinR, rel1.x * sinR + rel1.y * cosR);
            glm::vec2 p2 = c + glm::vec2(rel2.x * cosR - rel2.y * sinR, rel2.x * sinR + rel2.y * cosR);
            glm::vec2 p3 = c + glm::vec2(rel3.x * cosR - rel3.y * sinR, rel3.x * sinR + rel3.y * cosR);

            ImVec2 s0 = uiToScreen(p0);
            ImVec2 s1 = uiToScreen(p1);
            ImVec2 s2 = uiToScreen(p2);
            ImVec2 s3 = uiToScreen(p3);

            ImU32 outlineColor = (e == selectedEntity) ? IM_COL32(50, 150, 255, 230) : IM_COL32(80, 180, 255, 140);
            drawList->AddQuad(s0, s1, s2, s3, outlineColor, 1.5f);

            const float handleSize = 3.5f;
            auto drawHandle = [&](const ImVec2& pt) {
                drawList->AddRectFilled(ImVec2(pt.x - handleSize, pt.y - handleSize),
                                        ImVec2(pt.x + handleSize, pt.y + handleSize),
                                        IM_COL32(255, 255, 255, 240), 1.0f);
                drawList->AddRect(ImVec2(pt.x - handleSize, pt.y - handleSize),
                                  ImVec2(pt.x + handleSize, pt.y + handleSize),
                                  IM_COL32(40, 120, 220, 255), 1.0f);
            };
            drawHandle(s0);
            drawHandle(s1);
            drawHandle(s2);
            drawHandle(s3);
        }
    }

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
    auto& primaryRect = reg.get<RectTransformComponent>(selectedEntity);

    if (!m_WasGizmoUsing)
    {
        glm::vec2 gizmoCenter(0.0f);
        if (isMultiSelect)
        {
            int validCount = 0;
            for (auto e : selectedEntities)
            {
                if (e != entt::null && reg.valid(e) && reg.any_of<RectTransformComponent>(e) && !isAncestorSelected(e))
                {
                    const auto& r = reg.get<RectTransformComponent>(e);
                    gizmoCenter += glm::vec2((r.computedScreenRect.x + r.computedScreenRect.z) * 0.5f,
                                             (r.computedScreenRect.y + r.computedScreenRect.w) * 0.5f);
                    validCount++;
                }
            }
            if (validCount > 0)
                gizmoCenter /= static_cast<float>(validCount);
            else
                gizmoCenter = glm::vec2((primaryRect.computedScreenRect.x + primaryRect.computedScreenRect.z) * 0.5f,
                                        (primaryRect.computedScreenRect.y + primaryRect.computedScreenRect.w) * 0.5f);
        }
        else
        {
            gizmoCenter = glm::vec2((primaryRect.computedScreenRect.x + primaryRect.computedScreenRect.z) * 0.5f,
                                    (primaryRect.computedScreenRect.y + primaryRect.computedScreenRect.w) * 0.5f);
        }

        glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(gizmoCenter.x, gizmoCenter.y, 0.0f));
        glm::mat4 R = glm::mat4(1.0f);
        if (m_CurrentGizmoMode == ImGuizmo::LOCAL)
            R = glm::rotate(glm::mat4(1.0f), glm::radians(primaryRect.computedRotation), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(primaryRect.scale.x, primaryRect.scale.y, 1.0f));
        m_ActiveGizmoMatrix = T * R * S;
    }

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(displayPos.x, displayPos.y, displaySize.x, displaySize.y);

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::ortho(0.0f, targetW, 0.0f, targetH, -1000.0f, 1000.0f);

    ImGuizmo::GetStyle().RotationLineThickness = 3.0f;
    ImGuizmo::GetStyle().ScaleLineCircleSize = 5.0f;
    ImGuizmo::GetStyle().ScaleLineThickness = 3.0f;
    ImGuizmo::GetStyle().TranslationLineArrowSize = 5.0f;
    ImGuizmo::GetStyle().TranslationLineThickness = 3.0f;

    bool isCtrlDown = ImGui::GetIO().KeyCtrl || ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool useSnap = m_EnableSnap ? !isCtrlDown : isCtrlDown;

    float snapTranslate[3] = { m_snapTranslateUI, m_snapTranslateUI, m_snapTranslateUI };
    float snapScale[3] = { m_snapScale, m_snapScale, m_snapScale };
    float snapRotate[3] = { m_snapRotate, m_snapRotate, m_snapRotate };

    ImGuizmo::OPERATION uiOp = static_cast<ImGuizmo::OPERATION>(0);
    float* snapPtr = nullptr;
    if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE)
    {
        uiOp = static_cast<ImGuizmo::OPERATION>(ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y);
        if (useSnap) snapPtr = snapTranslate;
    }
    else if (m_CurrentGizmoOperation == ImGuizmo::ROTATE)
    {
        uiOp = ImGuizmo::ROTATE_Z;
        if (useSnap) snapPtr = snapRotate;
    }
    else if (m_CurrentGizmoOperation == ImGuizmo::SCALE)
    {
        uiOp = static_cast<ImGuizmo::OPERATION>(ImGuizmo::SCALE_X | ImGuizmo::SCALE_Y);
        if (useSnap) snapPtr = snapScale;
    }

    glm::mat4 preManipulateMatrix = m_ActiveGizmoMatrix;

    if (uiOp != 0)
    {
        ImGuizmo::Manipulate(
            glm::value_ptr(view), glm::value_ptr(projection),
            uiOp, m_CurrentGizmoMode,
            glm::value_ptr(m_ActiveGizmoMatrix), nullptr,
            snapPtr);
    }

    if (ImGuizmo::IsUsing())
    {
        if (!m_WasGizmoUsing)
        {
            m_InitialTransforms.clear();
            m_InitialRectTransforms.clear();
            m_InitialGizmoMatrix = preManipulateMatrix;
            glm::vec3 sk; glm::vec4 p;
            glm::decompose(m_InitialGizmoMatrix, m_InitialGizmoScale, m_InitialGizmoRot, m_InitialGizmoPos, sk, p);

            for (auto e : selectedEntities)
            {
                if (e != entt::null && reg.valid(e) && reg.any_of<RectTransformComponent>(e))
                {
                    auto& r = reg.get<RectTransformComponent>(e);
                    InitialRectTransform initR;
                    initR.localPos = r.position;
                    initR.size = r.size;
                    initR.rotation = r.rotation;
                    initR.scale = r.scale;
                    initR.screenCenter = glm::vec2((r.computedScreenRect.x + r.computedScreenRect.z) * 0.5f,
                                                   (r.computedScreenRect.y + r.computedScreenRect.w) * 0.5f);
                    if (reg.any_of<UUIDComponent>(e))
                        initR.uuid = reg.get<UUIDComponent>(e).uuid;
                    m_InitialRectTransforms[e] = initR;
                }
            }
            m_WasGizmoUsing = true;
            m_WasUIGizmo = true;
            engine->GetActiveScene().SetDirty(true);
        }

        glm::vec3 curGizmoPos, curGizmoScale, sk;
        glm::quat curGizmoRot;
        glm::vec4 persp;
        glm::decompose(m_ActiveGizmoMatrix, curGizmoScale, curGizmoRot, curGizmoPos, sk, persp);

        auto getParentRotation = [&](entt::entity target) -> float {
            if (scene.HasComponent<SceneTreeComponent>(target))
            {
                entt::entity parent = scene.GetComponent<SceneTreeComponent>(target).parent;
                if (parent != entt::null && reg.valid(parent) && parent != scene.GetRootEntity())
                {
                    if (reg.any_of<RectTransformComponent>(parent))
                        return reg.get<RectTransformComponent>(parent).computedRotation;
                }
            }
            return 0.0f;
        };

        if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE)
        {
            glm::vec2 deltaScreen = glm::vec2(curGizmoPos.x - m_InitialGizmoPos.x, curGizmoPos.y - m_InitialGizmoPos.y);
            if (useSnap && m_snapTranslateUI > 0.001f)
            {
                deltaScreen.x = std::round(deltaScreen.x / m_snapTranslateUI) * m_snapTranslateUI;
                deltaScreen.y = std::round(deltaScreen.y / m_snapTranslateUI) * m_snapTranslateUI;
            }

            for (auto e : selectedEntities)
            {
                if (isAncestorSelected(e)) continue;
                auto it = m_InitialRectTransforms.find(e);
                if (it == m_InitialRectTransforms.end()) continue;

                float parentRot = getParentRotation(e);
                glm::vec2 deltaLocal = (parentRot != 0.0f) ? UILayout::RotatePoint(deltaScreen, glm::vec2(0.0f), -parentRot) : deltaScreen;

                auto& r = reg.get<RectTransformComponent>(e);
                r.position = it->second.localPos + deltaLocal;
            }
        }
        else if (m_CurrentGizmoOperation == ImGuizmo::ROTATE)
        {
            float curAngle = glm::degrees(std::atan2(m_ActiveGizmoMatrix[0][1], m_ActiveGizmoMatrix[0][0]));
            float initAngle = glm::degrees(std::atan2(m_InitialGizmoMatrix[0][1], m_InitialGizmoMatrix[0][0]));
            float deltaAngle = curAngle - initAngle;
            if (useSnap && m_snapRotate > 0.001f)
            {
                deltaAngle = std::round(deltaAngle / m_snapRotate) * m_snapRotate;
            }

            for (auto e : selectedEntities)
            {
                if (isAncestorSelected(e)) continue;
                auto it = m_InitialRectTransforms.find(e);
                if (it == m_InitialRectTransforms.end()) continue;
                const auto& initR = it->second;
                auto& r = reg.get<RectTransformComponent>(e);

                if (!isMultiSelect || m_CurrentGizmoMode == ImGuizmo::LOCAL)
                {
                    r.rotation = initR.rotation + deltaAngle;
                }
                else
                {
                    r.rotation = initR.rotation + deltaAngle;
                    glm::vec2 groupCenter2D(m_InitialGizmoPos.x, m_InitialGizmoPos.y);
                    glm::vec2 newCenter = UILayout::RotatePoint(initR.screenCenter, groupCenter2D, deltaAngle);
                    glm::vec2 deltaScreen = newCenter - initR.screenCenter;

                    float parentRot = getParentRotation(e);
                    glm::vec2 deltaLocal = (parentRot != 0.0f) ? UILayout::RotatePoint(deltaScreen, glm::vec2(0.0f), -parentRot) : deltaScreen;
                    r.position = initR.localPos + deltaLocal;
                }
            }
        }
        else if (m_CurrentGizmoOperation == ImGuizmo::SCALE)
        {
            glm::vec2 scaleFactor(1.0f);
            if (std::abs(m_InitialGizmoScale.x) > 0.0001f) scaleFactor.x = curGizmoScale.x / m_InitialGizmoScale.x;
            if (std::abs(m_InitialGizmoScale.y) > 0.0001f) scaleFactor.y = curGizmoScale.y / m_InitialGizmoScale.y;
            if (useSnap && m_snapScale > 0.001f)
            {
                scaleFactor.x = std::round(scaleFactor.x / m_snapScale) * m_snapScale;
                scaleFactor.y = std::round(scaleFactor.y / m_snapScale) * m_snapScale;
            }

            for (auto e : selectedEntities)
            {
                if (isAncestorSelected(e)) continue;
                auto it = m_InitialRectTransforms.find(e);
                if (it == m_InitialRectTransforms.end()) continue;
                const auto& initR = it->second;
                auto& r = reg.get<RectTransformComponent>(e);

                if (!isMultiSelect || m_CurrentGizmoMode == ImGuizmo::LOCAL)
                {
                    r.scale = initR.scale * scaleFactor;
                    r.scale.x = std::max(0.001f, r.scale.x);
                    r.scale.y = std::max(0.001f, r.scale.y);
                }
                else
                {
                    r.scale = initR.scale * scaleFactor;
                    r.scale.x = std::max(0.001f, r.scale.x);
                    r.scale.y = std::max(0.001f, r.scale.y);

                    glm::vec2 groupCenter2D(m_InitialGizmoPos.x, m_InitialGizmoPos.y);
                    glm::vec2 offset = initR.screenCenter - groupCenter2D;
                    glm::vec2 newCenter = groupCenter2D + offset * scaleFactor;
                    glm::vec2 deltaScreen = newCenter - initR.screenCenter;

                    float parentRot = getParentRotation(e);
                    glm::vec2 deltaLocal = (parentRot != 0.0f) ? UILayout::RotatePoint(deltaScreen, glm::vec2(0.0f), -parentRot) : deltaScreen;
                    r.position = initR.localPos + deltaLocal;
                }
            }
        }

        UILayout::UpdateCanvasLayouts(&scene, targetW, targetH);
        scene.SetDirty(true);
    }
    else
    {
        if (m_WasGizmoUsing && m_WasUIGizmo)
        {
            Scene& activeScene = engine->GetActiveScene();
            auto& currentReg = activeScene.GetRegistry();

            std::vector<EntityRectTransformRecord> beforeRecords;
            std::vector<EntityRectTransformRecord> afterRecords;

            for (const auto& [entityHandle, initR] : m_InitialRectTransforms)
            {
                if (initR.uuid != 0 && currentReg.valid(entityHandle) && currentReg.any_of<RectTransformComponent>(entityHandle))
                {
                    auto& curR = currentReg.get<RectTransformComponent>(entityHandle);
                    bool changed = (curR.position != initR.localPos) ||
                                   (curR.size != initR.size) ||
                                   (curR.rotation != initR.rotation) ||
                                   (curR.scale != initR.scale);
                    if (changed)
                    {
                        beforeRecords.push_back({ initR.uuid, entityHandle, initR.localPos, initR.size, initR.rotation, initR.scale });
                        afterRecords.push_back({ initR.uuid, entityHandle, curR.position, curR.size, curR.rotation, curR.scale });
                    }
                }
            }

            if (!beforeRecords.empty())
            {
                auto cmd = std::make_unique<RectTransformCommand>(&activeScene, beforeRecords, afterRecords);
                CommandHistory::Get().Execute(std::move(cmd));
            }

            activeScene.SetDirty(true);
            UILayout::UpdateCanvasLayouts(&activeScene, targetW, targetH);
        }
        m_WasGizmoUsing = false;
        m_WasUIGizmo = false;
        m_InitialRectTransforms.clear();
    }
}

void Viewport::DrawPrefabBanner(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    if (!PrefabEditManager::Get().IsInPrefabMode())
        return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const std::string& prefabName = PrefabEditManager::Get().GetPrefabName();
    
    char bannerText[256];
    snprintf(bannerText, sizeof(bannerText), "Prefab: %s.rprefab", prefabName.c_str());

    const float btnBackW = 75.0f;
    const float btnSaveW = 55.0f;
    const float btnDiscardW = 65.0f;
    const float textW = ImGui::CalcTextSize(bannerText).x;
    const float padding = 12.0f;
    const float bannerW = btnBackW + 8.0f + textW + 16.0f + btnSaveW + 6.0f + btnDiscardW + padding * 2.0f;
    const float bannerH = 32.0f;

    const float bannerX = displayPos.x + std::max(10.0f, (displaySize.x - bannerW) * 0.5f);
    const float bannerY = displayPos.y + 10.0f;

    const ImVec2 rectMin(bannerX, bannerY);
    const ImVec2 rectMax(bannerX + bannerW, bannerY + bannerH);

    drawList->AddRectFilled(rectMin, rectMax, IM_COL32(20, 32, 52, 235), 6.0f);
    drawList->AddRect(rectMin, rectMax, IM_COL32(45, 105, 180, 255), 6.0f, 0, 1.5f);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 0.0f));

    ImGui::SetCursorScreenPos(ImVec2(bannerX + padding, bannerY + 4.0f));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.35f, 0.55f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.45f, 0.70f, 1.0f));
    if (ImGui::Button("<- Scene", ImVec2(btnBackW, 24.0f)))
    {
        PrefabEditManager::Get().ExitPrefabMode(*engine, true);
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.40f, 0.78f, 1.0f, 1.0f), "%s", bannerText);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.52f, 0.28f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.65f, 0.35f, 1.0f));
    if (ImGui::Button("Save", ImVec2(btnSaveW, 24.0f)))
    {
        PrefabEditManager::Get().SaveCurrentPrefab(*engine);
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.22f, 0.22f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.28f, 0.28f, 1.0f));
    if (ImGui::Button("Discard", ImVec2(btnDiscardW, 24.0f)))
    {
        PrefabEditManager::Get().ExitPrefabMode(*engine, false);
    }
    ImGui::PopStyleColor(2);

    ImGui::PopStyleVar(2);
}

void Viewport::DrawToolbar(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImVec2 leftToolbarPos = ImVec2(displayPos.x + 10.0f, displayPos.y + 10.0f);
    const ImVec2 leftToolbarSize = ImVec2(308.0f, 32.0f);

    drawList->AddRectFilled(leftToolbarPos, ImVec2(leftToolbarPos.x + leftToolbarSize.x, leftToolbarPos.y + leftToolbarSize.y),
        IM_COL32(24, 26, 30, 230), 6.0f);
    drawList->AddRect(leftToolbarPos, ImVec2(leftToolbarPos.x + leftToolbarSize.x, leftToolbarPos.y + leftToolbarSize.y),
        IM_COL32(50, 54, 62, 255), 6.0f, 0, 1.0f);

    const float cubeSize = 96.0f;
    const float margin = 10.0f;
    const ImVec2 rightToolbarSize = ImVec2(206.0f, 32.0f);
    const ImVec2 rightToolbarPos = ImVec2(displayPos.x + displaySize.x - margin - cubeSize - 8.0f - rightToolbarSize.x, displayPos.y + margin);

    drawList->AddRectFilled(rightToolbarPos, ImVec2(rightToolbarPos.x + rightToolbarSize.x, rightToolbarPos.y + rightToolbarSize.y),
        IM_COL32(24, 26, 30, 230), 6.0f);
    drawList->AddRect(rightToolbarPos, ImVec2(rightToolbarPos.x + rightToolbarSize.x, rightToolbarPos.y + rightToolbarSize.y),
        IM_COL32(50, 54, 62, 255), 6.0f, 0, 1.0f);

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

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 0.0f));

    ImGui::SetCursorScreenPos(ImVec2(leftToolbarPos.x + 4.0f, leftToolbarPos.y + 4.0f));

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

    if (ModeButton("Snap", m_EnableSnap, ImVec2(42.0f, 24.0f)))
    {
        m_EnableSnap = !m_EnableSnap;
        EditorSettings::Get().UpdateFromViewport(*this);
        EditorSettings::Get().Save();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Snapping (Ctrl)\nRight-click for Snap Settings");
    if (ImGui::BeginPopupContextItem("SnapContext"))
    {
        ImGui::OpenPopup("SnapSettingsPopup");
        ImGui::EndPopup();
    }

    ImGui::SameLine(0, 2.0f);
    if (ModeButton("##SnapArrow", false, ImVec2(16.0f, 24.0f)))
        ImGui::OpenPopup("SnapSettingsPopup");
    {
        ImVec2 pMin = ImGui::GetItemRectMin();
        ImVec2 pMax = ImGui::GetItemRectMax();
        ImVec2 center = ImVec2((pMin.x + pMax.x) * 0.5f, (pMin.y + pMax.y) * 0.5f);
        drawList->AddTriangleFilled(
            ImVec2(center.x - 3.5f, center.y - 1.5f),
            ImVec2(center.x + 3.5f, center.y - 1.5f),
            ImVec2(center.x, center.y + 2.5f),
            IM_COL32(180, 185, 195, 240)
        );
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snap Settings");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (ModeButton("View", false, ImVec2(46.0f, 24.0f)))
        ImGui::OpenPopup("ViewportViewPopup");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Viewport Actions & Selection");

    ImGui::SetCursorScreenPos(ImVec2(rightToolbarPos.x + 4.0f, rightToolbarPos.y + 4.0f));

    DebugBufferMode curDebugMode = engine->GetRenderLayer().GetDebugBufferMode();
    bool isWireframeActive = engine->GetRenderLayer().GetWireframe();
    const char* bufBtnLabel = "Display";
    float bufBtnWidth = 56.0f;

    if (isWireframeActive && curDebugMode == DebugBufferMode::None)
    {
        bufBtnLabel = "Wireframe";
        bufBtnWidth = 72.0f;
    }
    else
    {
        switch (curDebugMode)
        {
        case DebugBufferMode::Normal: bufBtnLabel = "Normal"; bufBtnWidth = 58.0f; break;
        case DebugBufferMode::Depth: bufBtnLabel = "Depth"; bufBtnWidth = 52.0f; break;
        case DebugBufferMode::Roughness: bufBtnLabel = "Roughness"; bufBtnWidth = 72.0f; break;
        case DebugBufferMode::Metallic: bufBtnLabel = "Metallic"; bufBtnWidth = 62.0f; break;
        case DebugBufferMode::SSAO: bufBtnLabel = "SSAO"; bufBtnWidth = 50.0f; break;
        case DebugBufferMode::SSGI: bufBtnLabel = "SSGI"; bufBtnWidth = 48.0f; break;
        case DebugBufferMode::SSR: bufBtnLabel = "SSR"; bufBtnWidth = 46.0f; break;
        case DebugBufferMode::DirectionalShadow: bufBtnLabel = "Shadow"; bufBtnWidth = 60.0f; break;
        case DebugBufferMode::SpotShadow: bufBtnLabel = "SpotShad"; bufBtnWidth = 66.0f; break;
        case DebugBufferMode::Bloom: bufBtnLabel = "Bloom"; bufBtnWidth = 52.0f; break;
        case DebugBufferMode::BrightPass: bufBtnLabel = "Bright"; bufBtnWidth = 54.0f; break;
        case DebugBufferMode::EntityID: bufBtnLabel = "EntityID"; bufBtnWidth = 62.0f; break;
        case DebugBufferMode::ScreenColor: bufBtnLabel = "Screen"; bufBtnWidth = 56.0f; break;
        case DebugBufferMode::ContactShadows: bufBtnLabel = "ContactShad"; bufBtnWidth = 84.0f; break;
        case DebugBufferMode::PointShadow: bufBtnLabel = "PointShad"; bufBtnWidth = 74.0f; break;
        case DebugBufferMode::MotionBlur: bufBtnLabel = "MotionBlur"; bufBtnWidth = 80.0f; break;
        default: break;
        }
    }

    bool isDebugActive = (curDebugMode != DebugBufferMode::None) || isWireframeActive;
    if (ModeButton(bufBtnLabel, isDebugActive, ImVec2(bufBtnWidth, 24.0f)))
        ImGui::OpenPopup("BufferSettingsPopup");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (ModeButton("Overlay", false, ImVec2(66.0f, 24.0f)))
        ImGui::OpenPopup("OverlaysPopup");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (ModeButton("Cam", false, ImVec2(42.0f, 24.0f)))
        ImGui::OpenPopup("CameraSettingsPopup");

    ImGui::PopStyleVar(2);

    if (ImGui::BeginPopup("BufferSettingsPopup"))
    {
        DebugBufferMode curMode = engine->GetRenderLayer().GetDebugBufferMode();
        bool isWireframe = engine->GetRenderLayer().GetWireframe();

        auto ModeItem = [&](const char* label, DebugBufferMode targetMode) {
            bool isSelected = (curMode == targetMode && !isWireframe);
            if (ImGui::Selectable(label, isSelected))
            {
                engine->GetRenderLayer().SetDebugBufferMode(targetMode);
                engine->GetRenderLayer().SetWireframe(false);
            }
        };
        
        ImGui::Separator();
        ImGui::Spacing();

        ModeItem("Default", DebugBufferMode::None);
        if (ImGui::Selectable("Wireframe", isWireframe))
        {
            bool newWireframe = !isWireframe;
            engine->GetRenderLayer().SetWireframe(newWireframe);
            if (newWireframe)
            {
                engine->GetRenderLayer().SetDebugBufferMode(DebugBufferMode::None);
            }
        }
        ImGui::Spacing();

        ImGui::TextDisabled("Geometry Buffers");
        ImGui::Separator();
        ModeItem("Normal", DebugBufferMode::Normal);
        ModeItem("Depth", DebugBufferMode::Depth);
        ModeItem("Roughness", DebugBufferMode::Roughness);
        ModeItem("Metallic", DebugBufferMode::Metallic);
        ImGui::Spacing();

        ImGui::TextDisabled("Lighting & Shadows");
        ImGui::Separator();
        ModeItem("Directional Shadow Map", DebugBufferMode::DirectionalShadow);
        if (curMode == DebugBufferMode::DirectionalShadow)
        {
            ImGui::Indent(16.0f);
            int curLayer = engine->GetRenderLayer().GetDebugBufferLayer();
            ImGui::Text("Cascade:");
            ImGui::SameLine();
            for (int c = 0; c < 4; ++c)
            {
                char cLabel[8];
                snprintf(cLabel, sizeof(cLabel), "%d", c);
                if (c > 0) ImGui::SameLine();
                if (ImGui::RadioButton(cLabel, curLayer == c))
                {
                    engine->GetRenderLayer().SetDebugBufferLayer(c);
                }
            }
            ImGui::Unindent(16.0f);
        }
        ModeItem("Point Shadow Map", DebugBufferMode::PointShadow);
        if (curMode == DebugBufferMode::PointShadow)
        {
            ImGui::Indent(16.0f);
            int curLayer = engine->GetRenderLayer().GetDebugBufferLayer();
            ImGui::Text("Light Index:");
            ImGui::SameLine();
            for (int l = 0; l < 4; ++l)
            {
                char lLabel[8];
                snprintf(lLabel, sizeof(lLabel), "%d", l);
                if (l > 0) ImGui::SameLine();
                if (ImGui::RadioButton(lLabel, curLayer == l))
                {
                    engine->GetRenderLayer().SetDebugBufferLayer(l);
                }
            }

            int curFace = engine->GetRenderLayer().GetDebugBufferFace();
            ImGui::Text("Face:");
            ImGui::SameLine();
            if (ImGui::RadioButton("All", curFace < 0))
            {
                engine->GetRenderLayer().SetDebugBufferFace(-1);
            }
            const char* faceNames[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
            for (int f = 0; f < 6; ++f)
            {
                ImGui::SameLine();
                if (ImGui::RadioButton(faceNames[f], curFace == f))
                {
                    engine->GetRenderLayer().SetDebugBufferFace(f);
                }
            }
            ImGui::Unindent(16.0f);
        }
        ModeItem("Spot Shadow Map", DebugBufferMode::SpotShadow);
        ImGui::Spacing();

        ImGui::TextDisabled("Screen Space Effects");
        ImGui::Separator();
        ModeItem("SSAO", DebugBufferMode::SSAO);
        ModeItem("Contact Shadows", DebugBufferMode::ContactShadows);
        ModeItem("SSGI", DebugBufferMode::SSGI);
        ModeItem("SSR", DebugBufferMode::SSR);
        ImGui::Spacing();

        ImGui::TextDisabled("Post Processing");
        ImGui::Separator();
        ModeItem("Bloom Buffer", DebugBufferMode::Bloom);
        ModeItem("Bright Pass", DebugBufferMode::BrightPass);
        ModeItem("Motion Blur Buffer", DebugBufferMode::MotionBlur);
        ModeItem("Screen Color", DebugBufferMode::ScreenColor);
        ModeItem("Entity ID Buffer", DebugBufferMode::EntityID);

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("SnapSettingsPopup"))
    {
        bool snapChanged = false;
        ImGui::TextDisabled("Snapping Values");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(90);
        if (ImGui::DragFloat("Translate (m)", &m_snapTranslate, 0.05f, 0.01f, 100.0f, "%.2f")) snapChanged = true;

        ImGui::SetNextItemWidth(90);
        if (ImGui::DragFloat("Translate UI (px)", &m_snapTranslateUI, 1.0f, 1.0f, 500.0f, "%.0f")) snapChanged = true;

        ImGui::SetNextItemWidth(90);
        if (ImGui::DragFloat("Rotate (deg)", &m_snapRotate, 1.0f, 0.1f, 180.0f, "%.1f")) snapChanged = true;

        ImGui::SetNextItemWidth(90);
        if (ImGui::DragFloat("Scale", &m_snapScale, 0.05f, 0.01f, 10.0f, "%.2f")) snapChanged = true;

        if (snapChanged)
        {
            EditorSettings::Get().UpdateFromViewport(*this);
            EditorSettings::Get().Save();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("ViewportViewPopup"))
    {
        bool viewChanged = false;
        const bool hasTransformSelection = EditorUtils::HasTransformSelection(*engine);

        ImGui::TextDisabled("View Actions");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

        const float btnWidth = 270.0f;
        const float btnHeight = 30.0f;

        auto DrawActionButton = [&](const char* label, const char* shortcut, bool enabled, auto onClick) {
            if (!enabled)
                ImGui::BeginDisabled();

            ImVec2 p0 = ImGui::GetCursorScreenPos();
            if (ImGui::Button(label, ImVec2(btnWidth, btnHeight)))
            {
                onClick();
                ImGui::CloseCurrentPopup();
            }

            ImVec2 textSize = ImGui::CalcTextSize(shortcut);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 shortcutPos = ImVec2(p0.x + btnWidth - textSize.x - 12.0f, p0.y + (btnHeight - textSize.y) * 0.5f);
            dl->AddText(shortcutPos, IM_COL32(160, 165, 175, enabled ? 230 : 110), shortcut);

            if (!enabled)
                ImGui::EndDisabled();
        };

        DrawActionButton("Align Transform with View", "Ctrl+Alt+M", hasTransformSelection, [&]() {
            EditorUtils::AlignSelectedWithView(*engine);
        });
        DrawActionButton("Snap to Floor", "PageDown", hasTransformSelection, [&]() {
            EditorUtils::SnapSelectedToFloor(*engine, false);
        });

        ImGui::PopStyleVar(3);

        ImGui::Spacing();
        ImGui::TextDisabled("Selection");
        ImGui::Separator();
        if (ImGui::Checkbox("Select Root Entity", &m_SelectRootEntity)) viewChanged = true;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("When clicking an entity in the viewport, automatically select the root entity of the tree.");

        if (viewChanged)
        {
            EditorSettings::Get().UpdateFromViewport(*this);
            EditorSettings::Get().Save();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("OverlaysPopup"))
    {
        bool overlaysChanged = false;

        ImGui::TextDisabled("Visual Overlays");
        ImGui::Separator();
        bool drawGizmos = m_DrawGizmos;
        if (ImGui::Checkbox("Draw Gizmos", &drawGizmos))
        {
            m_DrawGizmos = drawGizmos;
            GizmoPass::s_Enabled = drawGizmos;
            overlaysChanged = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Gizmos (G)");

        auto& debugSettings = DebugRenderer::Get().GetSettings();
        if (ImGui::Checkbox("Infinite Grid", &debugSettings.drawInfiniteGrid)) overlaysChanged = true;
        if (ImGui::Checkbox("Draw Colliders", &debugSettings.drawColliders)) overlaysChanged = true;
        if (ImGui::Checkbox("Draw Bounding Boxes", &debugSettings.drawBoundingBoxes)) overlaysChanged = true;
        if (ImGui::Checkbox("Draw LOD Levels", &debugSettings.drawLODLevels)) overlaysChanged = true;

        ImGui::Spacing();
        ImGui::TextDisabled("Statistics");
        ImGui::Separator();
        if (ImGui::Checkbox("Show Statistics", &m_ShowStatistics)) overlaysChanged = true;

        if (overlaysChanged)
        {
            EditorSettings::Get().UpdateFromViewport(*this);
            EditorSettings::Get().Save();
        }

        ImGui::EndPopup();
    }

    EditorCamera* cam = engine->GetEditorCamera();
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
}

void Viewport::DrawOverlayStats(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    bool isCtrlDown = ImGui::GetIO().KeyCtrl || ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool isSnappingActive = m_EnableSnap ? !isCtrlDown : isCtrlDown;
    if (isSnappingActive)
    {
        entt::entity selectedEntity = EditorSelection::Get().GetPrimary();
        bool isUI = selectedEntity != entt::null &&
                    engine->GetActiveScene().GetRegistry().valid(selectedEntity) &&
                    engine->GetActiveScene().GetRegistry().any_of<RectTransformComponent>(selectedEntity);

        const char* label = "";
        float val = 0.0f;
        if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE)
        {
            if (isUI) { label = "Snap: %.0fpx"; val = m_snapTranslateUI; }
            else { label = "Snap: %.2fm"; val = m_snapTranslate; }
        }
        else if (m_CurrentGizmoOperation == ImGuizmo::ROTATE) { label = "Snap: %.1f°"; val = m_snapRotate; }
        else if (m_CurrentGizmoOperation == ImGuizmo::SCALE) { label = "Snap: %.2f";  val = m_snapScale; }

        char buf[64];
        snprintf(buf, sizeof(buf), label, val);
        float snapY = (engine->GetRenderLayer().GetDebugBufferMode() != DebugBufferMode::None) ? 78.0f : 48.0f;
        ImVec2 textPos = ImVec2(displayPos.x + 12.0f, displayPos.y + snapY);
        ImVec2 snapTextSize = ImGui::CalcTextSize(buf);
        drawList->AddRectFilled(ImVec2(textPos.x - 4, textPos.y - 2), ImVec2(textPos.x + snapTextSize.x + 6, textPos.y + snapTextSize.y + 4), IM_COL32(18, 20, 24, 200), 4.0f);
        drawList->AddText(textPos, IM_COL32(245, 195, 65, 255), buf);
    }

    if (m_ShowStatistics)
    {
        float framerate = ImGui::GetIO().Framerate;
        float frameTimeMs = (framerate > 0.0f) ? (1000.0f / framerate) : 0.0f;

        char fpsBuf[64];
        snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS (%.2f ms)", framerate, frameTimeMs);

        ImU32 fpsColor = IM_COL32(90, 220, 120, 255);
        if (framerate < 30.0f)
            fpsColor = IM_COL32(240, 80, 80, 255);
        else if (framerate < 50.0f)
            fpsColor = IM_COL32(245, 200, 65, 255);

        auto& ctx = engine->GetRenderLayer().GetRenderContext();
        uint32_t resW = ctx.viewportWidth > 0 ? ctx.viewportWidth : static_cast<uint32_t>(displaySize.x);
        uint32_t resH = ctx.viewportHeight > 0 ? ctx.viewportHeight : static_cast<uint32_t>(displaySize.y);
        char resBuf[64];
        snprintf(resBuf, sizeof(resBuf), "%u x %u", resW, resH);

        const auto& stats = engine->GetRenderLayer().GetRenderStats();

        char drawCallBuf[32];
        snprintf(drawCallBuf, sizeof(drawCallBuf), "%u", stats.drawCalls);

        auto FormatStatNum = [](uint32_t val) -> std::string {
            std::string s = std::to_string(val);
            int n = static_cast<int>(s.length()) - 3;
            while (n > 0)
            {
                s.insert(n, ",");
                n -= 3;
            }
            return s;
        };

        std::string triStr = FormatStatNum(stats.triangles);
        std::string vertStr = FormatStatNum(stats.vertices);

        auto& scene = engine->GetActiveScene();
        auto& reg = scene.GetRegistry();
        uint32_t entityCount = static_cast<uint32_t>(reg.storage<entt::entity>().free_list());
        if (entityCount > 0 && reg.valid(scene.GetRootEntity()))
            entityCount--;
        char entBuf[32];
        snprintf(entBuf, sizeof(entBuf), "%u", entityCount);

        uint32_t totalMeshes = stats.meshCount + stats.skeletalMeshCount;
        char meshBuf[64];
        if (stats.skeletalMeshCount > 0)
            snprintf(meshBuf, sizeof(meshBuf), "%u (%u skel)", totalMeshes, stats.skeletalMeshCount);
        else
            snprintf(meshBuf, sizeof(meshBuf), "%u", totalMeshes);

        uint32_t dirLights = ctx.directionalLight.has_value() ? 1 : 0;
        uint32_t pointLights = static_cast<uint32_t>(ctx.pointLights.size());
        uint32_t spotLights = static_cast<uint32_t>(ctx.spotLights.size());
        uint32_t totalLights = dirLights + pointLights + spotLights;
        char lightBuf[64];
        if (totalLights > 0)
            snprintf(lightBuf, sizeof(lightBuf), "%u (%uD/%uP/%uS)", totalLights, dirLights, pointLights, spotLights);
        else
            snprintf(lightBuf, sizeof(lightBuf), "0");

        PhysicsStats physStats = scene.GetPhysicsSystem().GetStats();

        char bodiesBuf[64];
        bool isPlayState = (engine->GetActiveScene().GetState() == SceneState::PLAY);
        if (isPlayState)
        {
            snprintf(bodiesBuf, sizeof(bodiesBuf), "%u (%u act)", 
                physStats.totalBodies + physStats.characterBodies, 
                physStats.activeBodies);
        }
        else
        {
            if (physStats.characterBodies > 0)
            {
                snprintf(bodiesBuf, sizeof(bodiesBuf), "%u (%uD/%uS/%uC)", 
                    physStats.totalBodies + physStats.characterBodies,
                    physStats.dynamicBodies, 
                    physStats.staticBodies,
                    physStats.characterBodies);
            }
            else
            {
                snprintf(bodiesBuf, sizeof(bodiesBuf), "%u (%uD/%uS)", 
                    physStats.totalBodies, 
                    physStats.dynamicBodies, 
                    physStats.staticBodies);
            }
        }

        char collidersBuf[32];
        snprintf(collidersBuf, sizeof(collidersBuf), "%u", physStats.totalColliders);

        char physStepBuf[32];
        snprintf(physStepBuf, sizeof(physStepBuf), "%.2f ms", physStats.stepTimeMs);

        struct StatRow
        {
            const char* label;
            std::string value;
            ImU32 valueColor;
        };

        std::vector<StatRow> rows = {
            { "FPS", fpsBuf, fpsColor },
            { "Resolution", resBuf, IM_COL32(225, 230, 240, 255) },
            { "Draw Calls", drawCallBuf, IM_COL32(225, 230, 240, 255) },
            { "Triangles", triStr, IM_COL32(225, 230, 240, 255) },
            { "Vertices", vertStr, IM_COL32(225, 230, 240, 255) },
            { "Entities", entBuf, IM_COL32(225, 230, 240, 255) },
            { "Meshes", meshBuf, IM_COL32(225, 230, 240, 255) },
            { "Lights", lightBuf, IM_COL32(225, 230, 240, 255) },
            { "Bodies", bodiesBuf, IM_COL32(225, 230, 240, 255) },
            { "Colliders", collidersBuf, IM_COL32(225, 230, 240, 255) }
        };

        if (isPlayState)
        {
            ImU32 stepColor = IM_COL32(90, 220, 120, 255);
            if (physStats.stepTimeMs > 8.0f)
                stepColor = IM_COL32(240, 80, 80, 255);
            else if (physStats.stepTimeMs > 4.0f)
                stepColor = IM_COL32(245, 200, 65, 255);

            rows.push_back({ "Phys Step", physStepBuf, stepColor });
        }

        const float padX = 10.0f;
        const float padY = 8.0f;
        const float rowHeight = 17.0f;
        const float headerHeight = 22.0f;
        const size_t numRows = rows.size();

        float maxLabelW = 0.0f;
        float maxValW = 0.0f;
        for (const auto& r : rows)
        {
            maxLabelW = std::max(maxLabelW, ImGui::CalcTextSize(r.label).x);
            maxValW = std::max(maxValW, ImGui::CalcTextSize(r.value.c_str()).x);
        }

        float panelWidth = std::max(220.0f, maxLabelW + maxValW + padX * 3.0f + 16.0f);
        float panelHeight = headerHeight + padY * 2.0f + static_cast<float>(numRows) * rowHeight;

        bool isEditState = (engine->GetActiveScene().GetState() == SceneState::EDIT);
        float startY = isEditState ? 115.0f : 14.0f;
        ImVec2 panelPos = ImVec2(displayPos.x + displaySize.x - panelWidth - 14.0f, displayPos.y + startY);

        drawList->AddRectFilled(panelPos, ImVec2(panelPos.x + panelWidth, panelPos.y + panelHeight), IM_COL32(18, 20, 24, 215), 5.0f);
        drawList->AddRect(panelPos, ImVec2(panelPos.x + panelWidth, panelPos.y + panelHeight), IM_COL32(50, 54, 62, 200), 5.0f, 0, 1.0f);

        drawList->AddText(ImVec2(panelPos.x + padX, panelPos.y + 6.0f), IM_COL32(245, 195, 65, 255), "STATISTICS");

        float sepY = panelPos.y + headerHeight + 2.0f;
        drawList->AddLine(ImVec2(panelPos.x + 8.0f, sepY), ImVec2(panelPos.x + panelWidth - 8.0f, sepY), IM_COL32(55, 60, 70, 180), 1.0f);

        float curY = sepY + 5.0f;
        for (const auto& r : rows)
        {
            drawList->AddText(ImVec2(panelPos.x + padX, curY), IM_COL32(150, 155, 165, 255), r.label);

            ImVec2 valSize = ImGui::CalcTextSize(r.value.c_str());
            float valX = panelPos.x + panelWidth - padX - valSize.x;
            drawList->AddText(ImVec2(valX, curY), r.valueColor, r.value.c_str());

            curY += rowHeight;
        }
    }
}

void Viewport::DrawDebugBufferBadge(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    DebugBufferMode mode = engine->GetRenderLayer().GetDebugBufferMode();
    if (mode == DebugBufferMode::None)
        return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const char* modeName = "";
    switch (mode)
    {
    case DebugBufferMode::Normal: modeName = "Normal"; break;
    case DebugBufferMode::Depth: modeName = "Depth"; break;
    case DebugBufferMode::Roughness: modeName = "Roughness"; break;
    case DebugBufferMode::Metallic: modeName = "Metallic"; break;
    case DebugBufferMode::SSAO: modeName = "SSAO"; break;
    case DebugBufferMode::SSGI: modeName = "SSGI"; break;
    case DebugBufferMode::SSR: modeName = "SSR"; break;
    case DebugBufferMode::DirectionalShadow: modeName = "Directional Shadow Map"; break;
    case DebugBufferMode::SpotShadow: modeName = "Spot Shadow Map"; break;
    case DebugBufferMode::Bloom: modeName = "Bloom"; break;
    case DebugBufferMode::BrightPass: modeName = "Bright Pass"; break;
    case DebugBufferMode::EntityID: modeName = "Entity ID Buffer"; break;
    case DebugBufferMode::ScreenColor: modeName = "Screen Color"; break;
    case DebugBufferMode::ContactShadows: modeName = "Contact Shadows"; break;
    case DebugBufferMode::PointShadow: modeName = "Point Shadow Map"; break;
    case DebugBufferMode::MotionBlur: modeName = "Motion Blur Buffer"; break;
    default: break;
    }

    bool isEffectDisabled = false;
    const char* disableWarning = nullptr;
    auto& env = engine->GetActiveScene().GetEnvironment();
    if (mode == DebugBufferMode::SSAO && !env.SSAO)
    {
        isEffectDisabled = true;
        disableWarning = " [SSAO is OFF in Environment]";
    }
    else if (mode == DebugBufferMode::MotionBlur && !env.MotionBlur)
    {
        isEffectDisabled = true;
        disableWarning = " [Motion Blur is OFF in Environment]";
    }
    else if (mode == DebugBufferMode::ContactShadows && !env.ContactShadows)
    {
        isEffectDisabled = true;
        disableWarning = " [Contact Shadows is OFF in Environment]";
    }
    else if (mode == DebugBufferMode::SSGI && !env.SSGI)
    {
        isEffectDisabled = true;
        disableWarning = " [SSGI is OFF in Environment]";
    }

    char titleBuf[160];
    if (isEffectDisabled && disableWarning)
        snprintf(titleBuf, sizeof(titleBuf), "Debug View: %s%s", modeName, disableWarning);
    else if (mode == DebugBufferMode::DirectionalShadow)
        snprintf(titleBuf, sizeof(titleBuf), "Debug View: %s (Cascade %d)", modeName, engine->GetRenderLayer().GetDebugBufferLayer());
    else if (mode == DebugBufferMode::PointShadow)
    {
        int curFace = engine->GetRenderLayer().GetDebugBufferFace();
        const char* faceNames[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
        if (curFace >= 0 && curFace < 6)
            snprintf(titleBuf, sizeof(titleBuf), "Debug View: %s (Light %d, Face %s)", modeName, engine->GetRenderLayer().GetDebugBufferLayer(), faceNames[curFace]);
        else
            snprintf(titleBuf, sizeof(titleBuf), "Debug View: %s (Light %d, All Faces)", modeName, engine->GetRenderLayer().GetDebugBufferLayer());
    }
    else
        snprintf(titleBuf, sizeof(titleBuf), "Debug View: %s", modeName);

    ImVec2 badgePos = ImVec2(displayPos.x + 12.0f, displayPos.y + 48.0f);
    ImVec2 textSize = ImGui::CalcTextSize(titleBuf);
    float badgeW = textSize.x + 64.0f;
    float badgeH = 26.0f;

    ImU32 bgCol = isEffectDisabled ? IM_COL32(55, 25, 20, 230) : IM_COL32(20, 32, 50, 230);
    ImU32 borderCol = isEffectDisabled ? IM_COL32(230, 80, 50, 255) : IM_COL32(50, 140, 230, 255);

    drawList->AddRectFilled(badgePos, ImVec2(badgePos.x + badgeW, badgePos.y + badgeH), bgCol, 5.0f);
    drawList->AddRect(badgePos, ImVec2(badgePos.x + badgeW, badgePos.y + badgeH), borderCol, 5.0f, 0, 1.5f);

    ImVec2 textPos = ImVec2(badgePos.x + 8.0f, badgePos.y + (badgeH - textSize.y) * 0.5f);
    ImU32 textCol = isEffectDisabled ? IM_COL32(255, 130, 100, 255) : IM_COL32(110, 200, 255, 255);
    drawList->AddText(textPos, textCol, titleBuf);

    ImGui::SetCursorScreenPos(ImVec2(badgePos.x + badgeW - 52.0f, badgePos.y + 2.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.12f, 0.12f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    if (ImGui::Button("Reset##DebugReset", ImVec2(48.0f, 22.0f)))
    {
        engine->GetRenderLayer().SetDebugBufferMode(DebugBufferMode::None);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
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

entt::entity Viewport::ResolveRootEntity(Engine* engine, entt::entity entity) const
{
    if (!m_SelectRootEntity || entity == entt::null)
        return entity;

    auto& scene = engine->GetActiveScene();
    entt::entity current = entity;
    entt::entity rootCandidate = current;

    while (current != entt::null && current != scene.GetRootEntity())
    {
        if (scene.HasComponent<PrefabComponent>(current))
        {
            rootCandidate = current;
            break;
        }

        entt::entity parent = scene.GetParent(current);
        if (parent == entt::null || parent == scene.GetRootEntity())
        {
            rootCandidate = current;
            break;
        }
        current = parent;
        rootCandidate = current;
    }
    return rootCandidate;
}

void Viewport::HandleSelection(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    if (engine->GetActiveScene().GetState() != SceneState::EDIT)
    {
        m_SelectionBoxPotential = false;
        m_SelectionBoxActive = false;
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
    {
        m_SelectionBoxPotential = false;
        m_SelectionBoxActive = false;
    }

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool isMouseInViewport = mousePos.x >= displayPos.x && mousePos.x <= (displayPos.x + displaySize.x) &&
        mousePos.y >= displayPos.y && mousePos.y <= (displayPos.y + displaySize.y);

    const float gizmoSize = 96.0f;
    const float margin = 10.0f;
    bool isOverOrientationGizmo = mousePos.x >= (displayPos.x + displaySize.x - gizmoSize - margin) &&
        mousePos.x <= (displayPos.x + displaySize.x - margin) &&
        mousePos.y >= (displayPos.y + margin) &&
        mousePos.y <= (displayPos.y + margin + gizmoSize);

    const float leftToolbarWidth = 308.0f;
    const float leftToolbarHeight = 32.0f;
    bool isOverLeftToolbar = mousePos.x >= (displayPos.x + 10.0f) &&
        mousePos.x <= (displayPos.x + 10.0f + leftToolbarWidth) &&
        mousePos.y >= (displayPos.y + 10.0f) &&
        mousePos.y <= (displayPos.y + 10.0f + leftToolbarHeight);

    const float rightToolbarWidth = 206.0f;
    const float rightToolbarHeight = 32.0f;
    float rightToolbarX = displayPos.x + displaySize.x - margin - gizmoSize - 8.0f - rightToolbarWidth;
    bool isOverRightToolbar = mousePos.x >= rightToolbarX &&
        mousePos.x <= (rightToolbarX + rightToolbarWidth) &&
        mousePos.y >= (displayPos.y + margin) &&
        mousePos.y <= (displayPos.y + margin + rightToolbarHeight);

    bool isOverStats = false;
    if (m_ShowStatistics)
    {
        const float statsW = 240.0f;
        const float statsH = 260.0f;
        float statsY = (engine->GetActiveScene().GetState() == SceneState::EDIT) ? 115.0f : 14.0f;
        isOverStats = mousePos.x >= (displayPos.x + displaySize.x - statsW - 14.0f) &&
            mousePos.x <= (displayPos.x + displaySize.x - 14.0f) &&
            mousePos.y >= (displayPos.y + statsY) &&
            mousePos.y <= (displayPos.y + statsY + statsH);
    }

    bool isBlockedByUI = isOverOrientationGizmo || isOverLeftToolbar || isOverRightToolbar || isOverStats;

    if (isMouseInViewport && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !isBlockedByUI &&
        !ImGui::IsPopupOpen((const char*)nullptr, ImGuiPopupFlags_AnyPopupId))
    {
        m_SelectionBoxPotential = true;
        m_SelectionBoxActive = false;
        m_SelectionBoxStart = mousePos;
    }

    if (m_SelectionBoxPotential)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            float dragDist = std::hypot(mousePos.x - m_SelectionBoxStart.x, mousePos.y - m_SelectionBoxStart.y);
            if (!m_SelectionBoxActive && dragDist > 6.0f)
            {
                m_SelectionBoxActive = true;
            }

            if (m_SelectionBoxActive)
            {
                ImVec2 boxMin = ImVec2(
                    std::clamp(std::min(m_SelectionBoxStart.x, mousePos.x), displayPos.x, displayPos.x + displaySize.x),
                    std::clamp(std::min(m_SelectionBoxStart.y, mousePos.y), displayPos.y, displayPos.y + displaySize.y)
                );
                ImVec2 boxMax = ImVec2(
                    std::clamp(std::max(m_SelectionBoxStart.x, mousePos.x), displayPos.x, displayPos.x + displaySize.x),
                    std::clamp(std::max(m_SelectionBoxStart.y, mousePos.y), displayPos.y, displayPos.y + displaySize.y)
                );

                ImVec4 acc = GetAccentColor();
                ImU32 fillCol = ImGui::ColorConvertFloat4ToU32(ImVec4(acc.x, acc.y, acc.z, 0.18f));
                ImU32 borderCol = ImGui::ColorConvertFloat4ToU32(ImVec4(acc.x, acc.y, acc.z, 0.85f));

                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(boxMin, boxMax, fillCol, 0.0f);
                drawList->AddRect(boxMin, boxMax, borderCol, 0.0f, 0, 1.5f);
            }
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            ImGuiIO& io = ImGui::GetIO();
            bool isMultiModifier = io.KeyCtrl || io.KeyShift;

            auto resolveRootEntity = [&](entt::entity entity) -> entt::entity {
                return ResolveRootEntity(engine, entity);
            };

            if (m_SelectionBoxActive)
            {
                ImVec2 boxMin = ImVec2(
                    std::clamp(std::min(m_SelectionBoxStart.x, mousePos.x), displayPos.x, displayPos.x + displaySize.x),
                    std::clamp(std::min(m_SelectionBoxStart.y, mousePos.y), displayPos.y, displayPos.y + displaySize.y)
                );
                ImVec2 boxMax = ImVec2(
                    std::clamp(std::max(m_SelectionBoxStart.x, mousePos.x), displayPos.x, displayPos.x + displaySize.x),
                    std::clamp(std::max(m_SelectionBoxStart.y, mousePos.y), displayPos.y, displayPos.y + displaySize.y)
                );

                auto& context = engine->GetRenderLayer().GetRenderContext();
                float fboWidth = context.viewportWidth;
                float fboHeight = context.viewportHeight;

                float normMinX = std::clamp((boxMin.x - displayPos.x) / displaySize.x, 0.0f, 1.0f);
                float normMaxX = std::clamp((boxMax.x - displayPos.x) / displaySize.x, 0.0f, 1.0f);
                float normMinY = std::clamp((boxMin.y - displayPos.y) / displaySize.y, 0.0f, 1.0f);
                float normMaxY = std::clamp((boxMax.y - displayPos.y) / displaySize.y, 0.0f, 1.0f);

                uint32_t fboMinX = static_cast<uint32_t>(normMinX * (fboWidth - 1));
                uint32_t fboMaxX = static_cast<uint32_t>(normMaxX * (fboWidth - 1));
                uint32_t fboMinY = static_cast<uint32_t>((1.0f - normMaxY) * (fboHeight - 1));
                uint32_t fboMaxY = static_cast<uint32_t>((1.0f - normMinY) * (fboHeight - 1));

                uint32_t rectW = (fboMaxX >= fboMinX) ? (fboMaxX - fboMinX + 1) : 1;
                uint32_t rectH = (fboMaxY >= fboMinY) ? (fboMaxY - fboMinY + 1) : 1;

                auto pickedIDs = engine->Selection.PickRect(fboMinX, fboMinY, rectW, rectH);

                std::unordered_set<entt::entity> resolvedSet;
                auto& scene = engine->GetActiveScene();
                auto& reg = scene.GetRegistry();

                for (uint32_t id : pickedIDs)
                {
                    entt::entity e = static_cast<entt::entity>(id);
                    if (reg.valid(e))
                    {
                        resolvedSet.insert(resolveRootEntity(e));
                    }
                }

                if (auto* cam = engine->GetEditorCamera())
                {
                    glm::mat4 viewProj = cam->GetProjectionMatrix() * cam->GetViewMatrix();
                    auto view = reg.view<TransformComponent>();
                    for (auto e : view)
                    {
                        if (e == scene.GetRootEntity()) continue;
                        auto& tc = view.get<TransformComponent>(e);
                        glm::vec4 clip = viewProj * glm::vec4(tc.GetWorldPosition(), 1.0f);
                        if (clip.w > 0.0001f)
                        {
                            glm::vec3 ndc = glm::vec3(clip) / clip.w;
                            if (ndc.z >= -1.0f && ndc.z <= 1.0f)
                            {
                                float screenX = displayPos.x + (ndc.x * 0.5f + 0.5f) * displaySize.x;
                                float screenY = displayPos.y + ((1.0f - (ndc.y * 0.5f + 0.5f))) * displaySize.y;
                                if (screenX >= boxMin.x && screenX <= boxMax.x && screenY >= boxMin.y && screenY <= boxMax.y)
                                {
                                    resolvedSet.insert(resolveRootEntity(e));
                                }
                            }
                        }
                    }
                }

                std::vector<entt::entity> entitiesList(resolvedSet.begin(), resolvedSet.end());

                if (isMultiModifier)
                {
                    for (auto e : entitiesList)
                        EditorSelection::Get().Add(e);
                }
                else
                {
                    if (entitiesList.empty())
                        EditorSelection::Get().Clear();
                    else
                        EditorSelection::Get().SetSelection(entitiesList, entitiesList.front());
                }
            }
            else
            {
                glm::vec2 mouseViewportPos = { mousePos.x - displayPos.x, mousePos.y - displayPos.y };

                auto& context = engine->GetRenderLayer().GetRenderContext();
                float fboWidth = context.viewportWidth;
                float fboHeight = context.viewportHeight;

                float normX = std::clamp(mouseViewportPos.x / displaySize.x, 0.0f, 1.0f);
                float normY = std::clamp(mouseViewportPos.y / displaySize.y, 0.0f, 1.0f);

                uint32_t fboX = static_cast<uint32_t>(normX * (fboWidth - 1));
                uint32_t fboY = static_cast<uint32_t>((1.0f - normY) * (fboHeight - 1));

                uint32_t pickedID = engine->Selection.Pick(fboX, fboY);

                if (pickedID != 0)
                {
                    entt::entity entity = static_cast<entt::entity>(pickedID);
                    entity = resolveRootEntity(entity);

                    if (isMultiModifier)
                    {
                        EditorSelection::Get().Toggle(entity);
                    }
                    else
                    {
                        EditorSelection::Get().Select(entity);
                    }
                }
                else
                {
                    if (!isMultiModifier)
                    {
                        EditorSelection::Get().Clear();
                    }
                }
            }

            m_SelectionBoxPotential = false;
            m_SelectionBoxActive = false;
        }
    }
}

void Viewport::Draw(Engine* engine, bool orbitDragActive)
{
    if (!m_IsOpen)
        return;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        m_PanelPosition = ImGui::GetWindowPos();
        m_PanelSize = ImGui::GetWindowSize();
        m_SelectRootEntity = EditorSettings::Get().ViewportSettings.SelectRootEntity;
        const bool isPlaying = engine->GetActiveScene().GetState() == SceneState::PLAY;
        const bool viewportHasFocus = ImGui::IsWindowFocused();
        const bool capturedGameplayKeepsFocus = Input::IsGameplayInputEnabled() && Input::IsMouseCaptured();
        Input::SetGameplayInputEnabled(isPlaying && (viewportHasFocus || capturedGameplayKeepsFocus));

        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        viewportSize.x = std::max(viewportSize.x, 1.0f);
        viewportSize.y = std::max(viewportSize.y, 1.0f);

        StretchMode stretchMode = StretchMode::Viewport;
        StretchAspect stretchAspect = StretchAspect::Keep;
        uint32_t vpW = 1920;
        uint32_t vpH = 1080;

        if (ProjectManager::IsProjectLoaded())
        {
            const auto& settings = ProjectManager::GetActiveProject()->GetSettings();
            stretchMode = settings.stretchMode;
            stretchAspect = settings.stretchAspect;
            vpW = settings.viewportWidth > 0 ? settings.viewportWidth : 1920;
            vpH = settings.viewportHeight > 0 ? settings.viewportHeight : 1080;
        }

        uint32_t targetRenderW = vpW;
        uint32_t targetRenderH = vpH;

        if (stretchMode == StretchMode::Disabled || stretchMode == StretchMode::CanvasItems)
        {
            targetRenderW = static_cast<uint32_t>(viewportSize.x);
            targetRenderH = static_cast<uint32_t>(viewportSize.y);
        }
        else //Viewport
        {
            if (stretchAspect == StretchAspect::Expand)
            {
                float winAspect = viewportSize.x / viewportSize.y;
                float vpAspect = static_cast<float>(vpW) / static_cast<float>(vpH);
                if (winAspect > vpAspect)
                {
                    targetRenderW = static_cast<uint32_t>(std::max(1.0f, std::round(static_cast<float>(vpH) * winAspect)));
                    targetRenderH = vpH;
                }
                else
                {
                    targetRenderW = vpW;
                    targetRenderH = static_cast<uint32_t>(std::max(1.0f, std::round(static_cast<float>(vpW) / winAspect)));
                }
            }
            else if (stretchAspect == StretchAspect::KeepWidth)
            {
                targetRenderW = vpW;
                float aspect = viewportSize.x / viewportSize.y;
                targetRenderH = static_cast<uint32_t>(std::max(1.0f, std::round(static_cast<float>(vpW) / aspect)));
            }
            else if (stretchAspect == StretchAspect::KeepHeight)
            {
                targetRenderH = vpH;
                float aspect = viewportSize.x / viewportSize.y;
                targetRenderW = static_cast<uint32_t>(std::max(1.0f, std::round(static_cast<float>(vpH) * aspect)));
            }
            else
            {
                targetRenderW = vpW;
                targetRenderH = vpH;
            }
        }

        engine->GetRenderLayer().SetTargetResolution(targetRenderW, targetRenderH);

        const float textureWidth = static_cast<float>(targetRenderW);
        const float textureHeight = static_cast<float>(targetRenderH);
        const float textureAspect = textureWidth / textureHeight;

        ImVec2 displaySize = viewportSize;
        if (stretchMode == StretchMode::Disabled)
        {
            displaySize = ImVec2(textureWidth, textureHeight);
        }
        else if (stretchMode == StretchMode::Viewport)
        {
            if (stretchAspect == StretchAspect::Ignore || stretchAspect == StretchAspect::Expand)
            {
                displaySize = viewportSize;
            }
            else
            {
                if (viewportSize.x / viewportSize.y > textureAspect) {
                    displaySize.y = viewportSize.y;
                    displaySize.x = viewportSize.y * textureAspect;
                }
                else {
                    displaySize.x = viewportSize.x;
                    displaySize.y = viewportSize.x / textureAspect;
                }
            }
        }
        else // CanvasItems
        {
            displaySize = viewportSize;
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
            Input::SetViewportMousePos(glm::vec2(normX * textureWidth, normY * textureHeight));
        }

        const bool altHeld = ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);
        const bool suppressViewportEditing = altHeld || orbitDragActive;
        if (suppressViewportEditing)
        {
            m_WasGizmoUsing = false;
            m_WasUIGizmo = false;
            m_InitialTransforms.clear();
            m_InitialRectTransforms.clear();
            m_SelectionBoxPotential = false;
            m_SelectionBoxActive = false;
        }
        else
        {
            if (engine->GetActiveScene().GetState() == SceneState::EDIT)
            {
                if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered())
                {
                    if (!ImGui::GetIO().WantTextInput && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGuizmo::IsUsing())
                    {
                        if (ImGui::IsKeyPressed(ImGuiKey_T))
                        {
                            m_CurrentGizmoMode = (m_CurrentGizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
                            EditorSettings::Get().UpdateFromViewport(*this);
                            EditorSettings::Get().Save();
                        }
                        if (ImGui::IsKeyPressed(ImGuiKey_Q)) m_CurrentGizmoOperation = static_cast<ImGuizmo::OPERATION>(-1);
                        if (ImGui::IsKeyPressed(ImGuiKey_W)) m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
                        if (ImGui::IsKeyPressed(ImGuiKey_E)) m_CurrentGizmoOperation = ImGuizmo::ROTATE;
                        if (ImGui::IsKeyPressed(ImGuiKey_R)) m_CurrentGizmoOperation = ImGuizmo::SCALE;
                        if (ImGui::IsKeyPressed(ImGuiKey_G))
                        {
                            m_DrawGizmos = !m_DrawGizmos;
                            GizmoPass::s_Enabled = m_DrawGizmos;
                            EditorSettings::Get().UpdateFromViewport(*this);
                            EditorSettings::Get().Save();
                        }
                    }
                }
            }
            DrawGizmos(engine, displayPos, displaySize);
        }
        if(engine->GetActiveScene().GetState() == SceneState::EDIT)
        {
            DrawToolbar(engine, displayPos, displaySize);
            DrawOrientationGizmo(engine, displayPos, displaySize);
            DrawPrefabBanner(engine, displayPos, displaySize);
        }

        if (!suppressViewportEditing)
            HandleSelection(engine, displayPos, displaySize);
        DrawOverlayStats(engine, displayPos, displaySize);
        DrawDebugBufferBadge(engine, displayPos, displaySize);

        if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->Rect(), ImGui::GetID("ViewportDropTarget")))
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH", ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
                if (payload->Data && payload->DataSize > 0)
                {
                    std::string pathStr((const char*)payload->Data);

                    bool isScript = pathStr.ends_with(".lua");
                    bool isMaterial = pathStr.ends_with(".rmat") || pathStr.ends_with(".Material");
                    if (!isMaterial && pathStr.find('#') != std::string::npos)
                    {
                        AssetUUID subUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
                        if (subUUID.IsValid())
                        {
                            auto hashPos = pathStr.find('#');
                            auto basePath = pathStr.substr(0, hashPos);
                            auto metaPath = AssetMeta::GetMetaPath(basePath);
                            AssetMeta meta;
                            if (meta.LoadFromFile(metaPath))
                            {
                                for (const auto& sub : meta.subAssets)
                                {
                                    if (sub.uuid == subUUID && sub.type == "Material")
                                    {
                                        isMaterial = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    bool isPrefab = false;
                    std::string prefabPath = pathStr;
                    if (!isScript && !isMaterial)
                    {
                        if (!prefabPath.ends_with(".rprefab"))
                        {
                            std::filesystem::path p(prefabPath);
                            auto candidate = p;
                            candidate.replace_extension(".rprefab");
                            if (std::filesystem::exists(candidate))
                                prefabPath = candidate.string();
                        }
                        if (prefabPath.ends_with(".rprefab"))
                        {
                            isPrefab = true;
                        }
                    }

                    ImVec2 mousePos = ImGui::GetIO().MousePos;
                    entt::entity hoveredEntity = entt::null;
                    if (displaySize.x > 0.0f && displaySize.y > 0.0f)
                    {
                        glm::vec2 mouseViewportPos = { mousePos.x - displayPos.x, mousePos.y - displayPos.y };
                        if (mouseViewportPos.x >= 0.0f && mouseViewportPos.x <= displaySize.x &&
                            mouseViewportPos.y >= 0.0f && mouseViewportPos.y <= displaySize.y)
                        {
                            auto& context = engine->GetRenderLayer().GetRenderContext();
                            float fboWidth = context.viewportWidth;
                            float fboHeight = context.viewportHeight;
                            if (fboWidth > 0 && fboHeight > 0)
                            {
                                float normX = std::clamp(mouseViewportPos.x / displaySize.x, 0.0f, 1.0f);
                                float normY = std::clamp(mouseViewportPos.y / displaySize.y, 0.0f, 1.0f);

                                uint32_t fboX = static_cast<uint32_t>(normX * (fboWidth - 1));
                                uint32_t fboY = static_cast<uint32_t>((1.0f - normY) * (fboHeight - 1));

                                uint32_t pickedID = engine->Selection.Pick(fboX, fboY);
                                if (pickedID != 0)
                                {
                                    entt::entity e = static_cast<entt::entity>(pickedID);
                                    if (engine->GetActiveScene().GetRegistry().valid(e))
                                    {
                                        hoveredEntity = e;
                                    }
                                }
                            }
                        }
                    }

                    static entt::entity s_LastHoveredEntity = entt::null;
                    if (hoveredEntity != entt::null)
                    {
                        s_LastHoveredEntity = hoveredEntity;
                    }
                    else if (payload->IsDelivery() && s_LastHoveredEntity != entt::null && engine->GetActiveScene().GetRegistry().valid(s_LastHoveredEntity))
                    {
                        hoveredEntity = s_LastHoveredEntity;
                    }

                    if (payload->IsDelivery())
                    {
                        s_LastHoveredEntity = entt::null;
                        Scene& scene = engine->GetActiveScene();
                        if (isMaterial)
                        {
                            if (hoveredEntity != entt::null)
                            {
                                entt::entity targetEntity = hoveredEntity;
                                if (!scene.HasComponent<MeshComponent>(targetEntity) &&
                                    !scene.HasComponent<SkeletalMeshComponent>(targetEntity) &&
                                    !scene.HasComponent<MaterialComponent>(targetEntity))
                                {
                                    entt::entity root = ResolveRootEntity(engine, hoveredEntity);
                                    if (scene.HasComponent<MeshComponent>(root) ||
                                        scene.HasComponent<SkeletalMeshComponent>(root) ||
                                        scene.HasComponent<MaterialComponent>(root))
                                    {
                                        targetEntity = root;
                                    }
                                }

                                auto& assetRegistry = AssetManager::Get().GetRegistry();
                                AssetUUID matUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
                                if (!matUUID.IsValid())
                                    matUUID = assetRegistry.GetUUID(pathStr);
                                if (!matUUID.IsValid())
                                    matUUID = assetRegistry.GetOrCreateMeta(pathStr).uuid;

                                if (matUUID.IsValid())
                                {
                                    json before = SceneSerializer::SerializeEntity(scene, targetEntity, false);
                                    if (scene.HasComponent<MaterialComponent>(targetEntity))
                                    {
                                        scene.GetComponent<MaterialComponent>(targetEntity).SetMaterial(matUUID);
                                    }
                                    else
                                    {
                                        auto& matComp = scene.AddComponent<MaterialComponent>(targetEntity);
                                        matComp.SetMaterial(matUUID);
                                    }
                                    json after = SceneSerializer::SerializeEntity(scene, targetEntity, false);

                                    if (before != after)
                                    {
                                        CommandHistory::Get().Execute(std::make_unique<EntitySnapshotCommand>(
                                            &scene, targetEntity, before, after, "Assign Material"));
                                    }
                                    scene.SetDirty(true);
                                    EditorSelection::Get().Select(targetEntity);
                                }
                                else
                                {
                                    LOG_WARN("[Viewport] Failed to resolve UUID for material: {}", pathStr);
                                }
                            }
                            else
                            {
                                LOG_WARN("[Viewport] No entity under cursor to assign material: {}", pathStr);
                            }
                        }
                        else if (isScript)
                        {
                            if (hoveredEntity != entt::null)
                            {
                                entt::entity targetEntity = ResolveRootEntity(engine, hoveredEntity);

                                auto& assetRegistry = AssetManager::Get().GetRegistry();
                                AssetMeta scriptMeta = assetRegistry.GetOrCreateMeta(pathStr);
                                AssetUUID scriptUUID = scriptMeta.uuid;
                                if (!scriptUUID.IsValid())
                                    scriptUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
                                if (!scriptUUID.IsValid())
                                    scriptUUID = assetRegistry.GetUUID(pathStr);

                                if (scriptUUID.IsValid())
                                {
                                    json before = SceneSerializer::SerializeEntity(scene, targetEntity, false);
                                    if (scene.HasComponent<ScriptComponent>(targetEntity))
                                    {
                                        auto& scriptComp = scene.GetComponent<ScriptComponent>(targetEntity);
                                        if (scriptComp.scriptAssetUUID != scriptUUID)
                                        {
                                            scriptComp.scriptAssetUUID = scriptUUID;
                                            scriptComp.propertyValues = json::object();
                                            ScriptEngine::InvalidateScriptPropertyDefs(scriptUUID);
                                        }
                                    }
                                    else
                                    {
                                        auto& scriptComp = scene.AddComponent<ScriptComponent>(targetEntity);
                                        scriptComp.scriptAssetUUID = scriptUUID;
                                        scriptComp.propertyValues = json::object();
                                        ScriptEngine::InvalidateScriptPropertyDefs(scriptUUID);
                                    }
                                    json after = SceneSerializer::SerializeEntity(scene, targetEntity, false);

                                    if (before != after)
                                    {
                                        CommandHistory::Get().Execute(std::make_unique<EntitySnapshotCommand>(
                                            &scene, targetEntity, before, after, "Attach Script"));
                                    }
                                    scene.SetDirty(true);
                                    EditorSelection::Get().Select(targetEntity);
                                }
                                else
                                {
                                    LOG_WARN("[Viewport] Failed to resolve UUID for script: {}", pathStr);
                                }
                            }
                            else
                            {
                                LOG_WARN("[Viewport] No entity under cursor to attach script: {}", pathStr);
                            }
                        }
                        else if (isPrefab)
                        {
                            AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(prefabPath);
                            if (uuid.IsValid())
                            {
                                glm::vec3 spawnPos = CalculateSpawnPosition(engine, mousePos, displayPos, displaySize);
                                Entity inst = scene.Instantiate(uuid, spawnPos);
                                if (inst.GetHandle() != entt::null)
                                {
                                    CommandHistory::Get().Execute(std::make_unique<CreateEntityCommand>(&scene, inst.GetHandle()));
                                    EditorSelection::Get().Select(inst.GetHandle());
                                    scene.SetDirty(true);
                                }
                            }
                            else
                            {
                                LOG_WARN("[Viewport] Failed to read UUID from meta for: {}", prefabPath);
                            }
                        }
                    }
                    else // Preview mode
                    {
                        std::filesystem::path p(pathStr);
                        std::string assetName = p.filename().string();
                        if (pathStr.find('#') != std::string::npos)
                            assetName = pathStr.substr(pathStr.find('#') + 1);

                        if (isMaterial)
                        {
                            if (hoveredEntity != entt::null)
                            {
                                entt::entity targetEntity = hoveredEntity;
                                Scene& scene = engine->GetActiveScene();
                                if (!scene.HasComponent<MeshComponent>(targetEntity) &&
                                    !scene.HasComponent<SkeletalMeshComponent>(targetEntity) &&
                                    !scene.HasComponent<MaterialComponent>(targetEntity))
                                {
                                    entt::entity root = ResolveRootEntity(engine, hoveredEntity);
                                    if (scene.HasComponent<MeshComponent>(root) ||
                                        scene.HasComponent<SkeletalMeshComponent>(root) ||
                                        scene.HasComponent<MaterialComponent>(root))
                                    {
                                        targetEntity = root;
                                    }
                                }
                                std::string entityName = scene.HasComponent<TagComponent>(targetEntity) ?
                                    scene.GetComponent<TagComponent>(targetEntity).tag : "Entity";
                                ImGui::SetTooltip("Assign Material '%s' to '%s'", assetName.c_str(), entityName.c_str());
                            }
                            else
                            {
                                ImGui::SetTooltip("Assign Material '%s' (hover over an object)", assetName.c_str());
                            }
                        }
                        else if (isScript)
                        {
                            if (hoveredEntity != entt::null)
                            {
                                entt::entity targetEntity = ResolveRootEntity(engine, hoveredEntity);
                                Scene& scene = engine->GetActiveScene();
                                std::string entityName = scene.HasComponent<TagComponent>(targetEntity) ?
                                    scene.GetComponent<TagComponent>(targetEntity).tag : "Entity";
                                ImGui::SetTooltip("Attach Script '%s' to '%s'", assetName.c_str(), entityName.c_str());
                            }
                            else
                            {
                                ImGui::SetTooltip("Attach Script '%s' (hover over an object)", assetName.c_str());
                            }
                        }
                        else if (isPrefab)
                        {
                            std::string prefabName = std::filesystem::path(prefabPath).stem().string();
                            ImGui::SetTooltip("Instantiate Prefab '%s'", prefabName.c_str());
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (m_PendingContextMenu)
        {
            m_PendingContextMenu = false;

            const float gizmoSize = 96.0f;
            const float margin = 10.0f;
            bool isOverOrientationGizmo = m_ContextMenuMousePos.x >= (displayPos.x + displaySize.x - gizmoSize - margin) &&
                m_ContextMenuMousePos.x <= (displayPos.x + displaySize.x - margin) &&
                m_ContextMenuMousePos.y >= (displayPos.y + margin) &&
                m_ContextMenuMousePos.y <= (displayPos.y + margin + gizmoSize);

            const float leftToolbarWidth = 308.0f;
            const float leftToolbarHeight = 32.0f;
            bool isOverLeftToolbar = m_ContextMenuMousePos.x >= (displayPos.x + 10.0f) &&
                m_ContextMenuMousePos.x <= (displayPos.x + 10.0f + leftToolbarWidth) &&
                m_ContextMenuMousePos.y >= (displayPos.y + 10.0f) &&
                m_ContextMenuMousePos.y <= (displayPos.y + 10.0f + leftToolbarHeight);

            const float rightToolbarWidth = 206.0f;
            const float rightToolbarHeight = 32.0f;
            float rightToolbarX = displayPos.x + displaySize.x - margin - gizmoSize - 8.0f - rightToolbarWidth;
            bool isOverRightToolbar = m_ContextMenuMousePos.x >= rightToolbarX &&
                m_ContextMenuMousePos.x <= (rightToolbarX + rightToolbarWidth) &&
                m_ContextMenuMousePos.y >= (displayPos.y + margin) &&
                m_ContextMenuMousePos.y <= (displayPos.y + margin + rightToolbarHeight);

            bool isOverStats = false;
            if (m_ShowStatistics)
            {
                const float statsW = 240.0f;
                const float statsH = 260.0f;
                float statsY = (engine->GetActiveScene().GetState() == SceneState::EDIT) ? 115.0f : 14.0f;
                isOverStats = m_ContextMenuMousePos.x >= (displayPos.x + displaySize.x - statsW - 14.0f) &&
                    m_ContextMenuMousePos.x <= (displayPos.x + displaySize.x - 14.0f) &&
                    m_ContextMenuMousePos.y >= (displayPos.y + statsY) &&
                    m_ContextMenuMousePos.y <= (displayPos.y + statsY + statsH);
            }

            if (!isOverOrientationGizmo && !isOverLeftToolbar && !isOverRightToolbar && !isOverStats && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
            {
                m_ContextSpawnPos = CalculateSpawnPosition(engine, m_ContextMenuMousePos, displayPos, displaySize);
                ImGui::OpenPopup("ViewportContextMenu");
            }
        }

        DrawContextMenu(engine, displayPos, displaySize);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

glm::vec3 Viewport::CalculateSpawnPosition(Engine* engine, const ImVec2& mousePos, const ImVec2& displayPos, const ImVec2& displaySize)
{
    m_ContextSpawnNormal = std::nullopt;

    auto* cam = engine->GetEditorCamera();
    if (!cam)
        return glm::vec3(0.0f);

    glm::vec3 spawnPos = cam->Position + cam->Front * 5.0f;

    if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
        return spawnPos;

    float mouseX = std::clamp(mousePos.x - displayPos.x, 0.0f, displaySize.x);
    float mouseY = std::clamp(mousePos.y - displayPos.y, 0.0f, displaySize.y);

    float ndcX = (2.0f * mouseX) / displaySize.x - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / displaySize.y;

    glm::mat4 invVP = glm::inverse(cam->GetProjectionMatrix() * cam->GetViewMatrix());
    glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

    if (nearPoint.w == 0.0f || farPoint.w == 0.0f)
        return spawnPos;

    glm::vec3 rayOrigin = glm::vec3(nearPoint) / nearPoint.w;
    glm::vec3 rayEnd = glm::vec3(farPoint) / farPoint.w;
    glm::vec3 rayDir = glm::normalize(rayEnd - rayOrigin);

    auto& scene = engine->GetActiveScene();

    Physics::RaycastResult hit = scene.GetPhysicsSystem().GetPhysicsWorld().Raycast(rayOrigin, rayDir, 2000.0f);
    if (hit.hit)
    {
        m_ContextSpawnNormal = hit.normal;
        return hit.point;
    }

    auto& context = engine->GetRenderLayer().GetRenderContext();
    float fboWidth = context.viewportWidth;
    float fboHeight = context.viewportHeight;
    float normX = std::clamp(mouseX / displaySize.x, 0.0f, 1.0f);
    float normY = std::clamp(mouseY / displaySize.y, 0.0f, 1.0f);
    uint32_t fboX = static_cast<uint32_t>(normX * (fboWidth - 1));
    uint32_t fboY = static_cast<uint32_t>((1.0f - normY) * (fboHeight - 1));
    uint32_t pickedID = engine->Selection.Pick(fboX, fboY);

    if (pickedID != 0)
    {
        entt::entity pickedEntity = static_cast<entt::entity>(pickedID);
        if (scene.GetRegistry().valid(pickedEntity) && scene.HasComponent<TransformComponent>(pickedEntity))
        {
            const auto& tc = scene.GetComponent<TransformComponent>(pickedEntity);
            glm::mat4 worldMat = tc.GetWorldMatrix();

            if (scene.HasComponent<MeshComponent>(pickedEntity))
            {
                auto& mc = scene.GetComponent<MeshComponent>(pickedEntity);
                auto mesh = mc.GetMesh();
                if (mesh && mesh->IsValid())
                {
                    glm::mat4 invWorld = glm::inverse(worldMat);
                    glm::vec3 localRayOrigin = glm::vec3(invWorld * glm::vec4(rayOrigin, 1.0f));
                    glm::vec3 localRayDir = glm::normalize(glm::vec3(invWorld * glm::vec4(rayDir, 0.0f)));

                    auto verts = mesh->GetVertices();
                    auto indices = mesh->GetIndices();
                    float closestT = std::numeric_limits<float>::max();
                    glm::vec3 localHitNormal(0.0f, 1.0f, 0.0f);
                    bool hitAny = false;

                    for (size_t i = 0; i + 2 < indices.size(); i += 3)
                    {
                        const auto& v0 = verts[indices[i]].position;
                        const auto& v1 = verts[indices[i + 1]].position;
                        const auto& v2 = verts[indices[i + 2]].position;
                        float t = 0.0f;
                        if (rv::math::RayIntersectsTriangle(localRayOrigin, localRayDir, v0, v1, v2, t))
                        {
                            if (t > 0.0f && t < closestT)
                            {
                                closestT = t;
                                hitAny = true;
                                localHitNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                            }
                        }
                    }

                    if (hitAny)
                    {
                        glm::vec3 localHitPos = localRayOrigin + localRayDir * closestT;
                        glm::vec3 worldHitPos = glm::vec3(worldMat * glm::vec4(localHitPos, 1.0f));
                        glm::vec3 worldNormal = glm::normalize(glm::mat3(glm::transpose(invWorld)) * localHitNormal);
                        m_ContextSpawnNormal = worldNormal;
                        return worldHitPos;
                    }
                }
            }

            glm::vec3 entityPos = tc.GetWorldPosition();
            glm::vec3 planeNormal = -cam->Front;
            float denom = glm::dot(rayDir, planeNormal);
            if (std::abs(denom) > 1e-4f)
            {
                float t = glm::dot(entityPos - rayOrigin, planeNormal) / denom;
                if (t > 0.0f)
                {
                    m_ContextSpawnNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                    return rayOrigin + rayDir * t;
                }
            }
        }
    }

    if (std::abs(rayDir.y) > 0.001f)
    {
        float t = -rayOrigin.y / rayDir.y;
        if (t > 0.0f && t < 1000.0f)
        {
            m_ContextSpawnNormal = glm::vec3(0.0f, 1.0f, 0.0f);
            return rayOrigin + rayDir * t;
        }
    }

    return rayOrigin + rayDir * 5.0f;
}

void Viewport::DrawContextMenu(Engine* engine, const ImVec2& displayPos, const ImVec2& displaySize)
{
    if (ImGui::BeginPopup("ViewportContextMenu"))
    {
        auto& scene = engine->GetActiveScene();
        auto registerCreated = [&](entt::entity handle) {
            if (handle != entt::null && scene.GetRegistry().valid(handle))
            {
                CommandHistory::Get().Execute(std::make_unique<CreateEntityCommand>(&scene, handle));
                EditorSelection::Get().Select(handle);
                scene.SetDirty(true);
            }
        };

        if (ImGui::MenuItem("Create Empty Entity"))
        {
            entt::entity e = scene.CreateEntity("New Entity");
            scene.GetComponent<TransformComponent>(e).SetPosition(m_ContextSpawnPos);
            registerCreated(e);
        }

        if (ImGui::MenuItem("Camera"))
        {
            entt::entity camEnt = scene.CreateEntity("Camera");
            scene.GetComponent<TransformComponent>(camEnt).SetPosition(m_ContextSpawnPos);
            scene.AddComponent<CameraComponent>(camEnt);
            registerCreated(camEnt);
        }

        if (ImGui::MenuItem("Particle Emitter"))
        {
            Entity emitterEntity = EditorUtils::LoadPrimitive(scene, "Quad");
            if (emitterEntity)
            {
                if (emitterEntity.HasComponent<RigidbodyComponent>())
                    emitterEntity.RemoveComponent<RigidbodyComponent>();
                if (emitterEntity.HasComponent<ConvexHullColliderComponent>())
                    emitterEntity.RemoveComponent<ConvexHullColliderComponent>();

                emitterEntity.GetComponent<TagComponent>().tag = "Particle Emitter";
                emitterEntity.GetComponent<TransformComponent>().SetPosition(m_ContextSpawnPos);
                emitterEntity.AddComponent<ParticleEmitterComponent>();
                registerCreated(emitterEntity.GetHandle());
            }
        }

        if (ImGui::MenuItem("Audio Emitter"))
        {
            entt::entity audio = scene.CreateEntity("AudioEmitter");
            scene.GetComponent<TransformComponent>(audio).SetPosition(m_ContextSpawnPos);
            scene.AddComponent<AudioEmitterComponent>(audio);
            registerCreated(audio);
        }

        if (ImGui::MenuItem("Decal"))
        {
            entt::entity decalEnt = scene.CreateEntity("Decal");
            auto& tc = scene.GetComponent<TransformComponent>(decalEnt);
            tc.SetPosition(m_ContextSpawnPos);
            tc.SetEulerRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
            scene.AddComponent<DecalComponent>(decalEnt);
            registerCreated(decalEnt);
        }

        if (ImGui::BeginMenu("3D Objects"))
        {
            auto createPrimitive = [&](const std::string& name) {
                Entity prim = EditorUtils::LoadPrimitive(scene, name);
                if (prim.GetHandle() != entt::null)
                {
                    glm::vec3 pos = m_ContextSpawnPos;
                    if (m_ContextSpawnNormal.has_value())
                    {
                        pos += m_ContextSpawnNormal.value() * 0.5f;
                    }
                    prim.GetComponent<TransformComponent>().SetPosition(pos);
                    registerCreated(prim.GetHandle());
                }
            };

            if (ImGui::MenuItem("Cube")) createPrimitive("Cube");
            if (ImGui::MenuItem("Sphere")) createPrimitive("Sphere");
            if (ImGui::MenuItem("Cylinder")) createPrimitive("Cylinder");
            if (ImGui::MenuItem("Quad")) createPrimitive("Quad");
            if (ImGui::MenuItem("Cone")) createPrimitive("Cone");
            if (ImGui::MenuItem("Capsule")) createPrimitive("Capsule");
            if (ImGui::MenuItem("Torus")) createPrimitive("Torus");
            if (ImGui::MenuItem("Plane")) createPrimitive("Plane");
            if (ImGui::MenuItem("Monkey")) createPrimitive("Monkey");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Lights"))
        {
            if (ImGui::MenuItem("Directional Light"))
            {
                entt::entity light = scene.CreateEntity("DirectionalLight");
                auto& tc = scene.GetComponent<TransformComponent>(light);
                tc.SetPosition(m_ContextSpawnPos);
                tc.SetEulerRotation(glm::vec3(-60.0f, -90.0f, 0.0f));
                scene.AddComponent<DirectionalLightComponent>(light);
                registerCreated(light);
            }
            if (ImGui::MenuItem("Point Light"))
            {
                entt::entity light = scene.CreateEntity("PointLight");
                scene.GetComponent<TransformComponent>(light).SetPosition(m_ContextSpawnPos);
                scene.AddComponent<PointLightComponent>(light);
                registerCreated(light);
            }
            if (ImGui::MenuItem("Spot Light"))
            {
                entt::entity light = scene.CreateEntity("SpotLight");
                auto& tc = scene.GetComponent<TransformComponent>(light);
                tc.SetPosition(m_ContextSpawnPos);
                tc.SetEulerRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
                scene.AddComponent<SpotLightComponent>(light);
                registerCreated(light);
            }
            if (ImGui::MenuItem("Reflection Probe"))
            {
                entt::entity probeEnt = scene.CreateEntity("Reflection Probe");
                scene.GetComponent<TransformComponent>(probeEnt).SetPosition(m_ContextSpawnPos);
                scene.AddComponent<ReflectionProbeComponent>(probeEnt);
                registerCreated(probeEnt);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("UI"))
        {
            if (ImGui::MenuItem("UI Canvas"))
            {
                entt::entity canvas = scene.CreateEntity("Canvas");
                scene.AddComponent<UICanvasComponent>(canvas);
                scene.AddComponent<RectTransformComponent>(canvas);
                registerCreated(canvas);
            }
            if (ImGui::MenuItem("UI Text"))
            {
                entt::entity text = scene.CreateEntity("UI Text");
                scene.AddComponent<RectTransformComponent>(text);
                scene.AddComponent<UITextComponent>(text);
                registerCreated(text);
            }
            if (ImGui::MenuItem("UI Button"))
            {
                entt::entity button = scene.CreateEntity("UI Button");
                scene.AddComponent<RectTransformComponent>(button);
                scene.AddComponent<UIButtonComponent>(button);
                registerCreated(button);
            }
            if (ImGui::MenuItem("UI Image"))
            {
                entt::entity img = scene.CreateEntity("UI Image");
                scene.AddComponent<RectTransformComponent>(img);
                scene.AddComponent<UIImageComponent>(img);
                registerCreated(img);
            }
            if (ImGui::MenuItem("UI Slider"))
            {
                entt::entity slider = scene.CreateEntity("UI Slider");
                scene.AddComponent<RectTransformComponent>(slider);
                scene.AddComponent<UISliderComponent>(slider);
                registerCreated(slider);
            }
            if (ImGui::MenuItem("UI Progress Bar"))
            {
                entt::entity pbar = scene.CreateEntity("UI Progress Bar");
                scene.AddComponent<RectTransformComponent>(pbar);
                scene.AddComponent<UIProgressBarComponent>(pbar);
                registerCreated(pbar);
            }
            if (ImGui::MenuItem("UI Checkbox"))
            {
                entt::entity cb = scene.CreateEntity("UI Checkbox");
                scene.AddComponent<RectTransformComponent>(cb);
                scene.AddComponent<UICheckboxComponent>(cb);
                registerCreated(cb);
            }
            if (ImGui::MenuItem("UI Panel"))
            {
                entt::entity panel = scene.CreateEntity("UI Panel");
                auto& rect = scene.AddComponent<RectTransformComponent>(panel);
                rect.size = { 300.0f, 200.0f };
                scene.AddComponent<UIPanelComponent>(panel);
                registerCreated(panel);
            }
            if (ImGui::MenuItem("UI Vertical Container"))
            {
                entt::entity vbox = scene.CreateEntity("UI Vertical Container");
                auto& rect = scene.AddComponent<RectTransformComponent>(vbox);
                rect.size = { 200.0f, 300.0f };
                auto& lg = scene.AddComponent<UILayoutGroupComponent>(vbox);
                lg.type = UILayoutType::Vertical;
                registerCreated(vbox);
            }
            if (ImGui::MenuItem("UI Horizontal Container"))
            {
                entt::entity hbox = scene.CreateEntity("UI Horizontal Container");
                auto& rect = scene.AddComponent<RectTransformComponent>(hbox);
                rect.size = { 400.0f, 60.0f };
                auto& lg = scene.AddComponent<UILayoutGroupComponent>(hbox);
                lg.type = UILayoutType::Horizontal;
                registerCreated(hbox);
            }
            if (ImGui::MenuItem("UI Text Input"))
            {
                entt::entity input = scene.CreateEntity("UI Text Input");
                auto& rect = scene.AddComponent<RectTransformComponent>(input);
                rect.size = { 200.0f, 36.0f };
                scene.AddComponent<UITextInputComponent>(input);
                registerCreated(input);
            }
            if (ImGui::MenuItem("UI Scroll View"))
            {
                entt::entity sv = scene.CreateEntity("UI Scroll View");
                auto& rect = scene.AddComponent<RectTransformComponent>(sv);
                rect.size = { 250.0f, 200.0f };
                scene.AddComponent<UIScrollRectComponent>(sv);
                registerCreated(sv);
            }
            if (ImGui::MenuItem("UI Tab Container"))
            {
                entt::entity tabs = scene.CreateEntity("UI Tab Container");
                auto& rect = scene.AddComponent<RectTransformComponent>(tabs);
                rect.size = { 350.0f, 250.0f };
                scene.AddComponent<UITabContainerComponent>(tabs);
                registerCreated(tabs);
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }
}
