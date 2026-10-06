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
    orbitTarget = targetPosition + Front * orbitDistance;
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
    orbitTarget = targetPosition + Front * orbitDistance;
}

void EditorCamera::Update(NavigationMode mode)
{
    if (mode != NavigationMode::None)
        ProcessKeyboard();

    const float dt = std::max(Time::GetDeltaTime(), 0.0f);
    if (mode == NavigationMode::Orbit)
        Position = targetPosition;
    else
    {
        // Keep the previous editor-camera response while preventing a long frame
        // from making glm::mix extrapolate past the target position.
        const float blend = std::clamp(positionSmoothness * dt, 0.0f, 1.0f);
        Position = glm::mix(Position, targetPosition, blend);
    }
    UpdateFrustum();
}

void EditorCamera::Focus(const glm::vec3& focusPoint, float distance)
{
    orbitTarget = focusPoint;
    orbitDistance = std::max(distance, 0.1f);
    targetPosition = focusPoint - Front * distance;
    Position = targetPosition;
}

void EditorCamera::SetOrbitTarget(const glm::vec3& focusPoint)
{
    orbitTarget = focusPoint;
    orbitDistance = std::max(glm::length(Position - focusPoint), 0.1f);
    targetPosition = Position;
}

void EditorCamera::SetDirection(const glm::vec3& direction, const glm::vec3* focusPoint)
{
    if (glm::length(direction) < 0.0001f) return;
    glm::vec3 normDir = glm::normalize(direction);

    if (focusPoint)
    {
        float dist = glm::length(Position - *focusPoint);
        if (dist < 0.1f) dist = 5.0f;
        orbitTarget = *focusPoint;
        orbitDistance = dist;
        targetPosition = *focusPoint - normDir * dist;
        Position = targetPosition;
    }

    Pitch = glm::degrees(asin(std::clamp(normDir.y, -0.999f, 0.999f)));
    Yaw = glm::degrees(atan2(normDir.z, normDir.x));
    UpdateCameraVectors();
}

void EditorCamera::ProcessKeyboard()
{
    float dt = Time::GetDeltaTime();
    float velocity = (Input::IsKeyPressed(KeyCode::LeftShift) ? MovementSpeed * 2.0f : MovementSpeed) * dt;
    glm::vec3 movement{ 0.0f };

    if (Input::IsKeyPressed(KeyCode::W))
        movement += Front * velocity;

    if (Input::IsKeyPressed(KeyCode::S))
        movement -= Front * velocity;

    if (Input::IsKeyPressed(KeyCode::A))
        movement -= Right * velocity;

    if (Input::IsKeyPressed(KeyCode::D))
        movement += Right * velocity;

    if (Input::IsKeyPressed(KeyCode::Q))
        movement -= WorldUp * velocity;

    if (Input::IsKeyPressed(KeyCode::E))
        movement += WorldUp * velocity;

    targetPosition += movement;
    orbitTarget += movement;
}

void EditorCamera::OnMouseMoved(double xPosIn, double yPosIn, GLFWwindow* window, NavigationMode mode)
{
    if (!window || mode == NavigationMode::None)
        return;

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

    if (mode == NavigationMode::Orbit)
    {
        ProcessMouseMovement(xoffset, yoffset);
        targetPosition = orbitTarget - Front * orbitDistance;
        Position = targetPosition;
    }
    else if (mode == NavigationMode::FreeLook)
        ProcessMouseMovement(xoffset, yoffset);
    else if (mode == NavigationMode::Pan)
    {
        const float panScale = std::max(orbitDistance, 0.1f) * 0.002f;
        const glm::vec3 movement = (Right * xoffset + Up * yoffset) * panScale;
        targetPosition += movement;
        orbitTarget += movement;
    }
}

void EditorCamera::BeginMouseCapture(GLFWwindow* window)
{
    if (window)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    firstMouse = true;
}

void EditorCamera::EndMouseCapture(GLFWwindow* window)
{
    if (window)
    {
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    firstMouse = true;
}

void EditorCamera::ProcessMouseScroll(float yoffset)
{
    if (ProjectionType == Projection::Orthographic)
    {
        OrthoSize -= yoffset * 0.5f;
        if (OrthoSize < 0.1f) OrthoSize = 0.1f;
        if (OrthoSize > 500.0f) OrthoSize = 500.0f;
    }
    else
    {
        MovementSpeed += yoffset;
        MovementSpeed = std::clamp(MovementSpeed, 1.0f, 100.0f);
    }
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
