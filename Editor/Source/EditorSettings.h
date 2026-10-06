#pragma once

#include <filesystem>
#include "Renderer/Camera.h"
#include "Renderer/EditorCamera.h"
#include "GUI/Viewport.h"

namespace rv {

struct EditorCameraSettings
{
    float FOV = 90.0f;
    float OrthoSize = 10.0f;
    float NearClip = 0.1f;
    float FarClip = 1000.0f;
    int ProjectionType = 0; // 0 Perspective, 1 Orthographic
    float MovementSpeed = 3.0f;
    float SprintSpeed = 5.0f;
    float MouseSensitivity = 0.075f;
    float PositionSmoothness = 25.0f;
};

struct ViewportGizmoSettings
{
    int GizmoMode = 0; // 0 World, 1 Local
    bool EnableSnap = false;
    float SnapTranslate = 1.0f;
    float SnapRotate = 15.0f;
    float SnapScale = 0.5f;
    bool DrawColliders = false;
    bool DrawBoundingBoxes = false;
    bool DrawInfiniteGrid = true;
};

class EditorSettings
{
public:
    static EditorSettings& Get();

    void Load();
    void Save();

    void ApplyToCamera(EditorCamera& camera) const;
    void UpdateFromCamera(const EditorCamera& camera);

    void ApplyToViewport(Viewport& viewport) const;
    void UpdateFromViewport(const Viewport& viewport);

    EditorCameraSettings Camera;
    ViewportGizmoSettings ViewportSettings;

private:
    EditorSettings() = default;
    static std::filesystem::path GetSettingsFilePath();
};

}
