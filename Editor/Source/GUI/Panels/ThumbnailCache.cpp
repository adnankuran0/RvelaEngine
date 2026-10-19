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
#include <cmath>
#include <fstream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>
#include <nlohmann/json.hpp>
#include "Asset/AssetManager.h"
#include "Asset/AssetRegistry.h"
#include "Asset/AssetUUID.h"
#include "Utils/ProjectManager.h"

namespace rv {

bool IsSupportedThumbnailAsset(const std::filesystem::path& path)
{
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".rmat";
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

    std::mutex m_SourcePathMutex;
    std::unordered_map<std::string, std::filesystem::path> m_UuidToSourcePath;

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
        {
            std::lock_guard<std::mutex> lock(m_SourcePathMutex);
            m_UuidToSourcePath.clear();
        }
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
        {
            std::lock_guard<std::mutex> lock(m_SourcePathMutex);
            m_UuidToSourcePath.clear();
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

        // Check VRAM cache
        auto vramIt = m_VramCache.find(key);
        if (vramIt != m_VramCache.end() && vramIt->second.textureId != 0)
        {
            vramIt->second.lastVisibleFrame = m_FrameCounter;
            return (ImTextureID)(uintptr_t)vramIt->second.textureId;
        }

        // Check RAM cache
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

        // Queue for background worker decode
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

    static DecodeResult LoadImageThumbnail(const std::filesystem::path& path, std::filesystem::file_time_type lwt)
    {
        DecodeResult result;
        result.path = path;
        result.lastWriteTime = lwt;

        int srcW = 0, srcH = 0, channels = 0;
        stbi_uc* rawData = stbi_load(path.string().c_str(), &srcW, &srcH, &channels, 4);
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
        return result;
    }

    std::filesystem::path ResolveTextureSourcePath(const std::string& uuidStr)
    {
        if (uuidStr.empty())
            return {};

        {
            std::lock_guard<std::mutex> lock(m_SourcePathMutex);
            auto it = m_UuidToSourcePath.find(uuidStr);
            if (it != m_UuidToSourcePath.end() && !it->second.empty())
            {
                std::error_code ec;
                if (std::filesystem::exists(it->second, ec))
                    return it->second;
            }
        }

        //  Check asset registry directly
        auto uuid = AssetUUID::FromString(uuidStr);
        if (uuid.IsValid())
        {
            auto p = AssetManager::Get().GetRegistry().GetPath(uuid);
            if (!p.empty() && p.string().find(".cache") == std::string::npos)
            {
                std::string ext = p.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp")
                {
                    std::error_code ec;
                    if (std::filesystem::exists(p, ec))
                    {
                        std::lock_guard<std::mutex> lock(m_SourcePathMutex);
                        m_UuidToSourcePath[uuidStr] = p;
                        return p;
                    }
                }
            }
        }

        // Scan project assets directory for .rmeta files
        std::filesystem::path assetDir;
        if (ProjectManager::IsProjectLoaded())
            assetDir = ProjectManager::GetAssetDirectory();
        if (assetDir.empty())
            assetDir = AssetManager::Get().GetRegistry().GetAssetDir();

        if (!assetDir.empty())
        {
            std::error_code ec;
            if (std::filesystem::exists(assetDir, ec))
            {
                std::lock_guard<std::mutex> lock(m_SourcePathMutex);
                for (const auto& entry : std::filesystem::recursive_directory_iterator(assetDir, std::filesystem::directory_options::skip_permission_denied, ec))
                {
                    if (ec) break;
                    if (!entry.is_regular_file(ec)) continue;
                    if (entry.path().extension() == ".rmeta")
                    {
                        if (entry.path().string().find(".cache") != std::string::npos)
                            continue;

                        std::ifstream metaFile(entry.path());
                        if (!metaFile) continue;

                        nlohmann::json metaJson;
                        try {
                            metaJson = nlohmann::json::parse(metaFile, nullptr, false);
                        } catch (...) { continue; }

                        if (metaJson.is_discarded() || !metaJson.is_object()) continue;

                        auto srcPath = entry.path();
                        srcPath.replace_extension("");

                        std::string foundUuid = metaJson.value("uuid", "");
                        if (!foundUuid.empty())
                        {
                            std::error_code srcEc;
                            if (std::filesystem::exists(srcPath, srcEc))
                            {
                                m_UuidToSourcePath[foundUuid] = srcPath;
                            }
                        }

                        if (metaJson.contains("subAssets") && metaJson["subAssets"].is_array())
                        {
                            for (const auto& sub : metaJson["subAssets"])
                            {
                                if (sub.is_object())
                                {
                                    std::string subUuid = sub.value("uuid", "");
                                    if (!subUuid.empty())
                                    {
                                        std::error_code srcEc;
                                        if (std::filesystem::exists(srcPath, srcEc))
                                        {
                                            m_UuidToSourcePath[subUuid] = srcPath;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                auto it = m_UuidToSourcePath.find(uuidStr);
                if (it != m_UuidToSourcePath.end() && !it->second.empty())
                {
                    std::error_code ec2;
                    if (std::filesystem::exists(it->second, ec2))
                        return it->second;
                }
            }
        }

        return {};
    }

    struct LoadedTexture
    {
        int width = 0;
        int height = 0;
        std::vector<uint8_t> pixels;
    };

    static bool LoadTextureToRgba(const std::filesystem::path& path, int targetSize, LoadedTexture& outTex)
    {
        std::error_code ec;
        if (path.empty() || !std::filesystem::exists(path, ec))
            return false;

        int tw = 0, th = 0, tc = 0;
        stbi_uc* raw = stbi_load(path.string().c_str(), &tw, &th, &tc, 4);
        if (!raw || tw <= 0 || th <= 0)
        {
            if (raw) stbi_image_free(raw);
            return false;
        }

        outTex.width = targetSize;
        outTex.height = targetSize;
        outTex.pixels.resize(targetSize * targetSize * 4);

        stbir_resize_uint8_linear(
            raw, tw, th, tw * 4,
            outTex.pixels.data(), targetSize, targetSize, targetSize * 4,
            STBIR_RGBA);

        stbi_image_free(raw);
        return true;
    }

    DecodeResult RenderMaterialThumbnail(const std::filesystem::path& path, std::filesystem::file_time_type lwt)
    {
        DecodeResult result;
        result.path = path;
        result.lastWriteTime = lwt;

        std::ifstream file(path);
        if (!file.is_open())
        {
            result.failed = true;
            return result;
        }

        nlohmann::json j;
        try
        {
            j = nlohmann::json::parse(file, nullptr, false);
        }
        catch (...)
        {
            result.failed = true;
            return result;
        }

        if (j.is_discarded() || !j.is_object())
        {
            result.failed = true;
            return result;
        }

        float albedoR = 1.0f, albedoG = 1.0f, albedoB = 1.0f;
        if (j.contains("albedoColor") && j["albedoColor"].is_array() && j["albedoColor"].size() >= 3)
        {
            albedoR = std::clamp(j["albedoColor"][0].get<float>(), 0.0f, 1.0f);
            albedoG = std::clamp(j["albedoColor"][1].get<float>(), 0.0f, 1.0f);
            albedoB = std::clamp(j["albedoColor"][2].get<float>(), 0.0f, 1.0f);
        }

        float metallic = std::clamp(j.value("metallic", 0.0f), 0.0f, 1.0f);
        float roughness = std::clamp(j.value("roughness", 1.0f), 0.04f, 1.0f);

        float emissiveR = 0.0f, emissiveG = 0.0f, emissiveB = 0.0f;
        if (j.contains("emissiveColor") && j["emissiveColor"].is_array() && j["emissiveColor"].size() >= 3)
        {
            emissiveR = std::max(0.0f, j["emissiveColor"][0].get<float>());
            emissiveG = std::max(0.0f, j["emissiveColor"][1].get<float>());
            emissiveB = std::max(0.0f, j["emissiveColor"][2].get<float>());
        }
        float emissiveIntensity = std::max(0.0f, j.value("emissiveIntensity", 0.0f));

        bool useAlbedoMap = j.value("useAlbedoMap", false);
        std::string albedoUUIDStr = j.value("albedoTexture", "");
        LoadedTexture albedoTex;
        if (useAlbedoMap && !albedoUUIDStr.empty())
        {
            auto p = ResolveTextureSourcePath(albedoUUIDStr);
            LoadTextureToRgba(p, 128, albedoTex);
        }

        bool useNormalMap = j.value("useNormalMap", false);
        std::string normalUUIDStr = j.value("normalTexture", "");
        float normalScale = j.value("normalScale", 1.0f);
        LoadedTexture normalTex;
        if (useNormalMap && !normalUUIDStr.empty())
        {
            auto p = ResolveTextureSourcePath(normalUUIDStr);
            LoadTextureToRgba(p, 128, normalTex);
        }

        bool useRoughnessMap = j.value("useRoughnessMap", false);
        std::string roughUUIDStr = j.value("roughnessTexture", "");
        LoadedTexture roughTex;
        if (useRoughnessMap && !roughUUIDStr.empty())
        {
            auto p = ResolveTextureSourcePath(roughUUIDStr);
            LoadTextureToRgba(p, 128, roughTex);
        }

        bool useMetallicMap = j.value("useMetallicMap", false);
        std::string metalUUIDStr = j.value("metallicTexture", "");
        LoadedTexture metalTex;
        if (useMetallicMap && !metalUUIDStr.empty())
        {
            auto p = ResolveTextureSourcePath(metalUUIDStr);
            LoadTextureToRgba(p, 128, metalTex);
        }

        constexpr int THUMB_SIZE = 128;
        result.pixels.assign(THUMB_SIZE * THUMB_SIZE * 4, 0);

        const float cx = 64.0f;
        const float cy = 58.0f;
        const float radius = 48.0f;
        const float radiusSq = radius * radius;

        // Key light
        const float lx1 = 0.38f, ly1 = -0.65f, lz1 = 0.66f;
        const float l1Len = std::sqrt(lx1 * lx1 + ly1 * ly1 + lz1 * lz1);
        const float klx = lx1 / l1Len, kly = ly1 / l1Len, klz = lz1 / l1Len;

        // Fill light
        const float lx2 = -0.50f, ly2 = 0.35f, lz2 = 0.45f;
        const float l2Len = std::sqrt(lx2 * lx2 + ly2 * ly2 + lz2 * lz2);
        const float flx = lx2 / l2Len, fly = ly2 / l2Len, flz = lz2 / l2Len;

        // Half vector for key light
        const float hx1 = klx, hy1 = kly, hz1 = klz + 1.0f;
        const float h1Len = std::sqrt(hx1 * hx1 + hy1 * hy1 + hz1 * hz1);
        const float khx = hx1 / h1Len, khy = hy1 / h1Len, khz = hz1 / h1Len;

        for (int y = 0; y < THUMB_SIZE; ++y)
        {
            for (int x = 0; x < THUMB_SIZE; ++x)
            {
                const float px = x + 0.5f;
                const float py = y + 0.5f;
                const float dx = px - cx;
                const float dy = py - cy;
                const float distSq = dx * dx + dy * dy;

                uint8_t* outPixel = result.pixels.data() + (y * THUMB_SIZE + x) * 4;

                if (distSq <= radiusSq)
                {
                    const float dist = std::sqrt(distSq);
                    const float z = std::sqrt(std::max(0.0f, radiusSq - distSq));

                    float nx = dx / radius;
                    float ny = dy / radius;
                    float nz = z / radius;

                    float u = 0.5f + std::atan2(nx, nz) / 3.14159265f;
                    float v = 0.5f + std::asin(std::clamp(ny, -1.0f, 1.0f)) / 3.14159265f;
                    u = std::clamp(u, 0.0f, 0.999f);
                    v = std::clamp(v, 0.0f, 0.999f);

                    const int tx = (int)(u * 128.0f);
                    const int ty = (int)(v * 128.0f);
                    const int texIndex = (ty * 128 + tx) * 4;

                    float baseR = albedoR;
                    float baseG = albedoG;
                    float baseB = albedoB;

                    if (!albedoTex.pixels.empty())
                    {
                        const uint8_t* texP = albedoTex.pixels.data() + texIndex;
                        baseR *= (texP[0] / 255.0f);
                        baseG *= (texP[1] / 255.0f);
                        baseB *= (texP[2] / 255.0f);
                    }

                    if (!normalTex.pixels.empty())
                    {
                        const uint8_t* normP = normalTex.pixels.data() + texIndex;
                        float tnx = (normP[0] / 255.0f) * 2.0f - 1.0f;
                        float tny = (normP[1] / 255.0f) * 2.0f - 1.0f;
                        float tnz = (normP[2] / 255.0f) * 2.0f - 1.0f;

                        tnx *= normalScale;
                        tny *= normalScale;

                        const float tLen = std::sqrt(nz * nz + nx * nx);
                        if (tLen > 0.001f)
                        {
                            const float Tx = nz / tLen;
                            const float Ty = 0.0f;
                            const float Tz = -nx / tLen;

                            const float Bx = ny * Tz;
                            const float By = tLen;
                            const float Bz = -ny * Tx;

                            const float px_n = Tx * tnx + Bx * tny + nx * tnz;
                            const float py_n = Ty * tnx + By * tny + ny * tnz;
                            const float pz_n = Tz * tnx + Bz * tny + nz * tnz;

                            const float pLen = std::sqrt(px_n * px_n + py_n * py_n + pz_n * pz_n);
                            if (pLen > 0.001f)
                            {
                                nx = px_n / pLen;
                                ny = py_n / pLen;
                                nz = pz_n / pLen;
                            }
                        }
                    }

                    float rough = roughness;
                    if (!roughTex.pixels.empty())
                    {
                        const uint8_t* rP = roughTex.pixels.data() + texIndex;
                        rough *= (rP[0] / 255.0f);
                    }
                    rough = std::clamp(rough, 0.04f, 1.0f);

                    float metal = metallic;
                    if (!metalTex.pixels.empty())
                    {
                        const uint8_t* mP = metalTex.pixels.data() + texIndex;
                        metal *= (mP[0] / 255.0f);
                    }
                    metal = std::clamp(metal, 0.0f, 1.0f);

                    const float specPower = 2.0f / (rough * rough * rough * rough + 0.001f);
                    const float specNorm = std::min(2.5f, (specPower + 2.0f) / 10.0f) * (1.0f - rough * 0.4f);

                    const float nDotL1 = std::max(0.0f, nx * klx + (-ny) * (-kly) + nz * klz);
                    const float diff1 = std::pow((nDotL1 + 0.15f) / 1.15f, 1.2f) * 1.05f;

                    const float nDotL2 = std::max(0.0f, nx * flx + (-ny) * (-fly) + nz * flz);
                    const float diff2 = nDotL2 * 0.25f;

                    const float amb = 0.18f;
                    const float diffFactor = (diff1 + diff2 + amb) * (1.0f - metal * 0.85f);

                    const float nDotH1 = std::max(0.0f, nx * khx + (-ny) * (-khy) + nz * khz);
                    const float specFactor = std::pow(nDotH1, specPower) * specNorm;

                    const float specR = (1.0f - metal + baseR * metal) * specFactor;
                    const float specG = (1.0f - metal + baseG * metal) * specFactor;
                    const float specB = (1.0f - metal + baseB * metal) * specFactor;

                    const float nDotV = std::max(0.0f, nz);
                    const float fresnel = std::pow(1.0f - nDotV, 3.5f) * (0.30f + metal * 0.35f);
                    const float rimR = (0.9f * (1.0f - metal) + baseR * metal) * fresnel;
                    const float rimG = (0.95f * (1.0f - metal) + baseG * metal) * fresnel;
                    const float rimB = (1.0f * (1.0f - metal) + baseB * metal) * fresnel;

                    const float emitR = emissiveR * emissiveIntensity;
                    const float emitG = emissiveG * emissiveIntensity;
                    const float emitB = emissiveB * emissiveIntensity;

                    float finalR = baseR * diffFactor + specR + rimR + emitR;
                    float finalG = baseG * diffFactor + specG + rimG + emitG;
                    float finalB = baseB * diffFactor + specB + rimB + emitB;

                    finalR = finalR / (finalR + 0.85f) * 1.05f;
                    finalG = finalG / (finalG + 0.85f) * 1.05f;
                    finalB = finalB / (finalB + 0.85f) * 1.05f;

                    const float edgeDist = radius - dist;
                    const float alpha = std::clamp(edgeDist + 0.5f, 0.0f, 1.0f);

                    outPixel[0] = (uint8_t)(std::clamp(finalR, 0.0f, 1.0f) * 255.0f);
                    outPixel[1] = (uint8_t)(std::clamp(finalG, 0.0f, 1.0f) * 255.0f);
                    outPixel[2] = (uint8_t)(std::clamp(finalB, 0.0f, 1.0f) * 255.0f);
                    outPixel[3] = (uint8_t)(alpha * 255.0f);
                }
                else
                {
                    const float sdx = px - 64.0f;
                    const float sdy = py - 114.0f;
                    const float sdist = std::sqrt((sdx * sdx) / (44.0f * 44.0f) + (sdy * sdy) / (9.0f * 9.0f));

                    if (sdist < 1.0f)
                    {
                        const float shadowAlpha = std::pow(1.0f - sdist, 1.4f) * 0.40f;
                        outPixel[0] = 0;
                        outPixel[1] = 0;
                        outPixel[2] = 0;
                        outPixel[3] = (uint8_t)(shadowAlpha * 255.0f);
                    }
                    else
                    {
                        outPixel[0] = 0;
                        outPixel[1] = 0;
                        outPixel[2] = 0;
                        outPixel[3] = 0;
                    }
                }
            }
        }

        result.failed = false;
        return result;
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

            std::string ext = request.path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });

            DecodeResult result;
            if (ext == ".rmat")
            {
                result = RenderMaterialThumbnail(request.path, request.lastWriteTime);
            }
            else
            {
                result = LoadImageThumbnail(request.path, request.lastWriteTime);
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
