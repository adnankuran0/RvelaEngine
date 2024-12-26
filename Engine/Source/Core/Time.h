#include "GLFW/glfw3.h"
#include <thread>
#include <chrono>
#pragma once
class Time {
private:
    static  double lastFrameTime;
    static double deltaTime;
    static double fpsUpdateTime;
    static int frameCount;
    static float fps;
    static float timeScale;
    static double maxFPS;

public:

    static void update(); 

    static float getDeltaTime();

    static float getFPS();

    static void setTimeScale(float scale);

    static float getTimeScale();

    static float getTime();

    static float getMaxFPS();
    
    static void setMaxFPS(float maxFPS);

    static double getCurrentTime();

};
