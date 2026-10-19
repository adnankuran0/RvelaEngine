#pragma once
#include <filesystem>
#include <memory>
#include "ImGui/imgui.h"

namespace rv {

bool IsSupportedThumbnailAsset(const std::filesystem::path& path);
inline bool IsSupportedThumbnailImage(const std::filesystem::path& path) { return IsSupportedThumbnailAsset(path); }

class ThumbnailCache
{
public:
    static ThumbnailCache& Get();

    void BeginFrame();
    ImTextureID GetThumbnail(const std::filesystem::path& path);
    void EndFrame();

    void OnDirectoryChanged();
    void Reset();
    void Shutdown();

private:
    ThumbnailCache();
    ~ThumbnailCache();
    ThumbnailCache(const ThumbnailCache&) = delete;
    ThumbnailCache& operator=(const ThumbnailCache&) = delete;

    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

}
