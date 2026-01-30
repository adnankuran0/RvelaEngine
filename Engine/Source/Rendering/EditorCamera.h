#pragma once

#include "GLAD/gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Input/Input.h"
#include "Core/Time.h"
#include "Camera.h"
#include "Frustum.h"

class EditorCamera 
{

public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw = -90.0f;
    float Pitch = 0.0f;

    float MovementSpeed = 5.0f;
    float SprintSpeed = 10.0f;
    float MouseSensitivity = 0.075f;
    float Zoom = 70.0f;

    float lastX = 0.0f;
    float lastY = 0.0f;
    bool firstMouse = true;

    glm::mat4 projection;

    float positionSmoothness = 20.0f;
    glm::vec3 targetPosition;

    EditorCamera(glm::vec3 position = glm::vec3(0.0f, 4.0f, 10.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), int width = 1920, int height = 1080)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f))
    {
        Position = targetPosition = position;
        WorldUp = up;
        updateCameraVectors();
        lastX = width / 2.0f;
        lastY = height / 2.0f;

        projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
    }

    EditorCamera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch, int width, int height)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f))
    {
        Position = targetPosition = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
        lastX = width / 2.0f;
        lastY = height / 2.0f;

        projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
    }

    inline glm::mat4 GetViewMatrix() noexcept
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void Update()
    {
        
        if (Input::IsMouseButtonPressed(MouseCode::Button1))
            ProcessKeyboard();

        Position = glm::mix(Position, targetPosition, Time::GetDeltaTime() * positionSmoothness);
        updateCameraVectors();
        frustum.Update(projection * GetViewMatrix());
    }

    bool Intersects(const AABB& AABB)
    {
        return frustum.Intersects(AABB);
    }

    bool Intersects(const glm::mat4& projView,const AABB& AABB)
    {
        return frustum.Intersects(projView,AABB);
    }

    void onMouseMoved(double xPosIn, double yPosIn, GLFWwindow* window)
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

    void ProcessKeyboard()
    {
        double speed = Input::IsKeyPressed(KeyCode::LeftShift) ? SprintSpeed : MovementSpeed;
        float velocity = (float)(speed * Time::GetDeltaTime());
        glm::vec3 movement(0.0f);

        if (Input::IsKeyPressed(KeyCode::W))
            movement += Front;
        if (Input::IsKeyPressed(KeyCode::S))
            movement -= Front;
        if (Input::IsKeyPressed(KeyCode::A))
            movement -= Right;
        if (Input::IsKeyPressed(KeyCode::D))
            movement += Right;

        if (glm::length(movement) > 0.0f)
            movement = glm::normalize(movement);

        targetPosition += movement * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
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

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    Frustum frustum;

    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
