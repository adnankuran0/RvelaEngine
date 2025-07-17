#pragma once
#include "GLFW/glfw3.h"

class Time 
{
public:
    static void Update() noexcept;

    inline static float GetDeltaTime() noexcept
    {
        return static_cast<float>(deltaTime);
    }

    inline static float GetFixedDeltaTime() noexcept
    {
        return fixedDeltaTime;
    }

    inline static bool ShouldRunFixedUpdate() noexcept
    {
        return fixedTimeAccumulator >= fixedDeltaTime;
    }

    inline static void ConsumeFixedDeltaTime() noexcept
    {
        fixedTimeAccumulator -= fixedDeltaTime;
    }

    inline static float GetFixedTimeAccumulator() noexcept
    {
        return fixedTimeAccumulator;
    }

    inline static float GetFPS() noexcept
    {
        return fps;
    }

    inline static void SetTimeScale(float scale) noexcept
    {
        timeScale = scale;
    }

    inline static float GetTimeScale() noexcept
    {
        return timeScale;
    }

    inline static float GetMaxFPS() noexcept
    {
        return maxFPS;
    }
    
    inline static void SetMaxFPS(int maxFPS) noexcept
    {
        Time::maxFPS = maxFPS;
    }

    inline static double GetCurrentTime() noexcept 
    {
        return glfwGetTime();
    }

    inline static void SetFPSUpdateTime(int fpsUpdateTimeAsMiliseconds)
    {
        fpsUpdateTime = fpsUpdateTimeAsMiliseconds;
    }

private:
    static float lastFrameTime;  ///< The time of the last frame (in seconds).
    static float deltaTime;      ///< The time difference between the current and last frame (in seconds).
    static float lastFpsUpdateTime;  ///< The last time FPS was updated (in seconds).
    static float timeScale;      ///< The scaling factor for time (e.g., for slow motion).
    static float fps;            ///< The current frames per second.
    static int maxFPS;         ///< The maximum allowed FPS (0 means unlimited).
    static float fixedDeltaTime; ///< Fixed time step used for physics updates.
    static float fixedTimeAccumulator;  ///< Accumulates time for fixed updates to synchronize with frame rate.
    static int frameCount;        ///< The number of frames since the last FPS update.
    static int fpsUpdateTime;    ///< How frequent engine updates the fps
};
