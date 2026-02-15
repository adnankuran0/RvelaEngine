#pragma once

#include <glm/glm.hpp>
#include "Scene/ICamera.h"

struct GLFWwindow;

namespace rv {

class EditorCamera : public ICamera
{
public:
    EditorCamera(glm::vec3 position = { 0.0f, 4.0f, 10.0f },
        glm::vec3 up = { 0.0f, 1.0f, 0.0f },
        int width = 1920, int height = 1080);

    EditorCamera(float posX, float posY, float posZ,
        float upX, float upY, float upZ,
        float yaw, float pitch,
        int width, int height);

    glm::mat4 GetViewMatrix() override;
    glm::mat4 GetProjectionMatrix() override;
    glm::vec3 GetPosition() override;

    void Update();
    void OnMouseMoved(double xPosIn, double yPosIn, GLFWwindow* window);
    void ProcessMouseScroll(float yoffset);

private:
    void ProcessKeyboard();
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void UpdateCameraVectors();

public:
    glm::vec3 Position{};
    glm::vec3 Front{ 0.0f, 0.0f, -1.0f };
    glm::vec3 Up{};
    glm::vec3 Right{};
    glm::vec3 WorldUp{};

    float Yaw{ -90.0f };
    float Pitch{ 0.0f };

    float MovementSpeed{ 5.0f };
    float SprintSpeed{ 10.0f };
    float MouseSensitivity{ 0.075f };
    float Zoom{ 70.0f };

    float lastX{ 0.0f };
    float lastY{ 0.0f };
    bool firstMouse{ true };

    float positionSmoothness{ 20.0f };
    glm::vec3 targetPosition{};

    int width{ 1920 };
    int height{ 1080 };
};

}