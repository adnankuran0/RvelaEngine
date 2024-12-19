#include "Time.h"

double Time::deltaTime = 0.0;
double Time::lastFrameTime = 0.0;
double Time::fpsUpdateTime = 0.0;
int Time::frameCount = 0;
float Time::fps = 0.0f;
float Time::timeScale = 1.0f;

void Time::update()
{
    double currentFrameTime = glfwGetTime();
    deltaTime = (currentFrameTime - lastFrameTime) * timeScale;
    lastFrameTime = currentFrameTime;

    frameCount++;
    if (currentFrameTime - fpsUpdateTime >= 0.1) {
        fps = frameCount / static_cast<float>(currentFrameTime - fpsUpdateTime);
        frameCount = 0;
        fpsUpdateTime = currentFrameTime;
    }
}

double Time::getDeltaTime()
{
    return deltaTime;
}

float Time::getFPS()
{
    return fps;
}

void Time::setTimeScale(float scale)
{
    timeScale = scale;
}

float Time::getTimeScale()
{
    return timeScale;
}

float Time::getTime()
{
    return static_cast<float>(glfwGetTime());
}