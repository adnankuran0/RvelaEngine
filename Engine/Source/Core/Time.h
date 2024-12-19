#include "GLFW/glfw3.h"
#pragma once
class Time {
private:
    static  double lastFrameTime;
    static double deltaTime;
    static double fpsUpdateTime;
    static int frameCount;
    static float fps;
    static float timeScale;

public:

    static void update(); 

    static double getDeltaTime();

    static float getFPS();

    static void setTimeScale(float scale);

    static float getTimeScale();

    static float getTime();

};
