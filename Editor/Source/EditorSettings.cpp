#include "EditorSettings.h"
#include "Utils/FileUtils.h"
#include "Core/Log.h"
#include "Renderer/DebugRenderer.h"
#include "Render/GizmoPass.h"
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

            if (c.contains("position") && c["position"].is_array() && c["position"].size() == 3)
                Camera.Position = glm::vec3(c["position"][0].get<float>(), c["position"][1].get<float>(), c["position"][2].get<float>());
            if (c.contains("targetPosition") && c["targetPosition"].is_array() && c["targetPosition"].size() == 3)
                Camera.TargetPosition = glm::vec3(c["targetPosition"][0].get<float>(), c["targetPosition"][1].get<float>(), c["targetPosition"][2].get<float>());
            else
                Camera.TargetPosition = Camera.Position;
            if (c.contains("orbitTarget") && c["orbitTarget"].is_array() && c["orbitTarget"].size() == 3)
                Camera.OrbitTarget = glm::vec3(c["orbitTarget"][0].get<float>(), c["orbitTarget"][1].get<float>(), c["orbitTarget"][2].get<float>());
            if (c.contains("orbitDistance")) Camera.OrbitDistance = c["orbitDistance"].get<float>();
            if (c.contains("yaw")) Camera.Yaw = c["yaw"].get<float>();
            if (c.contains("pitch")) Camera.Pitch = c["pitch"].get<float>();
        }

        if (j.contains("sceneCameras") && j["sceneCameras"].is_object())
        {
            SceneCameraSettings.clear();
            for (auto& [path, sc] : j["sceneCameras"].items())
            {
                if (!sc.is_object()) continue;
                EditorCameraSettings camSettings = Camera;
                if (sc.contains("position") && sc["position"].is_array() && sc["position"].size() == 3)
                    camSettings.Position = glm::vec3(sc["position"][0].get<float>(), sc["position"][1].get<float>(), sc["position"][2].get<float>());
                if (sc.contains("targetPosition") && sc["targetPosition"].is_array() && sc["targetPosition"].size() == 3)
                    camSettings.TargetPosition = glm::vec3(sc["targetPosition"][0].get<float>(), sc["targetPosition"][1].get<float>(), sc["targetPosition"][2].get<float>());
                else
                    camSettings.TargetPosition = camSettings.Position;
                if (sc.contains("orbitTarget") && sc["orbitTarget"].is_array() && sc["orbitTarget"].size() == 3)
                    camSettings.OrbitTarget = glm::vec3(sc["orbitTarget"][0].get<float>(), sc["orbitTarget"][1].get<float>(), sc["orbitTarget"][2].get<float>());
                if (sc.contains("orbitDistance")) camSettings.OrbitDistance = sc["orbitDistance"].get<float>();
                if (sc.contains("yaw")) camSettings.Yaw = sc["yaw"].get<float>();
                if (sc.contains("pitch")) camSettings.Pitch = sc["pitch"].get<float>();
                SceneCameraSettings[path] = camSettings;
            }
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
            if (v.contains("drawLODLevels")) ViewportSettings.DrawLODLevels = v["drawLODLevels"].get<bool>();
            if (v.contains("drawInfiniteGrid")) ViewportSettings.DrawInfiniteGrid = v["drawInfiniteGrid"].get<bool>();
            if (v.contains("selectRootEntity")) ViewportSettings.SelectRootEntity = v["selectRootEntity"].get<bool>();
            if (v.contains("drawTransformGizmo")) ViewportSettings.DrawTransformGizmo = v["drawTransformGizmo"].get<bool>();
            if (v.contains("drawSceneGizmos")) ViewportSettings.DrawSceneGizmos = v["drawSceneGizmos"].get<bool>();
            if (v.contains("showStatistics")) ViewportSettings.ShowStatistics = v["showStatistics"].get<bool>();
        }

        if (j.contains("theme"))
        {
            Theme = j["theme"].get<int>();
        }

        if (j.contains("panels") && j["panels"].is_object())
        {
            for (auto& [key, val] : j["panels"].items())
            {
                if (val.is_boolean())
                    Panels[key] = val.get<bool>();
            }
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
        { "positionSmoothness", Camera.PositionSmoothness },
        { "position", { Camera.Position.x, Camera.Position.y, Camera.Position.z } },
        { "targetPosition", { Camera.TargetPosition.x, Camera.TargetPosition.y, Camera.TargetPosition.z } },
        { "orbitTarget", { Camera.OrbitTarget.x, Camera.OrbitTarget.y, Camera.OrbitTarget.z } },
        { "orbitDistance", Camera.OrbitDistance },
        { "yaw", Camera.Yaw },
        { "pitch", Camera.Pitch }
    };

    json sceneCamsJson = json::object();
    for (const auto& [path, cam] : SceneCameraSettings)
    {
        sceneCamsJson[path] = {
            { "position", { cam.Position.x, cam.Position.y, cam.Position.z } },
            { "targetPosition", { cam.TargetPosition.x, cam.TargetPosition.y, cam.TargetPosition.z } },
            { "orbitTarget", { cam.OrbitTarget.x, cam.OrbitTarget.y, cam.OrbitTarget.z } },
            { "orbitDistance", cam.OrbitDistance },
            { "yaw", cam.Yaw },
            { "pitch", cam.Pitch }
        };
    }
    j["sceneCameras"] = sceneCamsJson;

    j["viewport"] = {
        { "gizmoMode", ViewportSettings.GizmoMode },
        { "enableSnap", ViewportSettings.EnableSnap },
        { "snapTranslate", ViewportSettings.SnapTranslate },
        { "snapRotate", ViewportSettings.SnapRotate },
        { "snapScale", ViewportSettings.SnapScale },
        { "drawColliders", ViewportSettings.DrawColliders },
        { "drawBoundingBoxes", ViewportSettings.DrawBoundingBoxes },
        { "drawLODLevels", ViewportSettings.DrawLODLevels },
        { "drawInfiniteGrid", ViewportSettings.DrawInfiniteGrid },
        { "selectRootEntity", ViewportSettings.SelectRootEntity },
        { "drawTransformGizmo", ViewportSettings.DrawTransformGizmo },
        { "drawSceneGizmos", ViewportSettings.DrawSceneGizmos },
        { "showStatistics", ViewportSettings.ShowStatistics }
    };

    j["theme"] = Theme;
    j["panels"] = Panels;

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

    camera.Position = Camera.Position;
    camera.targetPosition = Camera.TargetPosition;
    camera.orbitTarget = Camera.OrbitTarget;
    camera.orbitDistance = Camera.OrbitDistance;
    camera.Yaw = Camera.Yaw;
    camera.Pitch = Camera.Pitch;
    camera.UpdateCameraVectors();
    camera.ResetMotion();
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

    Camera.Position = camera.Position;
    Camera.TargetPosition = camera.targetPosition;
    Camera.OrbitTarget = camera.orbitTarget;
    Camera.OrbitDistance = camera.orbitDistance;
    Camera.Yaw = camera.Yaw;
    Camera.Pitch = camera.Pitch;
}

