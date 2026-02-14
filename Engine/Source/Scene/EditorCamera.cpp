#include "rvelapch.h"
#include "EditorCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include "Input/Input.h"
#include "Core/Time.h"


EditorCamera::EditorCamera(glm::vec3 position,
    glm::vec3 up,
    int width,
    int height)
    : Position(position),
    WorldUp(up),
    width(width),
    height(height),
    targetPosition(position)
{
    UpdateCameraVectors();
}

EditorCamera::EditorCamera(float posX, float posY, float posZ,
    float upX, float upY, float upZ,
    float yaw, float pitch,
    int width, int height)
    : Position({ posX, posY, posZ }),
    WorldUp({ upX, upY, upZ }),
    Yaw(yaw),
    Pitch(pitch),
    width(width),
    height(height),
    targetPosition(Position)
{
    UpdateCameraVectors();
}

glm::mat4 EditorCamera::GetViewMatrix()
{
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 EditorCamera::GetProjectionMatrix()
{
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    return glm::perspective(glm::radians(Zoom), aspect, 0.1f, 1000.0f);
}

glm::vec3 EditorCamera::GetPosition()
{
    return Position;
}


void EditorCamera::Update()
{
    ProcessKeyboard();

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

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        ProcessMouseMovement(xoffset, yoffset);
}

void EditorCamera::ProcessMouseScroll(float yoffset)
{
    Zoom -= yoffset;

    if (Zoom < 1.0f)
        Zoom = 1.0f;

    if (Zoom > 90.0f)
        Zoom = 90.0f;
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
