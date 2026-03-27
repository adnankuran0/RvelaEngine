#pragma once

#include <glm/glm.hpp>
#include "Renderer/Camera.h"

struct GLFWwindow;

namespace rv {

class EditorCamera : public Camera
{
public:
    EditorCamera(glm::vec3 position = { 0.0f, 2.0f, 5.0f },
        glm::vec3 up = { 0.0f, 1.0f, 0.0f },
        int width = 1920, int height = 1080);

    EditorCamera(float posX, float posY, float posZ,
        float upX, float upY, float upZ,
        float yaw, float pitch,
        int width, int height);



    void Update();
    void OnMouseMoved(double xPosIn, double yPosIn, GLFWwindow* window);
    void ProcessMouseScroll(float yoffset);

private:
    void ProcessKeyboard();
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void UpdateCameraVectors();

public:
    float Yaw{ -90.0f };
    float Pitch{ 0.0f };

    float MovementSpeed{ 3.0f };
    float SprintSpeed{ 5.0f };
    float MouseSensitivity{ 0.075f };

    float lastX{ 0.0f };
    float lastY{ 0.0f };
    bool firstMouse{ true };

    float positionSmoothness{ 25.0f };
    glm::vec3 targetPosition{};


};

}