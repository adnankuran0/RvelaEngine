#include "AnimatorPanel.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/ImSequencer.h"
#include "Core/Engine.h"
#include "Scene/Components/AnimatorComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Animation/EaseType.h"
#include "AssetImporters/AnimationLibrarySerializer.h"
#include "Asset/AssetManager.h"
#include "EditorUtils.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace rv {

static const float SEQUENCE_FPS = 60.0f;

static int g_TimeMode = 0;
static int g_SnapFrames = 1;
static float g_SnapSeconds = 0.1f;
static bool g_SnapKeyframes = false;
static bool g_SnapCursor = false;

struct SelectedKey {
    int track;
    float time;
    bool operator==(const SelectedKey& o) const {
        return track == o.track && std::abs(time - o.time) < 0.001f;
    }
};
static std::vector<SelectedKey> g_SelectedKeys;

struct DragGhost {
    int track;
    float origTime;
    glm::vec3 v3;
    glm::quat q;
    Animation::EaseType ease;
    std::string eventName;
    std::string eventParam;
};
static std::vector<DragGhost> g_DragGhosts;
static bool g_IsDragging = false;
static float g_DragStartMouseTime = 0.0f;

struct ClipboardKey {
    int track;
    float timeOffset;
    glm::vec3 v3;
    glm::quat q;
    Animation::EaseType ease;
    std::string eventName;
    std::string eventParam;
};
static std::vector<ClipboardKey> g_Clipboard;

static bool g_IsBoxSelecting = false;
static ImVec2 g_BoxSelectStart;

static const char* k_EaseNames[] = { "Linear", "Ease In", "Ease Out", "Ease In-Out", "Ease Out-In", "Zero" };

static bool IsKeySelected(int track, float time) {
    for (const auto& k : g_SelectedKeys) {
        if (k.track == track && std::abs(k.time - time) < 0.001f)
            return true;
    }
    return false;
}

struct AnimationSequenceAdapter : public ImSequencer::SequenceInterface
{
    std::shared_ptr<Animation::AnimationClip> clip;
    AnimatorComponent* animator;
    entt::registry* registry;
    entt::entity entity;
    int frameMin = 0;

    AnimationSequenceAdapter(std::shared_ptr<Animation::AnimationClip> c, AnimatorComponent* anim, entt::registry* reg, entt::entity ent)
        : clip(c), animator(anim), registry(reg), entity(ent) {
    }

    virtual int GetFrameMin() const override { return frameMin; }
    virtual int GetFrameMax() const override
    {
        int maxF = static_cast<int>(clip->duration * SEQUENCE_FPS);
        return maxF > 60 ? maxF : 60;
    }

