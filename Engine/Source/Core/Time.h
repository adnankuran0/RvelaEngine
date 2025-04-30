#pragma once

#include "GLFW/glfw3.h"
#include <thread>
#include <chrono>

/**
 * @brief Manages time-related functionality for the game engine.
 *
 * The Time class provides a static interface to track and manage time-related metrics
 * such as delta time, frames per second (FPS), and time scaling. It also supports
 * FPS limiting and provides access to the current time since the application started.
 */
class Time {
private:
    static double lastFrameTime;  ///< The time of the last frame (in seconds).
    static double deltaTime;      ///< The time difference between the current and last frame (in seconds).
    static double fpsUpdateTime;  ///< The last time FPS was updated (in seconds).
    static int frameCount;        ///< The number of frames since the last FPS update.
    static double fps;            ///< The current frames per second.
    static double timeScale;      ///< The scaling factor for time (e.g., for slow motion).
    static double maxFPS;         ///< The maximum allowed FPS (0 means unlimited).
    static double fixedDeltaTime; ///< Fixed time step used for physics updates.
    static double fixedTimeAccumulator;  ///< Accumulates time for fixed updates to synchronize with frame rate.

public:

    static void Update() noexcept;

    inline static double GetDeltaTime() noexcept
    {
        return static_cast<float>(deltaTime);
    }

    inline static double GetFixedDeltaTime() noexcept
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

    inline static double GetFixedTimeAccumulator() noexcept
    {
        return fixedTimeAccumulator;
    }

    inline static double GetFPS() noexcept
    {
        return fps;
    }

    inline static void SetTimeScale(float scale) noexcept
    {
        timeScale = scale;
    }

    inline static double GetTimeScale() noexcept
    {
        return timeScale;
    }

    inline static double GetTime() noexcept
    {
        return static_cast<float>(glfwGetTime());
    }

    inline static double GetMaxFPS() noexcept
    {
        return maxFPS;
    }
    
    inline static void SetMaxFPS(float maxFPS) noexcept
    {
        Time::maxFPS = maxFPS;
    }

    inline static double GetCurrentTime() noexcept {
        return glfwGetTime();
    }

};
