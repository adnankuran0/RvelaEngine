#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../Core/Input/Input.h"
#include "../Core/Time.h"





// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw = -90.0f;
    float Pitch = 0.0f;
    // camera options
    float MovementSpeed = 2.5f;
    float SprintSpeed = 5.0f;
    float MouseSensitivity = 0.075f;
    float Zoom = 90.0f;

    float lastX = 0.0f;
    float lastY = 0.0f;
    bool firstMouse = true;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),int width = 1280,int height = 720) : Front(glm::vec3(0.0f, 0.0f, -1.0f))
    {
        Position = position;
        WorldUp = up;
        updateCameraVectors();
        lastX = width / 2.0f;
        lastY = height / 2.0f;
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch, int width, int height) : Front(glm::vec3(0.0f, 0.0f, -1.0f))
    { 
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
        lastX = width / 2.0f;
        lastY = height / 2.0f;
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void Update()
    {
        if (Input::IsMouseButtonPressed(MouseCode::Button1))
            ProcessKeyboard();
    }

    void onMouseMoved(double xPosIn, double yPosIn,GLFWwindow* window)
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
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        ProcessMouseMovement(xoffset, yoffset);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard()
    {
        float speed = Input::IsKeyPressed(KeyCode::LeftShift) ? SprintSpeed : MovementSpeed;
        float velocity = speed * Time::getDeltaTime();
        glm::vec3 movement(0.0f, 0.0f, 0.0f);

        if (Input::IsKeyPressed(KeyCode::W))
            movement += Front;
        if (Input::IsKeyPressed(KeyCode::S))
            movement -= Front;
        if (Input::IsKeyPressed(KeyCode::A))
            movement -= Right;
        if (Input::IsKeyPressed(KeyCode::D))
            movement += Right;

        // Normalize direction if moving diagonally
        if (glm::length(movement) > 0.0f)
            movement = glm::normalize(movement);

        // Apply the velocity based on the normalized direction
        Position += movement * velocity;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();

    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
#endif