#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/ImSequencer.h"
#include "Animation/AnimationClip.h"
#include "Animation/EaseType.h"
#include "Animation/IPropertyTrack.h"
#include "Animation/PropertyBindingRegistry.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/SceneTreeComponent.h"
#include "Scene/Components/TagComponent.h"
#include <entt/entt.h>
#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <type_traits>

namespace rv {

struct AnimatorComponent;

constexpr float SEQUENCE_FPS = 60.0f;

struct SelectedKey {
    int track;
    float time;
    bool operator==(const SelectedKey& o) const {
        return track == o.track && std::abs(time - o.time) < 0.001f;
    }
};

struct DragGhost {
    int track;
    float origTime;
    float fVal = 0.0f;
    glm::vec3 v3{ 0.0f };
    glm::vec4 v4{ 0.0f };
    glm::quat q{ 1.0f, 0.0f, 0.0f, 0.0f };
    bool bVal = false;
    Animation::EaseType ease{ Animation::EaseType::Linear };
    std::string eventName;
    std::string eventParam;
};

enum class DragMode { Translate, ScaleRight, ScaleLeft };

struct ClipboardKey {
    int track;
    float timeOffset;
    float fVal = 0.0f;
    glm::vec3 v3{ 0.0f };
    glm::vec4 v4{ 0.0f };
    glm::quat q{ 1.0f, 0.0f, 0.0f, 0.0f };
    bool bVal = false;
    Animation::EaseType ease{ Animation::EaseType::Linear };
    std::string eventName;
    std::string eventParam;
};

struct SequencerContext {
    int timeMode = 0;
    int snapFrames = 1;
    float snapSeconds = 0.1f;
    bool snapKeyframes = false;
    bool snapCursor = false;
    int selectedTrack = -1;

    entt::entity lastSelectedEntity = entt::null;
    float currentDragDelta = 0.0f;

    std::vector<SelectedKey> selectedKeys;
    std::vector<DragGhost> dragGhosts;
    std::vector<ClipboardKey> clipboard;

    bool isDragging = false;
    float dragStartMouseTime = 0.0f;
    DragMode dragMode = DragMode::Translate;
    float scaleAnchorTime = 0.0f;
    float scaleOrigSpan = 0.0f;

    bool isBoxSelecting = false;
    ImVec2 boxSelectStart{ 0.0f, 0.0f };

    bool IsKeySelected(int track, float time) const 
    {
        for (const auto& k : selectedKeys) {
            if (k.track == track && std::abs(k.time - time) < 0.001f)
                return true;
        }
        return false;
    }

    void ResetSelectionState() {
        selectedTrack = -1;
        selectedKeys.clear();
        isDragging = false;
        isBoxSelecting = false;
        dragGhosts.clear();
        currentDragDelta = 0.0f;
        dragMode = DragMode::Translate;
    }

    float CalculateNewTime(float origTime, float duration) const 
    {
        float newTime = origTime;
        if (dragMode == DragMode::ScaleRight && scaleOrigSpan > 0.0001f) {
            float newSpan = std::max(0.01f, scaleOrigSpan + currentDragDelta);
            float ratio = (origTime - scaleAnchorTime) / scaleOrigSpan;
            newTime = scaleAnchorTime + newSpan * ratio;
        }
        else if (dragMode == DragMode::ScaleLeft && scaleOrigSpan > 0.0001f) {
            float newSpan = std::max(0.01f, scaleOrigSpan - currentDragDelta);
            float ratio = (scaleAnchorTime - origTime) / scaleOrigSpan;
            newTime = scaleAnchorTime - newSpan * ratio;
        }
        else {
            newTime = origTime + currentDragDelta;
        }

        float step = (timeMode == 0) ? (static_cast<float>(snapFrames) / SEQUENCE_FPS) : snapSeconds;
        if (snapKeyframes && step > 0.0f) {
            newTime = std::round(newTime / step) * step;
        }
        return std::clamp(newTime, 0.0f, duration);
    }
};

entt::entity ResolvePathStatic(entt::registry& reg, entt::entity root, const std::string& path);

struct AnimationSequenceAdapter : public ImSequencer::SequenceInterface
{
    std::shared_ptr<Animation::AnimationClip> clip;
    AnimatorComponent* animator;
    entt::registry* registry;
    entt::entity entity;
    SequencerContext& ctx;
    int frameMin = 0;
    mutable std::vector<std::string> itemLabels;

    AnimationSequenceAdapter(std::shared_ptr<Animation::AnimationClip> c,
        AnimatorComponent* anim,
        entt::registry* reg,
        entt::entity ent,
        SequencerContext& context);

    virtual int GetFrameMin() const override { return frameMin; }
    virtual int GetFrameMax() const override;
    virtual int GetItemCount() const override;
    virtual int GetItemTypeCount() const override { return 0; }
    virtual const char* GetItemTypeName(int) const override { return ""; }
    virtual const char* GetItemLabel(int index) const override;
    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color) override;

    virtual void Add(int) override {}
    virtual void Del(int) override {}
    virtual void Duplicate(int) override {}
    virtual void Copy() override {}
    virtual void Paste() override {}
    virtual size_t GetCustomHeight(int) override { return 0; }
    virtual void DoubleClick(int) override {}
    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect) override {}

    virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect) override;

    float TimeToX(float time, const ImRect& rc) const;
    float XToTime(float x, const ImRect& rc) const;

private:
    void SetupDragMode(float clickedTime);
    void DrawEventTrack(ImDrawList* draw_list, const ImRect& rc);

    template <typename TrackType>
    void DrawTrackKeyframes(TrackType& track, ImU32 outlineColor, int trackIdx, ImDrawList* draw_list, const ImRect& rc);
};

}
