#pragma once
#include "Asset/Asset.h"
#include "Animation/AnimationClip.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace rv {

class AnimationLibraryAsset : public Asset
{
public:
    explicit AnimationLibraryAsset(AssetUUID uuid) : Asset(uuid) {}

    std::shared_ptr<Animation::AnimationClip> GetClip(const std::string& name) const
    {
        auto it = m_Clips.find(name);
        return (it != m_Clips.end()) ? it->second : nullptr;
    }

    void AddClip(const std::shared_ptr<Animation::AnimationClip>& clip)
    {
        if (clip)
            m_Clips[clip->name] = clip;
    }

    void RemoveClip(const std::string& name)
    {
        m_Clips.erase(name);
    }

    bool HasClip(const std::string& name) const
    {
        return m_Clips.find(name) != m_Clips.end();
    }

    const std::unordered_map<std::string, std::shared_ptr<Animation::AnimationClip>>& GetClips() const
    {
        return m_Clips;
    }

    bool IsValid() const { return !m_Clips.empty(); }

private:
    friend class AnimationLibraryLoader;
    friend class AnimationLibrarySerializer;

    std::unordered_map<std::string, std::shared_ptr<Animation::AnimationClip>> m_Clips;
};

}