#include "Viewport.h"
#include <iostream>
#include "ImGui/imgui.h"
#include "ImGui/ImGuizmo.h"
#include "Core/Engine.h"
#include <glm/gtx/matrix_decompose.hpp>
#include "Renderer/RenderLayer.h"
#include "Renderer/DebugRenderer.h"

using namespace rv;

void Viewport::DrawGizmos(Engine* engine, ImVec2& displayPos, ImVec2& displaySize, entt::entity selectedEntity)
{
    bool canDrawGizmo = selectedEntity != entt::null &&
        engine->GetActiveScene().GetRegistry().any_of<TransformComponent>(selectedEntity) &&
        engine->GetActiveScene().GetState() == SceneState::EDIT;
    if (!canDrawGizmo) return;

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(displayPos.x, displayPos.y, displaySize.x, displaySize.y);

    auto& tc = engine->GetActiveScene().GetRegistry().get<TransformComponent>(selectedEntity);
    glm::mat4 transform = tc.GetWorldMatrix();

    glm::mat4 view = engine->GetCamera()->GetViewMatrix();
    glm::mat4 projection = engine->GetCamera()->GetProjectionMatrix();

    if (ImGui::IsWindowFocused())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            m_CurrentGizmoMode = (m_CurrentGizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
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

    bool useSnap = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || m_EnableSnap;

    float snapTranslate[3] = { m_snapTranslate, m_snapTranslate, m_snapTranslate };
    float snapScale[3] = { m_snapScale, m_snapScale, m_snapScale };

    if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE)
    {
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
            m_CurrentGizmoOperation, m_CurrentGizmoMode,
            glm::value_ptr(transform), nullptr,
            useSnap ? snapTranslate : nullptr);
    }
    else if (m_CurrentGizmoOperation == ImGuizmo::ROTATE)
    {
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
            m_CurrentGizmoOperation, m_CurrentGizmoMode,
            glm::value_ptr(transform), nullptr,
            useSnap ? &m_snapRotate : nullptr);
    }
    else if (m_CurrentGizmoOperation == ImGuizmo::SCALE)
    {
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
            m_CurrentGizmoOperation, m_CurrentGizmoMode,
            glm::value_ptr(transform), nullptr,
            useSnap ? snapScale : nullptr);
    }

    if (ImGuizmo::IsUsing())
    {
        glm::mat4 parentWorld(1.0f);
        auto& scene = engine->GetActiveScene();
        auto& reg = scene.GetRegistry();
        auto& node = reg.get<SceneTreeComponent>(selectedEntity);
        if (node.parent != entt::null && reg.valid(node.parent))
        {
            parentWorld = reg.get<TransformComponent>(node.parent).GetWorldMatrix();
        }

        glm::mat4 localM = glm::inverse(parentWorld) * transform;
        glm::vec3 lPos, lScale, skew;
        glm::quat lRot;
        glm::vec4 persp;
        glm::decompose(localM, lScale, lRot, lPos, skew, persp);

        if (useSnap && m_CurrentGizmoOperation == ImGuizmo::SCALE)
        {
            lScale.x = glm::round(lScale.x / m_snapScale) * m_snapScale;
            lScale.y = glm::round(lScale.y / m_snapScale) * m_snapScale;
            lScale.z = glm::round(lScale.z / m_snapScale) * m_snapScale;
        }

        tc.SetPosition(lPos);
        tc.SetScale(lScale);
        tc.SetRotation(lRot);
    }
}