void EditorSettings::SaveCameraForScene(const std::string& scenePath, const EditorCamera& camera)
{
    UpdateFromCamera(camera);
    if (!scenePath.empty())
    {
        SceneCameraSettings[scenePath] = Camera;
    }
}

bool EditorSettings::LoadCameraForScene(const std::string& scenePath, EditorCamera& camera)
{
    if (!scenePath.empty())
    {
        auto it = SceneCameraSettings.find(scenePath);
        if (it != SceneCameraSettings.end())
        {
            Camera = it->second;
            ApplyToCamera(camera);
            return true;
        }
    }
    ApplyToCamera(camera);
    return false;
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
    debugSettings.drawLODLevels = ViewportSettings.DrawLODLevels;
    debugSettings.drawInfiniteGrid = ViewportSettings.DrawInfiniteGrid;
    viewport.SetSelectRootEntity(ViewportSettings.SelectRootEntity);
    viewport.SetDrawGizmos(ViewportSettings.DrawTransformGizmo);
    viewport.SetShowStatistics(ViewportSettings.ShowStatistics);
    GizmoPass::s_Enabled = ViewportSettings.DrawSceneGizmos;
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
    ViewportSettings.DrawLODLevels = debugSettings.drawLODLevels;
    ViewportSettings.DrawInfiniteGrid = debugSettings.drawInfiniteGrid;
    ViewportSettings.SelectRootEntity = viewport.GetSelectRootEntity();
    ViewportSettings.DrawTransformGizmo = viewport.GetDrawGizmos();
    ViewportSettings.ShowStatistics = viewport.GetShowStatistics();
    ViewportSettings.DrawSceneGizmos = GizmoPass::s_Enabled;
}
