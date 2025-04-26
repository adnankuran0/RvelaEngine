#include "rvelapch.h"
#include "Time.h"

// Static member definitions (all in seconds)
double Time::deltaTime = 0.0;
double Time::lastFrameTime = 0.0;
double Time::fpsUpdateTime = 0.0;
int Time::frameCount = 0;
double Time::fps = 0.0f;
double Time::timeScale = 1.0f;
double Time::maxFPS = 360.0;
double Time::fixedDeltaTime = 0.02; // 20ms default fixed timestep
double Time::fixedTimeAccumulator = 0.0;

/**
 * @brief Updates time-related metrics.
 *
 * Calculates the delta time, updates FPS, and applies FPS limiting if set.
 * Time values are handled in seconds internally for consistency.
 */
void Time::Update() noexcept
{
    // Ensure GLFW is initialized
    if (!glfwGetTime()) {
        deltaTime = 0.0;
        return;
    }

    double currentFrameTime = glfwGetTime() * 1000.0;

    double frameTimeTarget = (maxFPS == 0.0) ? 0.0 : (1000.0 / maxFPS);

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
    if (currentFrameTime - fpsUpdateTime >= 10.0) 
    {
        fps = static_cast<float>(frameCount) * (1000.0 / (currentFrameTime - fpsUpdateTime));
        frameCount = 0;
        fpsUpdateTime = currentFrameTime;
    }

    fixedTimeAccumulator += deltaTime;
}