void Viewport::DrawToolbar(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 toolbarPos = ImVec2(displayPos.x + 10.0f, displayPos.y + 10.0f);
    ImVec2 toolbarSize = ImVec2(300.0f, 32.0f);

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
        m_CurrentGizmoMode = (m_CurrentGizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Transform Space (T)");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (ModeButton("Snap", m_EnableSnap, ImVec2(44.0f, 24.0f)))
        m_EnableSnap = !m_EnableSnap;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Snapping (Ctrl)");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (ModeButton("View", false, ImVec2(42.0f, 24.0f)))
        ImGui::OpenPopup("ViewportSettingsPopup");

    auto& debugSettings = DebugRenderer::Get().GetSettings();
    if (ImGui::BeginPopup("ViewportSettingsPopup"))
    {
        ImGui::TextDisabled("Debug Visuals");
        ImGui::Separator();
        ImGui::Checkbox("Draw Colliders", &debugSettings.drawColliders);
        ImGui::Checkbox("Draw Bounding Boxes", &debugSettings.drawBoundingBoxes);

        ImGui::Spacing();
        ImGui::TextDisabled("Snapping Values");
        ImGui::Separator();

        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("Translate (m)", &m_snapTranslate, 0.05f, 0.01f, 100.0f, "%.2f");

        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("Rotate (deg)", &m_snapRotate, 1.0f, 0.1f, 180.0f, "%.1f");

        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("Scale", &m_snapScale, 0.05f, 0.01f, 10.0f, "%.2f");

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);
}

void Viewport::DrawOverlayStats(Engine* engine, ImVec2& displayPos, ImVec2& displaySize)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (ImGui::GetIO().KeyCtrl || m_EnableSnap)
    {
        const char* label = "";
        float val = 0.0f;
        if (m_CurrentGizmoOperation == ImGuizmo::TRANSLATE) { label = "Snap: %.2fm"; val = m_snapTranslate; }
        else if (m_CurrentGizmoOperation == ImGuizmo::ROTATE) { label = "Snap: %.1f°"; val = m_snapRotate; }
        else if (m_CurrentGizmoOperation == ImGuizmo::SCALE) { label = "Snap: %.2f";  val = m_snapScale; }

        char buf[64];
        snprintf(buf, sizeof(buf), label, val);
        ImVec2 textPos = ImVec2(displayPos.x + 12.0f, displayPos.y + 48.0f);
        drawList->AddRectFilled(ImVec2(textPos.x - 4, textPos.y - 2), ImVec2(textPos.x + 80, textPos.y + 18), IM_COL32(18, 20, 24, 200), 4.0f);
        drawList->AddText(textPos, IM_COL32(245, 195, 65, 255), buf);
    }

    char statBuf[128];
    snprintf(statBuf, sizeof(statBuf), "%.0f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImVec2 statTextSize = ImGui::CalcTextSize(statBuf);
    ImVec2 statPos = ImVec2(displayPos.x + displaySize.x - statTextSize.x - 14.0f, displayPos.y + 10.0f);

    drawList->AddRectFilled(ImVec2(statPos.x - 6.0f, statPos.y - 4.0f), ImVec2(statPos.x + statTextSize.x + 6.0f, statPos.y + statTextSize.y + 4.0f),
        IM_COL32(18, 20, 24, 180), 4.0f);
    drawList->AddText(statPos, IM_COL32(160, 165, 175, 255), statBuf);
}

void Viewport::HandleSelection(Engine* engine, ImVec2& displayPos, ImVec2& displaySize, entt::entity& selectedEntity)
{
    if (engine->GetActiveScene().GetState() != SceneState::EDIT) return;

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool isMouseInViewport = mousePos.x >= displayPos.x && mousePos.x <= (displayPos.x + displaySize.x) &&
        mousePos.y >= displayPos.y && mousePos.y <= (displayPos.y + displaySize.y);

    if (isMouseInViewport && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver())
    {
        glm::vec2 mouseViewportPos = { mousePos.x - displayPos.x, mousePos.y - displayPos.y };

        auto& context = engine->GetRenderLayer().GetRenderContext();
        float fboWidth = context.viewportWidth;
        float fboHeight = context.viewportHeight;

        uint32_t fboX = static_cast<uint32_t>(mouseViewportPos.x * fboWidth / displaySize.x);
        uint32_t fboY = static_cast<uint32_t>(fboHeight - ((displaySize.y - mouseViewportPos.y) * fboHeight / displaySize.y));

        uint32_t pickedID = engine->Selection.Pick(fboX, fboY);

        if (pickedID != 0)
        {
            entt::entity entity = static_cast<entt::entity>(pickedID);
            selectedEntity = entity;
            engine->GetActiveScene().SetSelectedEntity(entity);
        }
        else
        {
            selectedEntity = entt::null;
            engine->GetActiveScene().SetSelectedEntity(entt::null);
        }
    }
}

void Viewport::Draw(Engine* engine, entt::entity& selectedEntity)
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

        DrawGizmos(engine, displayPos, displaySize, selectedEntity);
        HandleSelection(engine, displayPos, displaySize, selectedEntity);

        if(engine->GetActiveScene().GetState() == SceneState::EDIT)
            DrawToolbar(engine, displayPos, displaySize);
        DrawOverlayStats(engine, displayPos, displaySize);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
