#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <glad/gl.h>
#include "Renderer/RenderPipeline.h"

namespace rv {

struct CPUSample
{
    std::string name;
    float durationMs = 0.0f;
    float avgMs = 0.0f;
    float minMs = 999999.0f;
    float maxMs = 0.0f;
    uint32_t callCount = 0;
    uint32_t depth = 0;
};

struct GPUSample
{
    std::string name;
    float durationMs = 0.0f;
    float avgMs = 0.0f;
    float minMs = 999999.0f;
    float maxMs = 0.0f;
    uint32_t callCount = 0;
};

class Profiler
{
public:
    static constexpr size_t HISTORY_SIZE = 128;

    static Profiler& Get()
    {
        static Profiler instance;
        return instance;
    }

    void BeginFrame();
    void EndFrame();

    void BeginCPUSample(const char* name);
    void EndCPUSample();

    void BeginGPUSample(const char* name);
    void EndGPUSample();

    void SetRenderStats(const RenderStats& stats) { m_RenderStats = stats; }
    [[nodiscard]] const RenderStats& GetRenderStats() const { return m_RenderStats; }

    [[nodiscard]] float GetCPUFrameTime() const { return m_CPUFrameTimeMs; }
    [[nodiscard]] float GetGPUFrameTime() const { return m_GPUFrameTimeMs; }
    [[nodiscard]] float GetFPS() const { return m_FPS; }

    [[nodiscard]] const std::vector<float>& GetCPUHistory() const { return m_CPUHistory; }
    [[nodiscard]] const std::vector<float>& GetGPUHistory() const { return m_GPUHistory; }
    [[nodiscard]] const std::vector<float>& GetFPSHistory() const { return m_FPSHistory; }
    [[nodiscard]] size_t GetHistoryOffset() const { return m_HistoryOffset; }

    [[nodiscard]] const std::vector<CPUSample>& GetCPUSamples() const { return m_DisplayedCPUSamples; }
    [[nodiscard]] const std::vector<GPUSample>& GetGPUSamples() const { return m_DisplayedGPUSamples; }

    [[nodiscard]] bool IsPaused() const { return m_IsPaused; }
    void SetPaused(bool paused) { m_IsPaused = paused; }
    void TogglePause() { m_IsPaused = !m_IsPaused; }

    void ResetStats();

    struct GPUMemoryInfo
    {
        bool isSupported = false;
        int totalMemoryMB = 0;
        int availableMemoryMB = 0;
    };
    [[nodiscard]] GPUMemoryInfo GetGPUMemoryInfo() const;

private:
    Profiler();
    ~Profiler();

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    struct InternalCPUScope
    {
        const char* name;
        std::chrono::high_resolution_clock::time_point startTime;
        uint32_t depth;
    };

    struct GPUQueryData
    {
        GLuint queries[2] = { 0, 0 };
        bool queryIssued[2] = { false, false };
        float lastDurationMs = 0.0f;
        float avgMs = 0.0f;
        float minMs = 999999.0f;
        float maxMs = 0.0f;
        uint32_t sampleCount = 0;
    };

    bool m_IsPaused = false;
    uint32_t m_FrameCounter = 0;

    std::chrono::high_resolution_clock::time_point m_FrameStartTime;
    float m_CPUFrameTimeMs = 0.0f;
    float m_GPUFrameTimeMs = 0.0f;
    float m_FPS = 0.0f;

    std::vector<float> m_CPUHistory;
    std::vector<float> m_GPUHistory;
    std::vector<float> m_FPSHistory;
    size_t m_HistoryOffset = 0;

    std::vector<InternalCPUScope> m_CPUScopeStack;
    std::vector<CPUSample> m_CurrentCPUSamples;
    std::vector<CPUSample> m_DisplayedCPUSamples;
    std::unordered_map<std::string, CPUSample> m_CPUAggregates;

    int m_CurrentBuffer = 0;
    int m_PreviousBuffer = 1;
    const char* m_ActiveGPUSample = nullptr;
    std::unordered_map<std::string, GPUQueryData> m_GPUQueries;
    std::vector<std::string> m_GPUSampleOrder;
    std::vector<GPUSample> m_DisplayedGPUSamples;

    RenderStats m_RenderStats;
};

class ScopedCPUTimer
{
public:
    explicit ScopedCPUTimer(const char* name)
    {
        Profiler::Get().BeginCPUSample(name);
    }
    ~ScopedCPUTimer()
    {
        Profiler::Get().EndCPUSample();
    }
};

class ScopedGPUTimer
{
public:
    explicit ScopedGPUTimer(const char* name)
    {
        Profiler::Get().BeginGPUSample(name);
    }
    ~ScopedGPUTimer()
    {
        Profiler::Get().EndGPUSample();
    }
};

#define RV_PROFILE_CPU(name) rv::ScopedCPUTimer __cpuProfileTimer##__LINE__(name)
#define RV_PROFILE_GPU(name) rv::ScopedGPUTimer __gpuProfileTimer##__LINE__(name)

}
