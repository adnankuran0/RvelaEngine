#include "EditorSettings.h"
#include "Utils/FileUtils.h"
#include "Core/Log.h"
#include "Renderer/DebugRenderer.h"
#include <nlohmann/json.hpp>
#include <fstream>

using namespace rv;
using json = nlohmann::json;

EditorSettings& EditorSettings::Get()
{
    static EditorSettings s_Instance;
    return s_Instance;
}

std::filesystem::path EditorSettings::GetSettingsFilePath()
{
    return EDITOR_PATH("editor_settings.json").GetAbsolute();
}

void EditorSettings::Load()
{
    std::filesystem::path filePath = GetSettingsFilePath();
    if (!std::filesystem::exists(filePath))
        return;

    std::ifstream file(filePath);
    if (!file.is_open())
        return;

    try
    {
        json j;
        file >> j;

        if (j.contains("camera") && j["camera"].is_object())
        {
            auto& c = j["camera"];
            if (c.contains("fov")) Camera.FOV = c["fov"].get<float>();
            if (c.contains("orthoSize")) Camera.OrthoSize = c["orthoSize"].get<float>();
            if (c.contains("nearClip")) Camera.NearClip = c["nearClip"].get<float>();
            if (c.contains("farClip")) Camera.FarClip = c["farClip"].get<float>();
            if (c.contains("projection")) Camera.ProjectionType = c["projection"].get<int>();
            if (c.contains("movementSpeed")) Camera.MovementSpeed = c["movementSpeed"].get<float>();
            if (c.contains("sprintSpeed")) Camera.SprintSpeed = c["sprintSpeed"].get<float>();
            if (c.contains("mouseSensitivity")) Camera.MouseSensitivity = c["mouseSensitivity"].get<float>();
            if (c.contains("positionSmoothness")) Camera.PositionSmoothness = c["positionSmoothness"].get<float>();
        }

        if (j.contains("viewport") && j["viewport"].is_object())
        {
            auto& v = j["viewport"];
            if (v.contains("gizmoMode")) ViewportSettings.GizmoMode = v["gizmoMode"].get<int>();
            if (v.contains("enableSnap")) ViewportSettings.EnableSnap = v["enableSnap"].get<bool>();
            if (v.contains("snapTranslate")) ViewportSettings.SnapTranslate = v["snapTranslate"].get<float>();
            if (v.contains("snapRotate")) ViewportSettings.SnapRotate = v["snapRotate"].get<float>();
            if (v.contains("snapScale")) ViewportSettings.SnapScale = v["snapScale"].get<float>();
            if (v.contains("drawColliders")) ViewportSettings.DrawColliders = v["drawColliders"].get<bool>();
            if (v.contains("drawBoundingBoxes")) ViewportSettings.DrawBoundingBoxes = v["drawBoundingBoxes"].get<bool>();
            if (v.contains("drawInfiniteGrid")) ViewportSettings.DrawInfiniteGrid = v["drawInfiniteGrid"].get<bool>();
        }
    }
    catch (const std::exception& e)
    {
        LOG_WARN("Failed to parse editor settings: {}", e.what());
    }
}

void EditorSettings::Save()
{
    std::filesystem::path filePath = GetSettingsFilePath();
    std::error_code ec;
    std::filesystem::create_directories(filePath.parent_path(), ec);

    json j;
    j["camera"] = {
        { "fov", Camera.FOV },
        { "orthoSize", Camera.OrthoSize },
        { "nearClip", Camera.NearClip },
        { "farClip", Camera.FarClip },
        { "projection", Camera.ProjectionType },
        { "movementSpeed", Camera.MovementSpeed },
        { "sprintSpeed", Camera.SprintSpeed },
        { "mouseSensitivity", Camera.MouseSensitivity },
        { "positionSmoothness", Camera.PositionSmoothness }
    };

    j["viewport"] = {
        { "gizmoMode", ViewportSettings.GizmoMode },
        { "enableSnap", ViewportSettings.EnableSnap },
        { "snapTranslate", ViewportSettings.SnapTranslate },
        { "snapRotate", ViewportSettings.SnapRotate },
        { "snapScale", ViewportSettings.SnapScale },
        { "drawColliders", ViewportSettings.DrawColliders },
        { "drawBoundingBoxes", ViewportSettings.DrawBoundingBoxes },
        { "drawInfiniteGrid", ViewportSettings.DrawInfiniteGrid }
    };

    std::ofstream file(filePath);
    if (file.is_open())
    {
        file << j.dump(4);
    }
}

void EditorSettings::ApplyToCamera(EditorCamera& camera) const
{
    camera.FOV = Camera.FOV;
    camera.OrthoSize = Camera.OrthoSize;
    camera.NearClip = Camera.NearClip;
    camera.FarClip = Camera.FarClip;
    camera.ProjectionType = (Camera.ProjectionType == 1) ? Camera::Projection::Orthographic : Camera::Projection::Perspective;
    camera.MovementSpeed = Camera.MovementSpeed;
    camera.SprintSpeed = Camera.SprintSpeed;
    camera.MouseSensitivity = Camera.MouseSensitivity;
    camera.positionSmoothness = Camera.PositionSmoothness;
    camera.UpdateFrustum();
}

void EditorSettings::UpdateFromCamera(const EditorCamera& camera)
{
    Camera.FOV = camera.FOV;
    Camera.OrthoSize = camera.OrthoSize;
    Camera.NearClip = camera.NearClip;
    Camera.FarClip = camera.FarClip;
    Camera.ProjectionType = (camera.ProjectionType == Camera::Projection::Orthographic) ? 1 : 0;
    Camera.MovementSpeed = camera.MovementSpeed;
    Camera.SprintSpeed = camera.SprintSpeed;
    Camera.MouseSensitivity = camera.MouseSensitivity;
    Camera.PositionSmoothness = camera.positionSmoothness;
}

void EditorSettings::ApplyToViewport(Viewport& viewport) const
{
    viewport.SetGizmoMode((ViewportSettings.GizmoMode == 1) ? ImGuizmo::LOCAL : ImGuizmo::WORLD);
    viewport.SetEnableSnap(ViewportSettings.EnableSnap);
    viewport.SetSnapTranslate(ViewportSettings.SnapTranslate);
    viewport.SetSnapRotate(ViewportSettings.SnapRotate);
    viewport.SetSnapScale(ViewportSettings.SnapScale);

    auto& debugSettings = DebugRenderer::Get().GetSettings();
    debugSettings.drawColliders = ViewportSettings.DrawColliders;
    debugSettings.drawBoundingBoxes = ViewportSettings.DrawBoundingBoxes;
    debugSettings.drawInfiniteGrid = ViewportSettings.DrawInfiniteGrid;
}

void EditorSettings::UpdateFromViewport(const Viewport& viewport)
{
    ViewportSettings.GizmoMode = (viewport.GetGizmoMode() == ImGuizmo::LOCAL) ? 1 : 0;
    ViewportSettings.EnableSnap = viewport.GetEnableSnap();
    ViewportSettings.SnapTranslate = viewport.GetSnapTranslate();
    ViewportSettings.SnapRotate = viewport.GetSnapRotate();
    ViewportSettings.SnapScale = viewport.GetSnapScale();

    const auto& debugSettings = DebugRenderer::Get().GetSettings();
    ViewportSettings.DrawColliders = debugSettings.drawColliders;
    ViewportSettings.DrawBoundingBoxes = debugSettings.drawBoundingBoxes;
    ViewportSettings.DrawInfiniteGrid = debugSettings.drawInfiniteGrid;
}
