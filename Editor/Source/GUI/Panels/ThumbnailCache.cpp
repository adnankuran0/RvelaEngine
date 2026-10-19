#include "ThumbnailCache.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>

namespace rv {

bool IsSupportedThumbnailImage(const std::filesystem::path& path)
{
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp";
}

struct ThumbnailCache::Impl
{
    struct RamEntry
    {
        std::filesystem::file_time_type lastWriteTime{};
        std::vector<uint8_t> pixels;
        uint64_t lastUsedFrame = 0;
        bool failed = false;
        bool queuedForUpload = false;
    };

    struct VramEntry
    {
        GLuint textureId = 0;
        uint64_t lastVisibleFrame = 0;
    };

    struct DecodeRequest
    {
        std::filesystem::path path;
        std::filesystem::file_time_type lastWriteTime;
    };

    struct DecodeResult
    {
        std::filesystem::path path;
        std::filesystem::file_time_type lastWriteTime;
        std::vector<uint8_t> pixels;
        bool failed = false;
    };

    std::thread m_WorkerThread;
    std::mutex m_QueueMutex;
    std::condition_variable m_QueueCv;
    bool m_StopWorker = false;
    std::vector<DecodeRequest> m_PendingRequests;
    std::unordered_set<std::string> m_InFlightPaths;
    std::vector<DecodeResult> m_CompletedResults;

    uint64_t m_FrameCounter = 0;
    std::unordered_map<std::string, RamEntry> m_RamCache;
    std::unordered_map<std::string, VramEntry> m_VramCache;
    std::vector<std::string> m_UploadQueue;

    Impl()
    {
        m_WorkerThread = std::thread(&Impl::WorkerLoop, this);
    }

    ~Impl()
    {
        Shutdown();
    }

    void Shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_StopWorker = true;
            m_PendingRequests.clear();
            m_InFlightPaths.clear();
            m_QueueCv.notify_all();
        }

        if (m_WorkerThread.joinable())
        {
            m_WorkerThread.join();
        }

