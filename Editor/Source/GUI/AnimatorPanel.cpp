#include "AnimatorPanel.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/ImSequencer.h"
#include "Core/Engine.h"
#include "Scene/Components/AnimatorComponent.h"
#include "Scene/Components/TransformComponent.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace rv {

    static const float SEQUENCE_FPS = 60.0f;

    struct KeyRef {
        int track; // 0: Pos, 1: Rot, 2: Scale
        int index;
        bool operator==(const KeyRef& o) const { return track == o.track && index == o.index; }
    };
    static std::vector<KeyRef> g_SelectedKeys;

    struct DragGhost {
        int track;
        float origTime;
        glm::vec3 v3;
        glm::quat q;
    };
    static std::vector<DragGhost> g_DragGhosts;
    static bool g_IsDragging = false;
    static float g_DragStartMouseTime = 0.0f;

    struct ClipboardKey {
        int track;
        float timeOffset;
        glm::vec3 v3;
        glm::quat q;
    };
    static std::vector<ClipboardKey> g_Clipboard;

    static bool g_IsBoxSelecting = false;
    static ImVec2 g_BoxSelectStart;

    static bool IsSelected(int track, int index) {
        return std::find(g_SelectedKeys.begin(), g_SelectedKeys.end(), KeyRef{ track, index }) != g_SelectedKeys.end();
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

        virtual int GetItemCount() const override { return 3; }
        virtual int GetItemTypeCount() const override { return 0; }
        virtual const char* GetItemTypeName(int typeIndex) const override { return ""; }
        virtual const char* GetItemLabel(int index) const override {
            if (index == 0) return "Position";
            if (index == 1) return "Rotation";
            if (index == 2) return "Scale";
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
                if (index == 0) *color = 0xFF444499;
                if (index == 1) *color = 0xFF449944;
                if (index == 2) *color = 0xFF994444;
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

            auto drawAndHandleKeyframes = [&](auto& track, ImU32 outlineColor, int trackIdx)
                {
                    int hoveredIndex = -1;
                    float size = 5.0f;

                    for (size_t i = 0; i < track.keyframes.size(); ++i)
                    {
                        float x = TimeToX(track.keyframes[i].time, rc);
                        float y = rc.Min.y + rc.GetHeight() * 0.5f;

                        ImRect kfRect(x - size * 1.5f, y - size * 1.5f, x + size * 1.5f, y + size * 1.5f);
                        bool hovered = kfRect.Contains(io.MousePos);
                        if (hovered) hoveredIndex = static_cast<int>(i);

                        if (g_IsBoxSelecting) {
                            ImRect boxRect(ImMin(g_BoxSelectStart, io.MousePos), ImMax(g_BoxSelectStart, io.MousePos));
                            if (boxRect.Contains(ImVec2(x, y)) && !IsSelected(trackIdx, static_cast<int>(i))) {
                                g_SelectedKeys.push_back({ trackIdx, (int)i });
                            }
                        }

                        bool selected = IsSelected(trackIdx, static_cast<int>(i));
                        ImU32 fillColor = IM_COL32(255, 255, 255, 255);

                        if (selected) fillColor = IM_COL32(0, 255, 255, 255); // Cyan
                        else if (hovered) fillColor = IM_COL32(255, 255, 150, 255); // Hover

                        ImVec2 p1(x, y - size), p2(x + size, y), p3(x, y + size), p4(x - size, y);
                        draw_list->AddQuadFilled(p1, p2, p3, p4, fillColor);
                        draw_list->AddQuad(p1, p2, p3, p4, outlineColor, 1.5f);
                    }

                    if (g_IsDragging)
                    {
                        float dragDelta = XToTime(io.MousePos.x, rc) - g_DragStartMouseTime;

                        for (const auto& ghost : g_DragGhosts)
                        {
                            if (ghost.track != trackIdx) continue;

                            float newTime = std::clamp(ghost.origTime + dragDelta, 0.0f, clip->duration);
                            float x = TimeToX(newTime, rc);
                            float y = rc.Min.y + rc.GetHeight() * 0.5f;

                            ImVec2 p1(x, y - size), p2(x + size, y), p3(x, y + size), p4(x - size, y);
                            draw_list->AddQuadFilled(p1, p2, p3, p4, IM_COL32(255, 200, 50, 255));
                            draw_list->AddQuad(p1, p2, p3, p4, IM_COL32(255, 255, 255, 200), 1.5f);
                        }
                    }

                    if (rc.Contains(io.MousePos) && !g_IsDragging)
                    {
                        if (io.MouseClicked[0])
                        {
                            if (hoveredIndex != -1)
                            {
                                if (io.KeyShift || io.KeyCtrl)
                                {
                                    if (IsSelected(trackIdx, hoveredIndex)) {
                                        g_SelectedKeys.erase(std::remove(g_SelectedKeys.begin(), g_SelectedKeys.end(), KeyRef{ trackIdx, hoveredIndex }), g_SelectedKeys.end());
                                    }
                                    else {
                                        g_SelectedKeys.push_back({ trackIdx, hoveredIndex });
                                    }
                                }
                                else
                                {
                                    if (!IsSelected(trackIdx, hoveredIndex)) {
                                        g_SelectedKeys.clear();
                                        g_SelectedKeys.push_back({ trackIdx, hoveredIndex });
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
                            if (IsSelected(trackIdx, hoveredIndex))
                            {
                                g_IsDragging = true;
                                g_DragStartMouseTime = XToTime(io.MousePos.x, rc);
                                g_DragGhosts.clear();

                                auto extractGhosts = [&](auto& trk, int tIdx, auto assignFunc) {
                                    std::vector<int> toErase;
                                    for (auto& sk : g_SelectedKeys) {
                                        if (sk.track == tIdx) {
                                            DragGhost g; g.track = tIdx; g.origTime = trk.keyframes[sk.index].time;
                                            assignFunc(g, trk.keyframes[sk.index].value);
                                            g_DragGhosts.push_back(g);
                                            toErase.push_back(sk.index);
                                        }
                                    }
                                    std::sort(toErase.rbegin(), toErase.rend());
                                    for (int idx : toErase) trk.keyframes.erase(trk.keyframes.begin() + idx);
                                    };

                                extractGhosts(clip->positionTrack, 0, [](DragGhost& g, const glm::vec3& val) { g.v3 = val; });
                                extractGhosts(clip->rotationTrack, 1, [](DragGhost& g, const glm::quat& val) { g.q = val; });
                                extractGhosts(clip->scaleTrack, 2, [](DragGhost& g, const glm::vec3& val) { g.v3 = val; });

                                g_SelectedKeys.clear();
                            }
                        }
                    }

                    if (g_IsDragging && io.MouseReleased[0])
                    {
                        float dragDelta = XToTime(io.MousePos.x, rc) - g_DragStartMouseTime;

                        for (const auto& ghost : g_DragGhosts)
                        {
                            float newTime = std::clamp(ghost.origTime + dragDelta, 0.0f, clip->duration);

                            if (ghost.track == 0) clip->positionTrack.AddKeyframe(newTime, ghost.v3);
                            if (ghost.track == 1) clip->rotationTrack.AddKeyframe(newTime, ghost.q);
                            if (ghost.track == 2) clip->scaleTrack.AddKeyframe(newTime, ghost.v3);

                            auto reselect = [&](const auto& trk) {
                                for (int i = 0; i < trk.keyframes.size(); i++) {
                                    if (std::abs(trk.keyframes[i].time - newTime) < 0.001f) {
                                        g_SelectedKeys.push_back({ ghost.track, i });
                                        break;
                                    }
                                }
                                };

                            if (ghost.track == 0) reselect(clip->positionTrack);
                            if (ghost.track == 1) reselect(clip->rotationTrack);
                            if (ghost.track == 2) reselect(clip->scaleTrack);
                        }

                        g_DragGhosts.clear();
                        g_IsDragging = false;
                    }
                };

            if (index == 0) drawAndHandleKeyframes(clip->positionTrack, IM_COL32(255, 50, 50, 255), 0);
            if (index == 1) drawAndHandleKeyframes(clip->rotationTrack, IM_COL32(50, 255, 50, 255), 1);
            if (index == 2) drawAndHandleKeyframes(clip->scaleTrack, IM_COL32(50, 100, 255, 255), 2);

            draw_list->PopClipRect();
        }
    };

    void AnimatorPanel::Draw(Engine* engine, entt::entity& selectedEntity)
    {
        ImGui::Begin("Animator / Sequencer");

        auto& registry = engine->GetActiveScene().GetRegistry();

        if (selectedEntity == entt::null || !registry.valid(selectedEntity))
        {
            ImGui::TextDisabled("Select an entity to animate.");
            ImGui::End();
            return;
        }

        if (!registry.any_of<AnimatorComponent>(selectedEntity))
        {
            ImGui::TextDisabled("Selected entity does not have an AnimatorComponent.");
            if (ImGui::Button("Add Animator")) registry.emplace<AnimatorComponent>(selectedEntity);
            ImGui::End();
            return;
        }

        auto& animator = registry.get<AnimatorComponent>(selectedEntity);
        if (!animator.currentClip)
        {
            ImGui::TextDisabled("No Animation Clip assigned.");
            if (ImGui::Button("Create Empty Clip")) {
                animator.currentClip = std::make_shared<Animation::AnimationClip>("New Sequence");
                animator.currentClip->duration = 2.0f;
            }
            ImGui::End();
            return;
        }

        auto& clip = animator.currentClip;
        ImGuiIO& io = ImGui::GetIO();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));

        if (ImGui::Button(animator.isPlaying ? "|| Pause" : "> Play")) {
            if (animator.isPlaying) animator.Pause();
            else animator.Play();
        }
        ImGui::SameLine();
        if (ImGui::Button("[ ] Stop")) {
            animator.Stop();
            animator.currentTime = 0.0f;
        }
        ImGui::SameLine(); ImGui::TextDisabled(" | "); ImGui::SameLine();

        ImGui::SetNextItemWidth(100.0f);
        const char* loopModes[] = { "None", "Linear", "PingPong" };
        int currentLoop = static_cast<int>(clip->loopMode);
        if (ImGui::Combo("Loop Mode", &currentLoop, loopModes, 3)) clip->loopMode = static_cast<Animation::LoopMode>(currentLoop);

        ImGui::SameLine(); ImGui::TextDisabled(" | "); ImGui::SameLine();

        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::DragFloat("Duration", &clip->duration, 0.1f, 0.1f, 100.0f, "%.2fs"))
            clip->duration = std::max(0.1f, clip->duration);

        ImGui::PopStyleVar();
        ImGui::Separator();

        ImGui::TextDisabled("Record Keyframes at %.2fs:", animator.currentTime);
        if (registry.any_of<TransformComponent>(selectedEntity))
        {
            auto& tc = registry.get<TransformComponent>(selectedEntity);

            auto recordAction = [&](const char* label, auto& track, auto value) {
                if (ImGui::Button(label)) {
                    track.AddKeyframe(animator.currentTime, value);
                    if (animator.currentTime > clip->duration) clip->duration = animator.currentTime;
                    clip->RecalculateDuration();
                }
                };

            recordAction("+ Pos Key", clip->positionTrack, tc.GetPosition());
            ImGui::SameLine();
            recordAction("+ Rot Key", clip->rotationTrack, tc.GetRotation());
            ImGui::SameLine();
            recordAction("+ Scale Key", clip->scaleTrack, tc.GetScale());
        }
        ImGui::Separator();

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !g_SelectedKeys.empty())
            {
                auto deleteFromTrack = [&](auto& trk, int tIdx) {
                    std::vector<int> toErase;
                    for (auto& sk : g_SelectedKeys) if (sk.track == tIdx) toErase.push_back(sk.index);
                    std::sort(toErase.rbegin(), toErase.rend());
                    for (int idx : toErase) trk.keyframes.erase(trk.keyframes.begin() + idx);
                    };
                deleteFromTrack(clip->positionTrack, 0);
                deleteFromTrack(clip->rotationTrack, 1);
                deleteFromTrack(clip->scaleTrack, 2);
                g_SelectedKeys.clear();
            }

            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C) && !g_SelectedKeys.empty())
            {
                g_Clipboard.clear();

                float minTime = FLT_MAX;
                auto checkMin = [&](auto& trk, int tIdx) {
                    for (auto& sk : g_SelectedKeys) {
                        if (sk.track == tIdx && trk.keyframes[sk.index].time < minTime)
                            minTime = trk.keyframes[sk.index].time;
                    }
                    };
                checkMin(clip->positionTrack, 0); checkMin(clip->rotationTrack, 1); checkMin(clip->scaleTrack, 2);

                auto copyToBoard = [&](auto& trk, int tIdx, auto assignFunc) {
                    for (auto& sk : g_SelectedKeys) {
                        if (sk.track == tIdx) {
                            ClipboardKey ck; ck.track = tIdx; ck.timeOffset = trk.keyframes[sk.index].time - minTime;
                            assignFunc(ck, trk.keyframes[sk.index].value);
                            g_Clipboard.push_back(ck);
                        }
                    }
                    };
                copyToBoard(clip->positionTrack, 0, [](ClipboardKey& ck, const glm::vec3& val) { ck.v3 = val; });
                copyToBoard(clip->rotationTrack, 1, [](ClipboardKey& ck, const glm::quat& val) { ck.q = val; });
                copyToBoard(clip->scaleTrack, 2, [](ClipboardKey& ck, const glm::vec3& val) { ck.v3 = val; });
            }

            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V) && !g_Clipboard.empty())
            {
                g_SelectedKeys.clear();
                float pasteTime = animator.currentTime;

                for (auto& ck : g_Clipboard)
                {
                    float t = pasteTime + ck.timeOffset;
                    if (ck.track == 0) clip->positionTrack.AddKeyframe(t, ck.v3);
                    if (ck.track == 1) clip->rotationTrack.AddKeyframe(t, ck.q);
                    if (ck.track == 2) clip->scaleTrack.AddKeyframe(t, ck.v3);

                    if (t > clip->duration) clip->duration = t;
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
            animator.currentTime = static_cast<float>(currentFrame) / SEQUENCE_FPS;
            animator.Pause();
            if (registry.any_of<TransformComponent>(selectedEntity)) {
                auto& tc = registry.get<TransformComponent>(selectedEntity);
                if (!clip->positionTrack.keyframes.empty()) tc.SetPosition(clip->positionTrack.Sample(animator.currentTime));
                if (!clip->rotationTrack.keyframes.empty()) tc.SetRotation(clip->rotationTrack.Sample(animator.currentTime));
                if (!clip->scaleTrack.keyframes.empty()) tc.SetScale(clip->scaleTrack.Sample(animator.currentTime));
                tc.SetDirty();
            }
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