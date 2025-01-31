#include "rvelapch.h"
#include "Time.h"
double Time::deltaTime = 0.0;
double Time::lastFrameTime = 0.0;
double Time::fpsUpdateTime = 0.0;
int Time::frameCount = 0;
float Time::fps = 0.0f;
float Time::timeScale = 1.0f;
double Time::maxFPS = 360.0;


void Time::update()
{
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

    frameCount++;
    if (currentFrameTime - fpsUpdateTime >= 10.0) 
    {
        fps = static_cast<float>(frameCount) * (1000.0 / (currentFrameTime - fpsUpdateTime));
        frameCount = 0;
        fpsUpdateTime = currentFrameTime;
    }
}


float Time::getDeltaTime()
{
    return static_cast<float>(deltaTime);
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

float Time::getMaxFPS()
{
    return maxFPS;
}

void Time::setMaxFPS(float maxFPS)
{
    Time::maxFPS = maxFPS;
}
