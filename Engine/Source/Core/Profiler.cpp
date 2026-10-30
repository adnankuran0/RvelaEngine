#include "rvelapch.h"
#include "Profiler.h"
#include <algorithm>
#include <numeric>

namespace rv {

Profiler::Profiler()
    : m_CPUHistory(HISTORY_SIZE, 0.0f)
    , m_GPUHistory(HISTORY_SIZE, 0.0f)
    , m_FPSHistory(HISTORY_SIZE, 0.0f)
{
}

Profiler::~Profiler()
{
    for (auto& [name, data] : m_GPUQueries)
    {
        if (data.queries[0] != 0)
        {
            glDeleteQueries(2, data.queries);
            data.queries[0] = 0;
            data.queries[1] = 0;
        }
    }
}

void Profiler::BeginFrame()
{
    m_FrameStartTime = std::chrono::high_resolution_clock::now();
    m_CPUScopeStack.clear();
    m_CurrentCPUSamples.clear();

    for (auto& [name, data] : m_GPUQueries)
    {
        if (data.queryIssued[m_PreviousBuffer])
        {
            GLint available = 0;
            glGetQueryObjectiv(data.queries[m_PreviousBuffer], GL_QUERY_RESULT_AVAILABLE, &available);
            if (available)
            {
                GLuint64 timeNs = 0;
                glGetQueryObjectui64v(data.queries[m_PreviousBuffer], GL_QUERY_RESULT, &timeNs);
                float durationMs = static_cast<float>(timeNs) / 1000000.0f;
                data.lastDurationMs = durationMs;
                data.sampleCount++;
                data.avgMs += (durationMs - data.avgMs) / std::min(data.sampleCount, 60u);
                data.minMs = std::min(data.minMs, durationMs);
                data.maxMs = std::max(data.maxMs, durationMs);
            }
            data.queryIssued[m_PreviousBuffer] = false;
        }
    }

    m_PreviousBuffer = m_CurrentBuffer;
    m_CurrentBuffer = 1 - m_CurrentBuffer;
}

void Profiler::EndFrame()
{
    auto frameEndTime = std::chrono::high_resolution_clock::now();
    float cpuDuration = std::chrono::duration<float, std::milli>(frameEndTime - m_FrameStartTime).count();

    if (!m_IsPaused)
    {
        m_CPUFrameTimeMs = cpuDuration;
        m_FPS = (m_CPUFrameTimeMs > 0.001f) ? (1000.0f / m_CPUFrameTimeMs) : 0.0f;

        m_DisplayedCPUSamples = m_CurrentCPUSamples;

        float totalGPU = 0.0f;
        std::vector<GPUSample> gpuSamples;
        gpuSamples.reserve(m_GPUSampleOrder.size());

        for (const auto& name : m_GPUSampleOrder)
        {
            auto it = m_GPUQueries.find(name);
            if (it != m_GPUQueries.end())
            {
                const auto& data = it->second;
                totalGPU += data.lastDurationMs;

                GPUSample sample;
                sample.name = name;
                sample.durationMs = data.lastDurationMs;
                sample.avgMs = data.avgMs;
                sample.minMs = (data.minMs == 999999.0f) ? 0.0f : data.minMs;
                sample.maxMs = data.maxMs;
                sample.callCount = data.sampleCount;
                gpuSamples.push_back(sample);
            }
        }

        m_GPUFrameTimeMs = totalGPU;
        m_DisplayedGPUSamples = std::move(gpuSamples);

        m_CPUHistory[m_HistoryOffset] = m_CPUFrameTimeMs;
        m_GPUHistory[m_HistoryOffset] = m_GPUFrameTimeMs;
        m_FPSHistory[m_HistoryOffset] = m_FPS;
        m_HistoryOffset = (m_HistoryOffset + 1) % HISTORY_SIZE;
    }

    m_FrameCounter++;
}

void Profiler::BeginCPUSample(const char* name)
{
    if (!name) return;

    InternalCPUScope scope;
    scope.name = name;
    scope.startTime = std::chrono::high_resolution_clock::now();
    scope.depth = static_cast<uint32_t>(m_CPUScopeStack.size());
    m_CPUScopeStack.push_back(scope);
}

void Profiler::EndCPUSample()
{
    if (m_CPUScopeStack.empty()) return;

    auto now = std::chrono::high_resolution_clock::now();
    InternalCPUScope scope = m_CPUScopeStack.back();
    m_CPUScopeStack.pop_back();

    float duration = std::chrono::duration<float, std::milli>(now - scope.startTime).count();

    auto& agg = m_CPUAggregates[scope.name];
    agg.callCount++;
    agg.avgMs += (duration - agg.avgMs) / std::min(agg.callCount, 60u);
    agg.minMs = std::min(agg.minMs, duration);
    agg.maxMs = std::max(agg.maxMs, duration);

    CPUSample sample;
    sample.name = scope.name;
    sample.durationMs = duration;
    sample.avgMs = agg.avgMs;
    sample.minMs = (agg.minMs == 999999.0f) ? 0.0f : agg.minMs;
    sample.maxMs = agg.maxMs;
    sample.callCount = agg.callCount;
    sample.depth = scope.depth;

    m_CurrentCPUSamples.push_back(sample);
}

void Profiler::BeginGPUSample(const char* name)
{
    if (!name || m_ActiveGPUSample != nullptr) return;

    auto it = m_GPUQueries.find(name);
    if (it == m_GPUQueries.end())
    {
        GPUQueryData data;
        glGenQueries(2, data.queries);
        m_GPUQueries[name] = data;
        m_GPUSampleOrder.push_back(name);
        it = m_GPUQueries.find(name);
    }

    glBeginQuery(GL_TIME_ELAPSED, it->second.queries[m_CurrentBuffer]);
    it->second.queryIssued[m_CurrentBuffer] = true;
    m_ActiveGPUSample = name;
}

void Profiler::EndGPUSample()
{
    if (m_ActiveGPUSample == nullptr) return;

    glEndQuery(GL_TIME_ELAPSED);
    m_ActiveGPUSample = nullptr;
}

void Profiler::ResetStats()
{
    m_CPUAggregates.clear();
    for (auto& [name, data] : m_GPUQueries)
    {
        data.avgMs = 0.0f;
        data.minMs = 999999.0f;
        data.maxMs = 0.0f;
        data.sampleCount = 0;
    }

    std::fill(m_CPUHistory.begin(), m_CPUHistory.end(), 0.0f);
    std::fill(m_GPUHistory.begin(), m_GPUHistory.end(), 0.0f);
    std::fill(m_FPSHistory.begin(), m_FPSHistory.end(), 0.0f);
    m_HistoryOffset = 0;
}

Profiler::GPUMemoryInfo Profiler::GetGPUMemoryInfo() const
{
    GPUMemoryInfo info;

    GLint totalKb = 0;
    GLint availKb = 0;

    glGetIntegerv(0x9048, &totalKb);
    if (glGetError() == GL_NO_ERROR && totalKb > 0)
    {
        glGetIntegerv(0x9049, &availKb);
        info.isSupported = true;
        info.totalMemoryMB = totalKb / 1024;
        info.availableMemoryMB = availKb / 1024;
        return info;
    }

    info.isSupported = false;
    return info;
}

}
