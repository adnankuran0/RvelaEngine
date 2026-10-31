#include "rvelapch.h"
#include "Time.h"
#include "GLFW/glfw3.h"
#include <thread>
#include <chrono>
#include <algorithm>

using namespace rv;

// Static member definitions (all in seconds)
float Time::deltaTime = 0.0f;
float Time::lastFrameTime = 0.0f;
float Time::fps = 0.0f;
float Time::timeScale = 1.0f;
float Time::fixedDeltaTime = 1.0f / 60.0f; // 20ms default fixed timestep
float Time::fixedTimeAccumulator = 0.0f;
float Time::lastFpsUpdateTime = 0.0f; 
int Time::maxFPS = 0;
int Time::fpsUpdateTime = 10; // as milliseconds
int Time::frameCount = 0;

double Time::GetCurrentTime() noexcept
{
    return glfwGetTime();
}

void Time::Update() noexcept
{
    float currentFrameTime = static_cast<float>(glfwGetTime()) * 1000.0f;

    float frameTimeTarget = (maxFPS <= 0) ? 0.0f : (1000.0f / static_cast<float>(maxFPS));

    float timeElapsed = currentFrameTime - lastFrameTime;

    if (frameTimeTarget > 0.0f && timeElapsed < frameTimeTarget)
    {
        float remaining = frameTimeTarget - timeElapsed;
        while (remaining > 1.5f)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            currentFrameTime = static_cast<float>(glfwGetTime()) * 1000.0f;
            timeElapsed = currentFrameTime - lastFrameTime;
            remaining = frameTimeTarget - timeElapsed;
        }

        while (timeElapsed < frameTimeTarget)
        {
            std::this_thread::yield();
            currentFrameTime = static_cast<float>(glfwGetTime()) * 1000.0f;
            timeElapsed = currentFrameTime - lastFrameTime;
        }
    }

    deltaTime = (currentFrameTime - lastFrameTime) / 1000.0f;
    if (deltaTime > 0.25f)
    {
        deltaTime = 0.25f;
    }

    lastFrameTime = currentFrameTime;

    // Update FPS
    frameCount++;
    if (currentFrameTime - lastFpsUpdateTime >= static_cast<float>(fpsUpdateTime))
    {
        fps = static_cast<float>(frameCount) * (1000.0f / (currentFrameTime - lastFpsUpdateTime));
        frameCount = 0;
        lastFpsUpdateTime = currentFrameTime;
    }

    fixedTimeAccumulator += deltaTime;

    const float maxAccumulator = fixedDeltaTime * 5.0f;
    if (fixedTimeAccumulator > maxAccumulator)
    {
        fixedTimeAccumulator = maxAccumulator;
    }
}