    virtual int GetItemCount() const override { return 4; }
    virtual int GetItemTypeCount() const override { return 0; }
    virtual const char* GetItemTypeName(int typeIndex) const override { return ""; }
    virtual const char* GetItemLabel(int index) const override {
        if (index == 0) return "Events";
        if (index == 1) return "Position";
        if (index == 2) return "Rotation";
        if (index == 3) return "Scale";
        return "";
    }

    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color) override
    {
        static int s = 0;
        static int e = 0;
        s = 0; e = GetFrameMax();
        if (start) *start = &s;
        if (end) *end = &e;
        if (type) *type = 0;
        if (color) {
            if (index == 0) *color = 0xFF33AAEE;
            if (index == 1) *color = 0xFF444499;
            if (index == 2) *color = 0xFF449944;
            if (index == 3) *color = 0xFF994444;
        }
    }

    virtual void Add(int type) override {}
    virtual void Del(int index) override {}
    virtual void Duplicate(int index) override {}
    virtual void Copy() override {}
    virtual void Paste() override {}

    virtual size_t GetCustomHeight(int index) override { return 0; }
    virtual void DoubleClick(int index) override {}
    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect) override {}

    float TimeToX(float time, const ImRect& rc) {
        float kfFrame = time * SEQUENCE_FPS;
        float totalFrames = (float)(GetFrameMax() - frameMin + 2.0f);
        float fraction = (kfFrame - frameMin + 0.5f) / totalFrames;
        return rc.Min.x + fraction * rc.GetWidth();
    }
    float XToTime(float x, const ImRect& rc) {
        float fraction = (x - rc.Min.x) / rc.GetWidth();
        float totalFrames = (float)(GetFrameMax() - frameMin + 2.0f);
        float kfFrame = fraction * totalFrames + frameMin - 0.5f;
        return kfFrame / SEQUENCE_FPS;
    }

    virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect) override
    {
        draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
        ImGuiIO& io = ImGui::GetIO();

        if (index == 0)
        {
            int hoveredIndex = -1;
            float size = 6.5f;

            for (size_t i = 0; i < clip->eventTrack.size(); ++i)
            {
                float evTime = clip->eventTrack[i].time;
                float x = TimeToX(evTime, rc);
                float y = rc.Min.y + rc.GetHeight() * 0.5f;

                ImRect kfRect(x - size * 1.5f, y - size * 1.5f, x + size * 1.5f, y + size * 1.5f);
                bool hovered = kfRect.Contains(io.MousePos);
                if (hovered) hoveredIndex = static_cast<int>(i);

                if (g_IsBoxSelecting) {
                    ImRect boxRect(ImMin(g_BoxSelectStart, io.MousePos), ImMax(g_BoxSelectStart, io.MousePos));
                    if (boxRect.Contains(ImVec2(x, y)) && !IsKeySelected(0, evTime)) {
                        g_SelectedKeys.push_back({ 0, evTime });
                    }
                }

                bool selected = IsKeySelected(0, evTime);

                ImU32 fillColor = IM_COL32(255, 204, 0, 255);
                if (selected) fillColor = IM_COL32(255, 255, 255, 255);
                else if (hovered) fillColor = IM_COL32(255, 235, 120, 255);

                ImVec2 p1(x - size, y - size);
                ImVec2 p2(x + size, y - size);
                ImVec2 p3(x, y + size);

                draw_list->AddTriangleFilled(p1, p2, p3, fillColor);
                draw_list->AddTriangle(p1, p2, p3, IM_COL32(180, 130, 0, 255), 1.5f);

                if (!clip->eventTrack[i].name.empty())
                {
                    char shortLabel[32];
                    snprintf(shortLabel, sizeof(shortLabel), " %s", clip->eventTrack[i].name.c_str());
                    draw_list->AddText(ImVec2(x + size * 0.8f, y - size), IM_COL32(255, 220, 80, 220), shortLabel);
                }
            }

            if (g_IsDragging)
            {
                float dragDelta = XToTime(io.MousePos.x, rc) - g_DragStartMouseTime;
                float step = (g_TimeMode == 0) ? ((float)g_SnapFrames / SEQUENCE_FPS) : g_SnapSeconds;

                for (const auto& ghost : g_DragGhosts)
                {
                    if (ghost.track != 0) continue;
                    float newTime = ghost.origTime + dragDelta;
                    if (g_SnapKeyframes && step > 0.0f) newTime = std::round(newTime / step) * step;
                    newTime = std::clamp(newTime, 0.0f, clip->duration);

                    float x = TimeToX(newTime, rc);
                    float y = rc.Min.y + rc.GetHeight() * 0.5f;

                    ImVec2 p1(x - size, y - size);
                    ImVec2 p2(x + size, y - size);
                    ImVec2 p3(x, y + size);
                    draw_list->AddTriangleFilled(p1, p2, p3, IM_COL32(255, 170, 0, 200));
                    draw_list->AddTriangle(p1, p2, p3, IM_COL32(255, 255, 255, 220), 1.5f);
                }
            }

            bool uiCapturingMouse = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);

            if (rc.Contains(io.MousePos) && !g_IsDragging && !uiCapturingMouse)
            {
                if (io.MouseClicked[1] && hoveredIndex != -1)
                {
                    float clickedTime = clip->eventTrack[hoveredIndex].time;
                    if (!IsKeySelected(0, clickedTime)) {
                        g_SelectedKeys.clear();
                        g_SelectedKeys.push_back({ 0, clickedTime });
                    }
                    ImGui::OpenPopup("EventEditPopup");
                }

                if (io.MouseDoubleClicked[0])
                {
                    if (hoveredIndex == -1)
                    {
                        float newTime = std::clamp(XToTime(io.MousePos.x, rc), 0.0f, clip->duration);
                        clip->AddEvent(newTime, "NewEvent", "");
                        g_SelectedKeys.clear();
                        g_SelectedKeys.push_back({ 0, newTime });
                    }
                    else
                    {
                        ImGui::OpenPopup("EventEditPopup");
                    }
                }
                else if (io.MouseClicked[0])
                {
                    if (hoveredIndex != -1)
                    {
                        float clickedTime = clip->eventTrack[hoveredIndex].time;
                        if (io.KeyShift || io.KeyCtrl)
                        {
                            if (IsKeySelected(0, clickedTime)) {
                                g_SelectedKeys.erase(std::remove(g_SelectedKeys.begin(), g_SelectedKeys.end(), SelectedKey{ 0, clickedTime }), g_SelectedKeys.end());
                            }
                            else {
                                g_SelectedKeys.push_back({ 0, clickedTime });
                            }
                        }
                        else
                        {
                            if (!IsKeySelected(0, clickedTime)) {
                                g_SelectedKeys.clear();
                                g_SelectedKeys.push_back({ 0, clickedTime });
                            }
                        }
                    }
                    else
                    {
                        if (!io.KeyShift && !io.KeyCtrl) g_SelectedKeys.clear();
                        g_IsBoxSelecting = true;
                        g_BoxSelectStart = io.MousePos;
                    }
                }

                if (io.MouseDownDuration[0] > 0.05f && hoveredIndex != -1 && !g_IsBoxSelecting)
                {
                    float clickedTime = clip->eventTrack[hoveredIndex].time;
                    if (IsKeySelected(0, clickedTime))
                    {
                        g_IsDragging = true;
                        g_DragStartMouseTime = XToTime(io.MousePos.x, rc);
                        g_DragGhosts.clear();

                        std::vector<int> toErase;
                        for (const auto& sk : g_SelectedKeys) {
                            if (sk.track == 0) {
                                for (size_t k = 0; k < clip->eventTrack.size(); ++k) {
                                    if (std::abs(clip->eventTrack[k].time - sk.time) < 0.001f) {
                                        DragGhost g;
                                        g.track = 0;
                                        g.origTime = clip->eventTrack[k].time;
                                        g.eventName = clip->eventTrack[k].name;
                                        g.eventParam = clip->eventTrack[k].parameter;
                                        g_DragGhosts.push_back(g);
                                        toErase.push_back((int)k);
                                        break;
                                    }
                                }
                            }
                        }
                        std::sort(toErase.rbegin(), toErase.rend());
                        for (int idx : toErase) clip->eventTrack.erase(clip->eventTrack.begin() + idx);
                        g_SelectedKeys.clear();
                    }
                }
            }
        }
        else
        {
            auto drawAndHandleKeyframes = [&](auto& track, ImU32 outlineColor, int trackIdx)
                {
                    int hoveredIndex = -1;
                    float size = 5.0f;

                    for (size_t i = 0; i < track.keyframes.size(); ++i)
                    {
                        float kfTime = track.keyframes[i].time;
                        float x = TimeToX(kfTime, rc);
                        float y = rc.Min.y + rc.GetHeight() * 0.5f;

                        ImRect kfRect(x - size * 1.5f, y - size * 1.5f, x + size * 1.5f, y + size * 1.5f);
                        bool hovered = kfRect.Contains(io.MousePos);
                        if (hovered) hoveredIndex = static_cast<int>(i);

                        if (g_IsBoxSelecting) {
                            ImRect boxRect(ImMin(g_BoxSelectStart, io.MousePos), ImMax(g_BoxSelectStart, io.MousePos));
                            if (boxRect.Contains(ImVec2(x, y)) && !IsKeySelected(trackIdx, kfTime)) {
                                g_SelectedKeys.push_back({ trackIdx, kfTime });
                            }
                        }

                        bool selected = IsKeySelected(trackIdx, kfTime);
                        ImU32 fillColor = IM_COL32(255, 255, 255, 255);

                        if (selected) fillColor = IM_COL32(0, 255, 255, 255);
                        else if (hovered) fillColor = IM_COL32(255, 255, 150, 255);

                        ImVec2 p1(x, y - size), p2(x + size, y), p3(x, y + size), p4(x - size, y);
                        draw_list->AddQuadFilled(p1, p2, p3, p4, fillColor);
                        draw_list->AddQuad(p1, p2, p3, p4, outlineColor, 1.5f);
                    }

                    if (g_IsDragging)
                    {
                        float dragDelta = XToTime(io.MousePos.x, rc) - g_DragStartMouseTime;
                        float step = (g_TimeMode == 0) ? ((float)g_SnapFrames / SEQUENCE_FPS) : g_SnapSeconds;

                        for (const auto& ghost : g_DragGhosts)
                        {
                            if (ghost.track != trackIdx) continue;

                            float newTime = ghost.origTime + dragDelta;
                            if (g_SnapKeyframes && step > 0.0f) newTime = std::round(newTime / step) * step;
                            newTime = std::clamp(newTime, 0.0f, clip->duration);

                            float x = TimeToX(newTime, rc);
                            float y = rc.Min.y + rc.GetHeight() * 0.5f;

                            ImVec2 p1(x, y - size), p2(x + size, y), p3(x, y + size), p4(x - size, y);
                            draw_list->AddQuadFilled(p1, p2, p3, p4, IM_COL32(255, 200, 50, 255));
                            draw_list->AddQuad(p1, p2, p3, p4, IM_COL32(255, 255, 255, 200), 1.5f);
                        }
                    }

                    bool uiCapturingMouse = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);

                    if (rc.Contains(io.MousePos) && !g_IsDragging && !uiCapturingMouse)
                    {
                        if (io.MouseClicked[1] && hoveredIndex != -1)
                        {
                            float clickedTime = track.keyframes[hoveredIndex].time;
                            if (!IsKeySelected(trackIdx, clickedTime)) {
                                g_SelectedKeys.clear();
                                g_SelectedKeys.push_back({ trackIdx, clickedTime });
                            }
                            ImGui::OpenPopup("KeyframeContextMenu");
                        }

                        if (io.MouseClicked[0])
                        {
                            if (hoveredIndex != -1)
                            {
                                float clickedTime = track.keyframes[hoveredIndex].time;
                                if (io.KeyShift || io.KeyCtrl)
                                {
                                    if (IsKeySelected(trackIdx, clickedTime)) {
                                        g_SelectedKeys.erase(std::remove(g_SelectedKeys.begin(), g_SelectedKeys.end(), SelectedKey{ trackIdx, clickedTime }), g_SelectedKeys.end());
                                    }
                                    else {
                                        g_SelectedKeys.push_back({ trackIdx, clickedTime });
                                    }
                                }
                                else
                                {
                                    if (!IsKeySelected(trackIdx, clickedTime)) {
                                        g_SelectedKeys.clear();
                                        g_SelectedKeys.push_back({ trackIdx, clickedTime });
                                    }
                                }
                            }
                            else
                            {
                                if (!io.KeyShift && !io.KeyCtrl) g_SelectedKeys.clear();
                                g_IsBoxSelecting = true;
                                g_BoxSelectStart = io.MousePos;
                            }
                        }

                        if (io.MouseDownDuration[0] > 0.05f && hoveredIndex != -1 && !g_IsBoxSelecting)
                        {
                            float clickedTime = track.keyframes[hoveredIndex].time;
                            if (IsKeySelected(trackIdx, clickedTime))
                            {
                                g_IsDragging = true;
                                g_DragStartMouseTime = XToTime(io.MousePos.x, rc);
                                g_DragGhosts.clear();

                                auto extractGhosts = [&](auto& trk, int tIdx, auto assignFunc) {
                                    std::vector<int> toErase;
                                    for (const auto& sk : g_SelectedKeys) {
                                        if (sk.track == tIdx) {
                                            for (size_t k = 0; k < trk.keyframes.size(); ++k) {
                                                if (std::abs(trk.keyframes[k].time - sk.time) < 0.001f) {
                                                    DragGhost g;
                                                    g.track = tIdx;
                                                    g.origTime = trk.keyframes[k].time;
                                                    g.ease = trk.keyframes[k].ease;
                                                    assignFunc(g, trk.keyframes[k].value);
                                                    g_DragGhosts.push_back(g);
                                                    toErase.push_back((int)k);
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    std::sort(toErase.rbegin(), toErase.rend());
                                    for (int idx : toErase) trk.keyframes.erase(trk.keyframes.begin() + idx);
                                    };

                                extractGhosts(clip->positionTrack, 1, [](DragGhost& g, const glm::vec3& val) { g.v3 = val; });
                                extractGhosts(clip->rotationTrack, 2, [](DragGhost& g, const glm::quat& val) { g.q = val; });
                                extractGhosts(clip->scaleTrack, 3, [](DragGhost& g, const glm::vec3& val) { g.v3 = val; });

                                g_SelectedKeys.clear();
                            }
                        }
                    }
                };

            if (index == 1) drawAndHandleKeyframes(clip->positionTrack, IM_COL32(255, 50, 50, 255), 1);
            if (index == 2) drawAndHandleKeyframes(clip->rotationTrack, IM_COL32(50, 255, 50, 255), 2);
            if (index == 3) drawAndHandleKeyframes(clip->scaleTrack, IM_COL32(50, 100, 255, 255), 3);
        }

        if (g_IsDragging && io.MouseReleased[0])
        {
            float dragDelta = XToTime(io.MousePos.x, rc) - g_DragStartMouseTime;
            float step = (g_TimeMode == 0) ? ((float)g_SnapFrames / SEQUENCE_FPS) : g_SnapSeconds;

            for (const auto& ghost : g_DragGhosts)
            {
                float newTime = ghost.origTime + dragDelta;
                if (g_SnapKeyframes && step > 0.0f) newTime = std::round(newTime / step) * step;
                newTime = std::clamp(newTime, 0.0f, clip->duration);

                if (ghost.track == 0) clip->AddEvent(newTime, ghost.eventName, ghost.eventParam);
                else if (ghost.track == 1) clip->positionTrack.AddKeyframe(newTime, ghost.v3, ghost.ease);
                else if (ghost.track == 2) clip->rotationTrack.AddKeyframe(newTime, ghost.q, ghost.ease);
                else if (ghost.track == 3) clip->scaleTrack.AddKeyframe(newTime, ghost.v3, ghost.ease);

                g_SelectedKeys.push_back({ ghost.track, newTime });
            }

            g_DragGhosts.clear();
            g_IsDragging = false;
        }

        draw_list->PopClipRect();
    }
};

void AnimatorPanel::Draw(Engine* engine, entt::entity& selectedEntity)
{
    ImGui::Begin("Animator");

    auto& registry = engine->GetActiveScene().GetRegistry();

    if (selectedEntity == entt::null || !registry.valid(selectedEntity))
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y * 0.45f));
        const char* msg = "Select an entity to animate.";
        float textWidth = ImGui::CalcTextSize(msg).x;
        ImGui::SetCursorPosX((size.x - textWidth) * 0.5f);
        ImGui::TextDisabled("%s", msg);
        ImGui::End();
        return;
    }

    if (!registry.any_of<AnimatorComponent>(selectedEntity))
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y * 0.4f));
        const char* msg = "Selected entity does not have an AnimatorComponent.";
        float textWidth = ImGui::CalcTextSize(msg).x;
        ImGui::SetCursorPosX((size.x - textWidth) * 0.5f);
        ImGui::TextDisabled("%s", msg);

        ImGui::Spacing();
        ImGui::SetCursorPosX((size.x - 140.0f) * 0.5f);
        if (ImGui::Button("Add Animator", ImVec2(140.0f, 26.0f)))
            registry.emplace<AnimatorComponent>(selectedEntity);

        ImGui::End();
        return;
    }

    auto& animator = registry.get<AnimatorComponent>(selectedEntity);

    if (!animator.library || !animator.libraryUUID.IsValid())
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y * 0.35f));

        const char* msg = "No Animation Library assigned to this entity.";
        float textWidth = ImGui::CalcTextSize(msg).x;
        ImGui::SetCursorPosX((size.x - textWidth) * 0.5f);
        ImGui::TextDisabled("%s", msg);

        const char* subMsg = "Assign an Animation Library in the Inspector or drop one below:";
        float subTextWidth = ImGui::CalcTextSize(subMsg).x;
        ImGui::SetCursorPosX((size.x - subTextWidth) * 0.5f);
        ImGui::TextDisabled("%s", subMsg);

        ImGui::Spacing();
        float dropSlotWidth = 320.0f;
        ImGui::SetCursorPosX((size.x - dropSlotWidth) * 0.5f);
        ImGui::Button("Drop .ranimlib asset here", ImVec2(dropSlotWidth, 32.0f));

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                std::string pathStr((const char*)payload->Data);
                if (pathStr.ends_with(".ranimlib") || pathStr.ends_with(".ranim"))
                {
                    AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                    if (uuid.IsValid())
                    {
                        animator.SetLibrary(uuid);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::End();
        return;
    }

    if (animator.library->GetClips().empty())
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y * 0.4f));

        const char* msg = "Animation Library contains no clips.";
        float textWidth = ImGui::CalcTextSize(msg).x;
        ImGui::SetCursorPosX((size.x - textWidth) * 0.5f);
        ImGui::TextDisabled("%s", msg);

        ImGui::Spacing();
        ImGui::SetCursorPosX((size.x - 140.0f) * 0.5f);
        if (ImGui::Button("Create Default Clip", ImVec2(140.0f, 26.0f)))
        {
            auto firstClip = std::make_shared<Animation::AnimationClip>("Default");
            firstClip->duration = 2.0f;
            animator.library->AddClip(firstClip);
            animator.SetClip("Default");
        }

        ImGui::End();
        return;
    }

    if (!animator.currentClip)
    {
        animator.SetClip(animator.library->GetClips().begin()->first);
    }

    auto& clip = animator.currentClip;
    ImGuiIO& io = ImGui::GetIO();

    auto ApplyCurrentTimeToTransform = [&]() {
        if (registry.any_of<TransformComponent>(selectedEntity)) {
            auto& tc = registry.get<TransformComponent>(selectedEntity);
            if (!clip->positionTrack.keyframes.empty()) tc.SetPosition(clip->positionTrack.Sample(animator.currentTime));
            if (!clip->rotationTrack.keyframes.empty()) tc.SetRotation(clip->rotationTrack.Sample(animator.currentTime));
            if (!clip->scaleTrack.keyframes.empty()) tc.SetScale(clip->scaleTrack.Sample(animator.currentTime));
            tc.SetDirty();
        }
        };

    auto SetSelectedEase = [&](Animation::EaseType newEase) {
        for (const auto& sk : g_SelectedKeys)
        {
            auto applyToTrack = [&](auto& track) {
                for (auto& kf : track.keyframes) {
                    if (std::abs(kf.time - sk.time) < 0.001f) {
                        kf.ease = newEase;
                        break;
                    }
                }
                };

            if (sk.track == 1) applyToTrack(clip->positionTrack);
            else if (sk.track == 2) applyToTrack(clip->rotationTrack);
            else if (sk.track == 3) applyToTrack(clip->scaleTrack);
        }

        ApplyCurrentTimeToTransform();
        };

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));

    if (ImGui::Button("Save Lib"))
    {
        auto path = AssetManager::Get().GetRegistry().GetPath(animator.libraryUUID);
        if (!path.empty())
        {
            AnimationLibrarySerializer::Save(animator.library, path);
        }
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(140.0f);
    std::string comboPreview = animator.currentClipName.empty() ? (clip ? clip->name : "Select Clip") : animator.currentClipName;
    if (ImGui::BeginCombo("##ClipSelector", comboPreview.c_str()))
    {
        for (const auto& [name, c] : animator.library->GetClips())
        {
            bool isSelected = (animator.currentClipName == name);
            if (ImGui::Selectable(name.c_str(), isSelected))
            {
                animator.SetClip(name);
                g_SelectedKeys.clear();
                ApplyCurrentTimeToTransform();
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("+ Clip"))
        ImGui::OpenPopup("NewClipModal");

    if (ImGui::BeginPopup("NewClipModal"))
    {
        static char newClipNameBuf[64] = "NewClip";
        ImGui::InputText("Clip Name", newClipNameBuf, sizeof(newClipNameBuf));
        if (ImGui::Button("Create", ImVec2(100, 0)))
        {
            auto newClip = std::make_shared<Animation::AnimationClip>(newClipNameBuf);
            newClip->duration = 2.0f;
            animator.library->AddClip(newClip);
            animator.SetClip(newClipNameBuf);
            g_SelectedKeys.clear();
            ApplyCurrentTimeToTransform();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Edit Clip") && clip)
        ImGui::OpenPopup("EditClipPopup");

    if (ImGui::BeginPopup("EditClipPopup"))
    {
        ImGui::TextDisabled("Rename Clip");
        static char renameBuf[64] = "";
        static std::string lastEditingClip = "";

        if (lastEditingClip != animator.currentClipName) {
            snprintf(renameBuf, sizeof(renameBuf), "%s", animator.currentClipName.c_str());
            lastEditingClip = animator.currentClipName;
        }

        ImGui::InputText("##RenameClip", renameBuf, sizeof(renameBuf));
        ImGui::SameLine();
        if (ImGui::Button("Apply"))
        {
            std::string newName = renameBuf;
            if (!newName.empty() && newName != animator.currentClipName)
            {
                std::string oldName = animator.currentClipName;

                clip->name = newName;
                animator.library->AddClip(clip);

                auto& clipsMap = const_cast<std::unordered_map<std::string, std::shared_ptr<Animation::AnimationClip>>&>(animator.library->GetClips());
                clipsMap.erase(oldName);

                animator.SetClip(newName);
                lastEditingClip = newName;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Separator();

        ImGui::TextDisabled("Duplicate Clip");
        if (ImGui::Button("Duplicate", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            std::string dupName = animator.currentClipName + "_Copy";
            int counter = 1;
            while (animator.library->GetClips().find(dupName) != animator.library->GetClips().end())
            {
                dupName = animator.currentClipName + "_Copy" + std::to_string(counter++);
            }

            auto duplicatedClip = std::make_shared<Animation::AnimationClip>(*clip);
            duplicatedClip->name = dupName;

            animator.library->AddClip(duplicatedClip);
            animator.SetClip(dupName);

            g_SelectedKeys.clear();
            ApplyCurrentTimeToTransform();
            ImGui::CloseCurrentPopup();
        }

        ImGui::Separator();

        ImGui::TextDisabled("Delete Clip");
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.30f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("Delete", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            auto& clipsMap = const_cast<std::unordered_map<std::string, std::shared_ptr<Animation::AnimationClip>>&>(animator.library->GetClips());
            clipsMap.erase(animator.currentClipName);

            if (clipsMap.empty())
            {
                animator.currentClip = nullptr;
                animator.currentClipName = "";
            }
            else
            {
                animator.SetClip(clipsMap.begin()->first);
            }

            g_SelectedKeys.clear();
            ApplyCurrentTimeToTransform();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    if (ImGui::Button(animator.isPlaying ? "|| Pause" : "> Play")) {
        if (animator.isPlaying) animator.Pause();
        else animator.Play();
    }
    ImGui::SameLine();
    if (ImGui::Button("[ ] Stop")) {
        animator.Stop();
        animator.currentTime = 0.0f;
        ApplyCurrentTimeToTransform();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Looping:");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(95.0f);
    const char* loopModes[] = { "None", "Linear", "PingPong" };
    int currentLoop = static_cast<int>(clip->loopMode);
    if (ImGui::Combo("##LoopMode", &currentLoop, loopModes, 3))
        clip->loopMode = static_cast<Animation::LoopMode>(currentLoop);

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Duration:");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(65.0f);
    if (ImGui::DragFloat("##Duration", &clip->duration, 0.1f, 0.1f, 100.0f, "%.2fs"))
        clip->duration = std::max(0.1f, clip->duration);

    float snapBlockWidth = 430.0f;
    float currentPosX = ImGui::GetCursorPosX();
    float targetPosX = ImGui::GetWindowContentRegionMax().x - snapBlockWidth - 5.0f;
    if (targetPosX > currentPosX)
        ImGui::SameLine(targetPosX);
    else
        ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Display:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::Combo("##TimeMode", &g_TimeMode, "Frames\0Seconds\0");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Snap:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    if (g_TimeMode == 0) {
        if (g_SnapFrames < 1) g_SnapFrames = 1;
        ImGui::DragInt("##SnapFrames", &g_SnapFrames, 1.0f, 1, 120, "%d f");
    }
    else {
        ImGui::DragFloat("##SnapSeconds", &g_SnapSeconds, 0.01f, 0.01f, 10.0f, "%.2fs");
    }

    ImGui::SameLine();
    ImGui::Checkbox("Keyframes##SnapKF", &g_SnapKeyframes);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snap keyframes while dragging");

    ImGui::SameLine();
    ImGui::Checkbox("Cursor##SnapCur", &g_SnapCursor);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snap timeline cursor while scrubbing");

    ImGui::PopStyleVar();
    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 0));

    ImGui::AlignTextToFramePadding();
    if (g_TimeMode == 0)
        ImGui::TextDisabled("%d f", (int)std::round(animator.currentTime * SEQUENCE_FPS));
    else
        ImGui::TextDisabled("%.2fs", animator.currentTime);

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.65f, 0.05f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.78f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.55f, 0.0f, 1.0f));
    if (ImGui::Button("+ Event"))
    {
        clip->AddEvent(animator.currentTime, "NewEvent", "");
        g_SelectedKeys.clear();
        g_SelectedKeys.push_back({ 0, animator.currentTime });
    }
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Animation Event at current playhead time");

    if (registry.any_of<TransformComponent>(selectedEntity))
    {
        ImGui::SameLine();
        auto& tc = registry.get<TransformComponent>(selectedEntity);

        auto RecordPos = [&]() {
            clip->positionTrack.AddKeyframe(animator.currentTime, tc.GetPosition());
            if (animator.currentTime > clip->duration) clip->duration = animator.currentTime;
            clip->RecalculateDuration();
            };
        auto RecordRot = [&]() {
            clip->rotationTrack.AddKeyframe(animator.currentTime, tc.GetRotation());
            if (animator.currentTime > clip->duration) clip->duration = animator.currentTime;
            clip->RecalculateDuration();
            };
        auto RecordScale = [&]() {
            clip->scaleTrack.AddKeyframe(animator.currentTime, tc.GetScale());
            if (animator.currentTime > clip->duration) clip->duration = animator.currentTime;
            clip->RecalculateDuration();
            };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.50f, 0.85f, 0.8f));
        if (ImGui::Button("Key All (K)")) {
            RecordPos(); RecordRot(); RecordScale();
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Record Position, Rotation and Scale keyframes (Hotkey: K)");

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 0.7f));
        if (ImGui::Button("+ Pos")) RecordPos();
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.70f, 0.20f, 0.7f));
        if (ImGui::Button("+ Rot")) RecordRot();
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.85f, 0.7f));
        if (ImGui::Button("+ Scale")) RecordScale();
        ImGui::PopStyleColor();

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_K))
        {
            RecordPos(); RecordRot(); RecordScale();
        }
    }

    bool hasTransformSelection = false;
    for (const auto& sk : g_SelectedKeys) {
        if (sk.track >= 1 && sk.track <= 3) { hasTransformSelection = true; break; }
    }

    if (hasTransformSelection)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Easing:");
        ImGui::SameLine();

        Animation::EaseType currentEase = Animation::EaseType::LINEAR;
        for (const auto& sk : g_SelectedKeys)
        {
            if (sk.track >= 1 && sk.track <= 3)
            {
                auto findCurrentEase = [&](const auto& track) {
                    for (const auto& kf : track.keyframes) {
                        if (std::abs(kf.time - sk.time) < 0.001f) {
                            currentEase = kf.ease;
                            break;
                        }
                    }
                    };
                if (sk.track == 1) findCurrentEase(clip->positionTrack);
                else if (sk.track == 2) findCurrentEase(clip->rotationTrack);
                else if (sk.track == 3) findCurrentEase(clip->scaleTrack);
                break;
            }
        }

        int selectedEaseInt = static_cast<int>(currentEase);

        ImGui::SetNextItemWidth(125.0f);
        if (ImGui::BeginCombo("##EasingSelector", k_EaseNames[selectedEaseInt]))
        {
            for (int i = 0; i < 6; i++)
            {
                bool isSelected = (selectedEaseInt == i);
                if (ImGui::Selectable(k_EaseNames[i], isSelected))
                {
                    SetSelectedEase(static_cast<Animation::EaseType>(i));
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    if (g_SelectedKeys.size() == 1 && g_SelectedKeys[0].track == 0)
    {
        float selectedEventTime = g_SelectedKeys[0].time;
        Animation::AnimationEvent* selectedEv = nullptr;
        for (auto& ev : clip->eventTrack) {
            if (std::abs(ev.time - selectedEventTime) < 0.001f) {
                selectedEv = &ev;
                break;
            }
        }

        if (selectedEv)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
            ImGui::TextUnformatted("Event:");
            ImGui::PopStyleColor();
            ImGui::SameLine();

            char nameBuf[64];
            snprintf(nameBuf, sizeof(nameBuf), "%s", selectedEv->name.c_str());
            ImGui::SetNextItemWidth(110.0f);
            if (ImGui::InputText("##QuickEvName", nameBuf, sizeof(nameBuf)))
            {
                selectedEv->name = nameBuf;
            }

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Param:");
            ImGui::SameLine();

            char paramBuf[64];
            snprintf(paramBuf, sizeof(paramBuf), "%s", selectedEv->parameter.c_str());
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputText("##QuickEvParam", paramBuf, sizeof(paramBuf)))
            {
                selectedEv->parameter = paramBuf;
            }
        }
    }

    ImGui::PopStyleVar(2);
    ImGui::Separator();

    if (ImGui::BeginPopup("KeyframeContextMenu"))
    {
        ImGui::TextDisabled("Set Easing (%zu selected)", g_SelectedKeys.size());
        ImGui::Separator();
        for (int i = 0; i < 6; i++)
        {
            if (ImGui::MenuItem(k_EaseNames[i]))
            {
                SetSelectedEase(static_cast<Animation::EaseType>(i));
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("EventEditPopup"))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
        ImGui::Text("Edit Animation Event");
        ImGui::PopStyleColor();
        ImGui::Separator();

        if (!g_SelectedKeys.empty() && g_SelectedKeys[0].track == 0)
        {
            float selTime = g_SelectedKeys[0].time;
            for (auto& ev : clip->eventTrack)
            {
                if (std::abs(ev.time - selTime) < 0.001f)
                {
                    char nameBuf[64];
                    snprintf(nameBuf, sizeof(nameBuf), "%s", ev.name.c_str());
                    if (ImGui::InputText("Function Name", nameBuf, sizeof(nameBuf)))
                        ev.name = nameBuf;

                    char paramBuf[128];
                    snprintf(paramBuf, sizeof(paramBuf), "%s", ev.parameter.c_str());
                    if (ImGui::InputText("Parameter", paramBuf, sizeof(paramBuf)))
                        ev.parameter = paramBuf;

                    ImGui::Spacing();
                    if (ImGui::Button("Delete Event", ImVec2(120, 0)))
                    {
                        clip->eventTrack.erase(std::remove_if(clip->eventTrack.begin(), clip->eventTrack.end(),
                            [&](const Animation::AnimationEvent& e) { return std::abs(e.time - selTime) < 0.001f; }),
                            clip->eventTrack.end());
                        g_SelectedKeys.clear();
                        ImGui::CloseCurrentPopup();
                    }
                    break;
                }
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !g_SelectedKeys.empty())
        {
            auto deleteFromTrack = [&](auto& trk, int tIdx) {
                std::vector<int> toErase;
                for (const auto& sk : g_SelectedKeys) {
                    if (sk.track == tIdx) {
                        for (size_t k = 0; k < trk.keyframes.size(); ++k) {
                            if (std::abs(trk.keyframes[k].time - sk.time) < 0.001f) {
                                toErase.push_back((int)k);
                                break;
                            }
                        }
                    }
                }
                std::sort(toErase.rbegin(), toErase.rend());
                for (int idx : toErase) trk.keyframes.erase(trk.keyframes.begin() + idx);
                };
            deleteFromTrack(clip->positionTrack, 1);
            deleteFromTrack(clip->rotationTrack, 2);
            deleteFromTrack(clip->scaleTrack, 3);

            std::vector<int> eventsToErase;
            for (const auto& sk : g_SelectedKeys) {
                if (sk.track == 0) {
                    for (size_t k = 0; k < clip->eventTrack.size(); ++k) {
                        if (std::abs(clip->eventTrack[k].time - sk.time) < 0.001f) {
                            eventsToErase.push_back((int)k);
                            break;
                        }
                    }
                }
            }
            std::sort(eventsToErase.rbegin(), eventsToErase.rend());
            for (int idx : eventsToErase) clip->eventTrack.erase(clip->eventTrack.begin() + idx);

            g_SelectedKeys.clear();
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C) && !g_SelectedKeys.empty())
        {
            g_Clipboard.clear();
            float minTime = FLT_MAX;
            for (const auto& sk : g_SelectedKeys) {
                if (sk.time < minTime) minTime = sk.time;
            }

            auto copyToBoard = [&](auto& trk, int tIdx, auto assignFunc) {
                for (const auto& sk : g_SelectedKeys) {
                    if (sk.track == tIdx) {
                        for (const auto& kf : trk.keyframes) {
                            if (std::abs(kf.time - sk.time) < 0.001f) {
                                ClipboardKey ck;
                                ck.track = tIdx;
                                ck.timeOffset = kf.time - minTime;
                                ck.ease = kf.ease;
                                assignFunc(ck, kf.value);
                                g_Clipboard.push_back(ck);
                                break;
                            }
                        }
                    }
                }
                };
            copyToBoard(clip->positionTrack, 1, [](ClipboardKey& ck, const glm::vec3& val) { ck.v3 = val; });
            copyToBoard(clip->rotationTrack, 2, [](ClipboardKey& ck, const glm::quat& val) { ck.q = val; });
            copyToBoard(clip->scaleTrack, 3, [](ClipboardKey& ck, const glm::vec3& val) { ck.v3 = val; });

            for (const auto& sk : g_SelectedKeys) {
                if (sk.track == 0) {
                    for (const auto& ev : clip->eventTrack) {
                        if (std::abs(ev.time - sk.time) < 0.001f) {
                            ClipboardKey ck;
                            ck.track = 0;
                            ck.timeOffset = ev.time - minTime;
                            ck.eventName = ev.name;
                            ck.eventParam = ev.parameter;
                            g_Clipboard.push_back(ck);
                            break;
                        }
                    }
                }
            }
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V) && !g_Clipboard.empty())
        {
            g_SelectedKeys.clear();
            float pasteTime = animator.currentTime;

            for (auto& ck : g_Clipboard)
            {
                float t = pasteTime + ck.timeOffset;
                if (ck.track == 0) clip->AddEvent(t, ck.eventName, ck.eventParam);
                if (ck.track == 1) clip->positionTrack.AddKeyframe(t, ck.v3, ck.ease);
                if (ck.track == 2) clip->rotationTrack.AddKeyframe(t, ck.q, ck.ease);
                if (ck.track == 3) clip->scaleTrack.AddKeyframe(t, ck.v3, ck.ease);

                if (t > clip->duration) clip->duration = t;
                g_SelectedKeys.push_back({ ck.track, t });
            }
            clip->RecalculateDuration();
        }
    }

    AnimationSequenceAdapter sequence(clip, &animator, &registry, selectedEntity);
    int currentFrame = static_cast<int>(animator.currentTime * SEQUENCE_FPS);
    int oldFrame = currentFrame;
    int firstFrame = 0;
    int selectedEntry = -1;
    bool expanded = true;

    ImSequencer::Sequencer(
        &sequence,
        &currentFrame,
        &expanded,
        &selectedEntry,
        &firstFrame,
        ImSequencer::SEQUENCER_EDIT_NONE | ImSequencer::SEQUENCER_CHANGE_FRAME
    );

    if (currentFrame != oldFrame && !g_IsDragging)
    {
        float newTime = static_cast<float>(currentFrame) / SEQUENCE_FPS;
        if (g_SnapCursor)
        {
            float step = (g_TimeMode == 0) ? ((float)g_SnapFrames / SEQUENCE_FPS) : g_SnapSeconds;
            if (step > 0.0f)
                newTime = std::round(newTime / step) * step;
            newTime = std::clamp(newTime, 0.0f, clip->duration);
            currentFrame = static_cast<int>(newTime * SEQUENCE_FPS);
        }
        animator.currentTime = newTime;
        animator.Pause();
        ApplyCurrentTimeToTransform();
    }

    if (g_IsBoxSelecting)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImRect box(ImMin(g_BoxSelectStart, io.MousePos), ImMax(g_BoxSelectStart, io.MousePos));
        dl->AddRectFilled(box.Min, box.Max, IM_COL32(0, 150, 255, 80));
        dl->AddRect(box.Min, box.Max, IM_COL32(0, 200, 255, 200));

        if (io.MouseReleased[0]) g_IsBoxSelecting = false;
    }

    ImGui::End();
}
}