#include "rvelapch.h"
#include "EditorCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include "Input/Input.h"
#include "Core/Time.h"

using namespace rv;

EditorCamera::EditorCamera(glm::vec3 position,
    glm::vec3 up,
    int width,
    int height)
    : targetPosition(position)
{
    UpdateCameraVectors();
}

EditorCamera::EditorCamera(float posX, float posY, float posZ,
    float upX, float upY, float upZ,
    float yaw, float pitch,
    int width, int height)
    : 
    Yaw(yaw),
    Pitch(pitch),
    targetPosition(Position)
{
    UpdateCameraVectors();
}

void EditorCamera::Update()
{
    if (Input::IsMouseButtonPressed(MouseCode::Button1))
        ProcessKeyboard();

    UpdateFrustum();
    float dt = Time::GetDeltaTime();
    Position = glm::mix(Position, targetPosition, positionSmoothness * dt);
}

void EditorCamera::ProcessKeyboard()
{
    float dt = Time::GetDeltaTime();
    float velocity = (Input::IsKeyPressed(KeyCode::LeftShift) ? SprintSpeed : MovementSpeed) * dt;

    if (Input::IsKeyPressed(KeyCode::W))
        targetPosition += Front * velocity;

    if (Input::IsKeyPressed(KeyCode::S))
        targetPosition -= Front * velocity;

    if (Input::IsKeyPressed(KeyCode::A))
        targetPosition -= Right * velocity;

    if (Input::IsKeyPressed(KeyCode::D))
        targetPosition += Right * velocity;

    if (Input::IsKeyPressed(KeyCode::Q))
        targetPosition -= WorldUp * velocity;

    if (Input::IsKeyPressed(KeyCode::E))
        targetPosition += WorldUp * velocity;
}

void EditorCamera::OnMouseMoved(double xPosIn, double yPosIn, GLFWwindow* window)
{
    if (!Input::IsMouseButtonPressed(MouseCode::Button1))
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
        return;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    float xpos = static_cast<float>(xPosIn);
    float ypos = static_cast<float>(yPosIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    ProcessMouseMovement(xoffset, yoffset);
}

void EditorCamera::ProcessMouseScroll(float yoffset)
{
    FOV -= yoffset;

    if (FOV < 1.0f)
        FOV = 1.0f;

    if (FOV > 90.0f)
        FOV = 90.0f;
}

void EditorCamera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;

        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    UpdateCameraVectors();
}

void EditorCamera::UpdateCameraVectors()
{
    glm::vec3 front;

    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
