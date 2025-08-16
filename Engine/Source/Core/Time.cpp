#include "rvelapch.h"
#include "Time.h"

// Static member definitions (all in seconds)
float Time::deltaTime = 0.0f;
float Time::lastFrameTime = 0.0f;
float Time::fps = 0.0f;
float Time::timeScale = 1.0f;
float Time::fixedDeltaTime = 1.0f / 60.0f; // 20ms default fixed timestep
float Time::fixedTimeAccumulator = 0.0f;
float Time::lastFpsUpdateTime = 0.0f; 
int Time::maxFPS = 360;
int Time::fpsUpdateTime = 10; // as miliseconds
int Time::frameCount = 0;

void Time::Update() noexcept
{
    double currentFrameTime = glfwGetTime() * 1000.0;

    double frameTimeTarget = (maxFPS == 0) ? 0.0 : (1000.0 / maxFPS);

    double timeElapsed = currentFrameTime - lastFrameTime;

    while (timeElapsed < frameTimeTarget)
    {
        currentFrameTime = glfwGetTime() * 1000.0;
        timeElapsed = currentFrameTime - lastFrameTime;
    }

    deltaTime = (currentFrameTime - lastFrameTime) / 1000.0;
    lastFrameTime = currentFrameTime;

    // Update FPS
    frameCount++;
    if (currentFrameTime - lastFpsUpdateTime >= fpsUpdateTime)
    {
        fps = static_cast<float>(frameCount) * (1000.0 / (currentFrameTime - lastFpsUpdateTime));
        frameCount = 0;
        lastFpsUpdateTime = currentFrameTime;
    }

    fixedTimeAccumulator += deltaTime;
}