        ClearVram();
        m_RamCache.clear();
        m_UploadQueue.clear();
    }

    void ClearVram()
    {
        bool hasGl = (glfwGetCurrentContext() != nullptr);
        for (auto& [k, v] : m_VramCache)
        {
            if (hasGl && v.textureId != 0)
            {
                glDeleteTextures(1, &v.textureId);
            }
        }
        m_VramCache.clear();
        for (auto& [k, r] : m_RamCache)
        {
            r.queuedForUpload = false;
        }
    }

    void OnDirectoryChanged()
    {
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_PendingRequests.clear();
            m_InFlightPaths.clear();
            m_CompletedResults.clear();
        }
        m_UploadQueue.clear();
        ClearVram();
    }

    void Reset()
    {
        OnDirectoryChanged();
        m_RamCache.clear();
    }

    void BeginFrame()
    {
        m_FrameCounter++;
        UploadPendingTextures();
    }

    ImTextureID GetThumbnail(const std::filesystem::path& path)
    {
        std::string key = path.string();

        // 1. Check VRAM cache
        auto vramIt = m_VramCache.find(key);
        if (vramIt != m_VramCache.end() && vramIt->second.textureId != 0)
        {
            vramIt->second.lastVisibleFrame = m_FrameCounter;
            return (ImTextureID)(uintptr_t)vramIt->second.textureId;
        }

        // 2. Check RAM cache
        auto ramIt = m_RamCache.find(key);
        if (ramIt != m_RamCache.end())
        {
            ramIt->second.lastUsedFrame = m_FrameCounter;
            if (ramIt->second.failed)
                return 0;

            if (!ramIt->second.queuedForUpload)
            {
                ramIt->second.queuedForUpload = true;
                m_UploadQueue.push_back(key);
            }
            return 0;
        }

        // 3. Queue for background worker decode
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            if (m_InFlightPaths.find(key) == m_InFlightPaths.end())
            {
                std::error_code ec;
                auto lwt = std::filesystem::last_write_time(path, ec);
                if (!ec)
                {
                    m_InFlightPaths.insert(key);
                    m_PendingRequests.push_back({ path, lwt });
                    m_QueueCv.notify_one();
                }
            }
        }

        return 0;
    }

    void EndFrame()
    {
        EvictUnusedVram();
        EvictUnusedRam();
    }

    void WorkerLoop()
    {
        stbi_set_flip_vertically_on_load_thread(0);

        while (true)
        {
            DecodeRequest request;
            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                m_QueueCv.wait(lock, [this]() {
                    return m_StopWorker || !m_PendingRequests.empty();
                });

                if (m_StopWorker)
                    break;

                request = std::move(m_PendingRequests.back());
                m_PendingRequests.pop_back();
            }

            DecodeResult result;
            result.path = request.path;
            result.lastWriteTime = request.lastWriteTime;

            int srcW = 0, srcH = 0, channels = 0;
            stbi_uc* rawData = stbi_load(request.path.string().c_str(), &srcW, &srcH, &channels, 4);
            if (rawData && srcW > 0 && srcH > 0)
            {
                constexpr int THUMB_SIZE = 128;
                int scaledW = THUMB_SIZE;
                int scaledH = THUMB_SIZE;
                if (srcW > srcH)
                {
                    scaledW = THUMB_SIZE;
                    scaledH = std::max(1, (srcH * THUMB_SIZE) / srcW);
                }
                else
                {
                    scaledH = THUMB_SIZE;
                    scaledW = std::max(1, (srcW * THUMB_SIZE) / srcH);
                }

                std::vector<uint8_t> resized(scaledW * scaledH * 4);
                if (srcW == scaledW && srcH == scaledH)
                {
                    std::memcpy(resized.data(), rawData, scaledW * scaledH * 4);
                }
                else
                {
                    stbir_resize_uint8_linear(
                        rawData, srcW, srcH, srcW * 4,
                        resized.data(), scaledW, scaledH, scaledW * 4,
                        STBIR_RGBA);
                }
                stbi_image_free(rawData);

                result.pixels.assign(THUMB_SIZE * THUMB_SIZE * 4, 0);
                int offsetX = (THUMB_SIZE - scaledW) / 2;
                int offsetY = (THUMB_SIZE - scaledH) / 2;

                for (int y = 0; y < scaledH; ++y)
                {
                    uint8_t* dst = result.pixels.data() + ((offsetY + y) * THUMB_SIZE + offsetX) * 4;
                    const uint8_t* src = resized.data() + (y * scaledW) * 4;
                    std::memcpy(dst, src, scaledW * 4);
                }
                result.failed = false;
            }
            else
            {
                if (rawData)
                    stbi_image_free(rawData);
                result.failed = true;
            }

            {
                std::lock_guard<std::mutex> lock(m_QueueMutex);
                m_InFlightPaths.erase(result.path.string());
                m_CompletedResults.push_back(std::move(result));
            }
        }
    }

    void UploadPendingTextures()
    {
        std::vector<DecodeResult> completed;
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            if (!m_CompletedResults.empty())
            {
                completed = std::move(m_CompletedResults);
                m_CompletedResults.clear();
            }
        }

        for (auto& item : completed)
        {
            std::string key = item.path.string();
            RamEntry entry;
            entry.lastWriteTime = item.lastWriteTime;
            entry.pixels = std::move(item.pixels);
            entry.lastUsedFrame = m_FrameCounter;
            entry.failed = item.failed;
            entry.queuedForUpload = false;

            if (!item.failed && !entry.pixels.empty())
            {
                entry.queuedForUpload = true;
                m_UploadQueue.push_back(key);
            }
            m_RamCache[key] = std::move(entry);
        }

        size_t uploadsThisFrame = 0;
        constexpr size_t MAX_UPLOADS_PER_FRAME = 6;

        while (!m_UploadQueue.empty() && uploadsThisFrame < MAX_UPLOADS_PER_FRAME)
        {
            std::string key = m_UploadQueue.back();
            m_UploadQueue.pop_back();

            auto ramIt = m_RamCache.find(key);
            if (ramIt != m_RamCache.end())
            {
                ramIt->second.queuedForUpload = false;
            }

            auto vramIt = m_VramCache.find(key);
            if (vramIt != m_VramCache.end() && vramIt->second.textureId != 0)
                continue;

            if (ramIt == m_RamCache.end() || ramIt->second.failed || ramIt->second.pixels.empty())
                continue;

            GLuint tex = 0;
            glGenTextures(1, &tex);
            if (tex != 0)
            {
                glBindTexture(GL_TEXTURE_2D, tex);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, ramIt->second.pixels.data());
                glBindTexture(GL_TEXTURE_2D, 0);

                VramEntry vEntry;
                vEntry.textureId = tex;
                vEntry.lastVisibleFrame = m_FrameCounter;
                m_VramCache[key] = vEntry;

                uploadsThisFrame++;
            }
        }
    }

    void EvictUnusedVram()
    {
        constexpr uint64_t VRAM_EVICT_FRAMES = 60;
        constexpr size_t MAX_VRAM_TEXTURES = 64;

        bool hasGl = (glfwGetCurrentContext() != nullptr);

        for (auto it = m_VramCache.begin(); it != m_VramCache.end(); )
        {
            if (m_FrameCounter > it->second.lastVisibleFrame &&
                (m_FrameCounter - it->second.lastVisibleFrame) > VRAM_EVICT_FRAMES)
            {
                if (hasGl && it->second.textureId != 0)
                {
                    glDeleteTextures(1, &it->second.textureId);
                }
                it = m_VramCache.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (m_VramCache.size() > MAX_VRAM_TEXTURES)
        {
            std::vector<std::pair<std::string, uint64_t>> candidates;
            for (const auto& [k, v] : m_VramCache)
            {
                if (v.lastVisibleFrame < m_FrameCounter)
                {
                    candidates.push_back({ k, v.lastVisibleFrame });
                }
            }

            std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
                return a.second < b.second;
            });

            for (const auto& [k, frame] : candidates)
            {
                if (m_VramCache.size() <= MAX_VRAM_TEXTURES)
                    break;

                auto it = m_VramCache.find(k);
                if (it != m_VramCache.end())
                {
                    if (hasGl && it->second.textureId != 0)
                    {
                        glDeleteTextures(1, &it->second.textureId);
                    }
                    m_VramCache.erase(it);
                }
            }
        }
    }

    void EvictUnusedRam()
    {
        constexpr size_t MAX_RAM_CACHE_ENTRIES = 500;
        if (m_RamCache.size() <= MAX_RAM_CACHE_ENTRIES)
            return;

        std::vector<std::pair<std::string, uint64_t>> candidates;
        for (const auto& [k, v] : m_RamCache)
        {
            if (m_VramCache.find(k) == m_VramCache.end())
            {
                candidates.push_back({ k, v.lastUsedFrame });
            }
        }

        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

        for (const auto& [k, frame] : candidates)
        {
            if (m_RamCache.size() <= MAX_RAM_CACHE_ENTRIES)
                break;
            m_RamCache.erase(k);
        }
    }
};

ThumbnailCache& ThumbnailCache::Get()
{
    static ThumbnailCache instance;
    return instance;
}

ThumbnailCache::ThumbnailCache()
    : m_Impl(std::make_unique<Impl>())
{
}

ThumbnailCache::~ThumbnailCache() = default;

void ThumbnailCache::BeginFrame()
{
    m_Impl->BeginFrame();
}

ImTextureID ThumbnailCache::GetThumbnail(const std::filesystem::path& path)
{
    return m_Impl->GetThumbnail(path);
}

void ThumbnailCache::EndFrame()
{
    m_Impl->EndFrame();
}

void ThumbnailCache::OnDirectoryChanged()
{
    m_Impl->OnDirectoryChanged();
}

void ThumbnailCache::Reset()
{
    m_Impl->Reset();
}

void ThumbnailCache::Shutdown()
{
    m_Impl->Shutdown();
}

}
