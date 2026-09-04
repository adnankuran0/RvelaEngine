#include "AnimationSequenceAdapter.h"
#include "Scene/Components/AnimatorComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/SceneTreeComponent.h"
#include "Scene/Components/TagComponent.h"
#include <algorithm>
#include <sstream>

namespace rv {

entt::entity ResolvePathStatic(entt::registry& reg, entt::entity root, const std::string& path)
{
    if (path.empty()) return root;
    entt::entity current = root;
    std::istringstream stream(path);
    std::string childName;

    while (std::getline(stream, childName, '/')) {
        if (!reg.valid(current) || !reg.any_of<SceneTreeComponent>(current))
            return entt::null;

        auto& tree = reg.get<SceneTreeComponent>(current);
        bool found = false;

        for (auto child : tree.children) {
            if (reg.valid(child) && reg.any_of<TagComponent>(child)) {
                if (reg.get<TagComponent>(child).tag == childName) {
                    current = child;
                    found = true;
                    break;
                }
            }
        }
        if (!found) return entt::null;
    }
    return current;
}

AnimationSequenceAdapter::AnimationSequenceAdapter(std::shared_ptr<Animation::AnimationClip> c,
    AnimatorComponent* anim,
    entt::registry* reg,
    entt::entity ent,
    SequencerContext& context)
    : clip(c), animator(anim), registry(reg), entity(ent), ctx(context)
{
}

int AnimationSequenceAdapter::GetFrameMax() const
{
    int maxF = static_cast<int>(std::round(clip->duration * SEQUENCE_FPS));
    return std::max(1, maxF);
}

int AnimationSequenceAdapter::GetItemCount() const
{
    return 4 + static_cast<int>(clip->propertyTracks.size());
}

const char* AnimationSequenceAdapter::GetItemLabel(int index) const
{
    if (index == 0) return "Events";
    if (index == 1) return "Position";
    if (index == 2) return "Rotation";
    if (index == 3) return "Scale";

    size_t propIdx = index - 4;
    if (propIdx < clip->propertyTracks.size()) {
        if (itemLabels.size() <= propIdx)
            itemLabels.resize(clip->propertyTracks.size());

        const auto& track = clip->propertyTracks[propIdx];
        size_t dotPos = track->propertyName.find('.');
        std::string shortProp = (dotPos != std::string::npos) ? track->propertyName.substr(dotPos + 1) : track->propertyName;

        if (track->targetPath.empty()) {
            itemLabels[propIdx] = shortProp;
        }
        else {
            itemLabels[propIdx] = track->targetPath + " - " + shortProp;
        }

        return itemLabels[propIdx].c_str();
    }
    return "";
}

void AnimationSequenceAdapter::Get(int index, int** start, int** end, int* type, unsigned int* color)
{
    static int s = 0;
    static int e = 0;
    s = 0; e = GetFrameMax();
    if (start) *start = &s;
    if (end) *end = &e;
    if (type) *type = 0;

    if (color) {
        ImU32 lightGray = IM_COL32(95, 95, 100, 255);
        ImU32 darkGray = IM_COL32(78, 78, 83, 255);

        *color = (index % 2 == 0) ? lightGray : darkGray;

        if (index == 0) *color = IM_COL32(105, 95, 55, 255);
        else if (index == 1) *color = IM_COL32(115, 75, 75, 255);
        else if (index == 2) *color = IM_COL32(75, 115, 85, 255);
        else if (index == 3) *color = IM_COL32(75, 80, 120, 255);
    }
}

float AnimationSequenceAdapter::TimeToX(float time, const ImRect& rc) const
{
    float kfFrame = time * SEQUENCE_FPS;
    float totalFrames = static_cast<float>(GetFrameMax() - frameMin + 2.0f);
    float fraction = (kfFrame - frameMin + 0.5f) / totalFrames;
    return rc.Min.x + fraction * rc.GetWidth();
}

float AnimationSequenceAdapter::XToTime(float x, const ImRect& rc) const
{
    float fraction = (x - rc.Min.x) / rc.GetWidth();
    float totalFrames = static_cast<float>(GetFrameMax() - frameMin + 2.0f);
    float kfFrame = fraction * totalFrames + frameMin - 0.5f;
    return kfFrame / SEQUENCE_FPS;
}

void AnimationSequenceAdapter::SetupDragMode(float clickedTime)
{
    if (ctx.selectedKeys.size() <= 1) {
        ctx.dragMode = DragMode::Translate;
        return;
    }

    float minT = FLT_MAX;
    float maxT = -FLT_MAX;
    for (const auto& sk : ctx.selectedKeys) {
        minT = std::min(minT, sk.time);
        maxT = std::max(maxT, sk.time);
    }

    if (maxT - minT > 0.001f) {
        if (std::abs(clickedTime - maxT) < 0.001f) {
            ctx.dragMode = DragMode::ScaleRight;
            ctx.scaleAnchorTime = minT;
            ctx.scaleOrigSpan = maxT - minT;
        }
        else if (std::abs(clickedTime - minT) < 0.001f) {
            ctx.dragMode = DragMode::ScaleLeft;
            ctx.scaleAnchorTime = maxT;
            ctx.scaleOrigSpan = maxT - minT;
        }
        else {
            ctx.dragMode = DragMode::Translate;
        }
    }
    else {
        ctx.dragMode = DragMode::Translate;
    }
}

void AnimationSequenceAdapter::CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect)
{
    draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
    ImGuiIO& io = ImGui::GetIO();

    bool isHoveringTrack = rc.Contains(io.MousePos);
    if (ctx.isDragging && isHoveringTrack) {
        ctx.currentDragDelta = XToTime(io.MousePos.x, rc) - ctx.dragStartMouseTime;
    }

    if (index == 0) {
        DrawEventTrack(draw_list, rc);
    }
    else if (index == 1) {
        DrawTrackKeyframes(clip->positionTrack, IM_COL32(255, 50, 50, 255), 1, draw_list, rc);
    }
    else if (index == 2) {
        DrawTrackKeyframes(clip->rotationTrack, IM_COL32(50, 255, 50, 255), 2, draw_list, rc);
    }
    else if (index == 3) {
        DrawTrackKeyframes(clip->scaleTrack, IM_COL32(50, 100, 255, 255), 3, draw_list, rc);
    }
    else {
        size_t propIdx = index - 4;
        if (propIdx < clip->propertyTracks.size()) {
            auto pTrack = clip->propertyTracks[propIdx];
            if (pTrack->GetType() == Animation::PropertyType::Float) {
                auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(pTrack);
                DrawTrackKeyframes(t->track, IM_COL32(220, 140, 40, 255), index, draw_list, rc);
            }
            else if (pTrack->GetType() == Animation::PropertyType::Vec3) {
                auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack);
                DrawTrackKeyframes(t->track, IM_COL32(60, 180, 200, 255), index, draw_list, rc);
            }
            else if (pTrack->GetType() == Animation::PropertyType::Vec4) {
                auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(pTrack);
                DrawTrackKeyframes(t->track, IM_COL32(200, 60, 200, 255), index, draw_list, rc);
            }
            else if (pTrack->GetType() == Animation::PropertyType::Quat) {
                auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pTrack);
                DrawTrackKeyframes(t->track, IM_COL32(40, 200, 140, 255), index, draw_list, rc);
            }
            else if (pTrack->GetType() == Animation::PropertyType::Bool) {
                auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<bool>>(pTrack);
                DrawTrackKeyframes(t->track, IM_COL32(85, 153, 221, 255), index, draw_list, rc);
            }
        }
    }

    draw_list->PopClipRect();
}

void AnimationSequenceAdapter::DrawEventTrack(ImDrawList* draw_list, const ImRect& rc)
{
    ImGuiIO& io = ImGui::GetIO();
    bool isHoveringTrack = rc.Contains(io.MousePos);
    bool uiCapturingMouse = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);

    int hoveredIndex = -1;
    float size = 6.5f;

    for (size_t i = 0; i < clip->eventTrack.size(); ++i) {
        float evTime = clip->eventTrack[i].time;
        float x = std::round(TimeToX(evTime, rc));
        float y = std::round(rc.Min.y + rc.GetHeight() * 0.5f);

        ImRect kfRect(x - size * 1.5f, y - size * 1.5f, x + size * 1.5f, y + size * 1.5f);
        bool hovered = kfRect.Contains(io.MousePos);
        if (hovered) hoveredIndex = static_cast<int>(i);

        if (ctx.isBoxSelecting) {
            ImRect boxRect(ImMin(ctx.boxSelectStart, io.MousePos), ImMax(ctx.boxSelectStart, io.MousePos));
            if (boxRect.Contains(ImVec2(x, y)) && !ctx.IsKeySelected(0, evTime)) {
                ctx.selectedKeys.push_back({ 0, evTime });
            }
        }

        bool selected = ctx.IsKeySelected(0, evTime);
        ImU32 fillColor = selected ? IM_COL32(255, 255, 255, 255) : (hovered ? IM_COL32(255, 235, 120, 255) : IM_COL32(255, 204, 0, 255));

        ImVec2 p1(x - size, y - size), p2(x + size, y - size), p3(x, y + size);
        draw_list->AddTriangleFilled(p1, p2, p3, fillColor);
        draw_list->AddTriangle(p1, p2, p3, IM_COL32(180, 130, 0, 255), 1.5f);

        if (!clip->eventTrack[i].name.empty()) {
            char shortLabel[32];
            snprintf(shortLabel, sizeof(shortLabel), " %s", clip->eventTrack[i].name.c_str());
            draw_list->AddText(ImVec2(x + size * 0.8f, y - size), IM_COL32(255, 220, 80, 220), shortLabel);
        }
    }

    if (ctx.isDragging) {
        for (const auto& ghost : ctx.dragGhosts) {
            if (ghost.track != 0) continue;
            float newTime = ctx.CalculateNewTime(ghost.origTime, clip->duration);

            float x = std::round(TimeToX(newTime, rc));
            float y = std::round(rc.Min.y + rc.GetHeight() * 0.5f);

            ImVec2 p1(x - size, y - size), p2(x + size, y - size), p3(x, y + size);
            draw_list->AddTriangleFilled(p1, p2, p3, IM_COL32(255, 170, 0, 200));
            draw_list->AddTriangle(p1, p2, p3, IM_COL32(255, 255, 255, 220), 1.5f);
        }
    }

    if (isHoveringTrack && !ctx.isDragging && !uiCapturingMouse) {
        if (io.MouseClicked[1] && hoveredIndex != -1) {
            float clickedTime = clip->eventTrack[hoveredIndex].time;
            if (!ctx.IsKeySelected(0, clickedTime)) {
                ctx.selectedKeys.clear();
                ctx.selectedKeys.push_back({ 0, clickedTime });
            }
            ImGui::OpenPopup("EventEditPopup");
        }

        if (io.MouseDoubleClicked[0]) {
            if (hoveredIndex == -1) {
                float newTime = std::clamp(XToTime(io.MousePos.x, rc), 0.0f, clip->duration);
                clip->AddEvent(newTime, "NewEvent", "");
                ctx.selectedKeys.clear();
                ctx.selectedKeys.push_back({ 0, newTime });
            }
        }
        else if (io.MouseClicked[0]) {
            if (hoveredIndex != -1) {
                float clickedTime = clip->eventTrack[hoveredIndex].time;
                if (io.KeyShift || io.KeyCtrl) {
                    if (ctx.IsKeySelected(0, clickedTime)) {
                        ctx.selectedKeys.erase(std::remove(ctx.selectedKeys.begin(), ctx.selectedKeys.end(), SelectedKey{ 0, clickedTime }), ctx.selectedKeys.end());
                    }
                    else {
                        ctx.selectedKeys.push_back({ 0, clickedTime });
                    }
                }
                else {
                    if (!ctx.IsKeySelected(0, clickedTime)) {
                        ctx.selectedKeys.clear();
                        ctx.selectedKeys.push_back({ 0, clickedTime });
                    }
                }
            }
            else {
                if (!io.KeyShift && !io.KeyCtrl) {
                    ctx.selectedKeys.clear();
                }
                ctx.isBoxSelecting = true;
                ctx.boxSelectStart = io.MousePos;
            }
        }

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f) && hoveredIndex != -1 && !ctx.isBoxSelecting && !ctx.isDragging) {
            float clickedTime = clip->eventTrack[hoveredIndex].time;
            if (ctx.IsKeySelected(0, clickedTime)) {
                ctx.isDragging = true;
                ctx.dragStartMouseTime = XToTime(io.MousePos.x, rc);
                ctx.currentDragDelta = 0.0f;
                SetupDragMode(clickedTime);
                ctx.dragGhosts.clear();

                for (const auto& sk : ctx.selectedKeys) {
                    if (sk.track == 0) {
                        for (size_t k = 0; k < clip->eventTrack.size(); ++k) {
                            if (std::abs(clip->eventTrack[k].time - sk.time) < 0.001f) {
                                DragGhost g;
                                g.track = 0;
                                g.origTime = clip->eventTrack[k].time;
                                g.eventName = clip->eventTrack[k].name;
                                g.eventParam = clip->eventTrack[k].parameter;
                                ctx.dragGhosts.push_back(g);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

template <typename TrackType>
void AnimationSequenceAdapter::DrawTrackKeyframes(TrackType& track, ImU32 outlineColor, int trackIdx, ImDrawList* draw_list, const ImRect& rc)
{
    ImGuiIO& io = ImGui::GetIO();
    bool isHoveringTrack = rc.Contains(io.MousePos);
    bool uiCapturingMouse = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);

    int hoveredIndex = -1;
    float size = 5.0f;

    for (size_t i = 0; i < track.keyframes.size(); ++i) {
        float kfTime = track.keyframes[i].time;

        float x = std::round(TimeToX(kfTime, rc));
        float y = std::round(rc.Min.y + rc.GetHeight() * 0.5f);

        ImRect kfRect(x - size * 1.5f, y - size * 1.5f, x + size * 1.5f, y + size * 1.5f);
        bool hovered = kfRect.Contains(io.MousePos);
        if (hovered) hoveredIndex = static_cast<int>(i);

        if (ctx.isBoxSelecting) {
            ImRect boxRect(ImMin(ctx.boxSelectStart, io.MousePos), ImMax(ctx.boxSelectStart, io.MousePos));
            if (boxRect.Contains(ImVec2(x, y)) && !ctx.IsKeySelected(trackIdx, kfTime)) {
                ctx.selectedKeys.push_back({ trackIdx, kfTime });
            }
        }

        bool selected = ctx.IsKeySelected(trackIdx, kfTime);
        ImU32 fillColor = selected ? IM_COL32(0, 255, 255, 255) : (hovered ? IM_COL32(200, 200, 100, 255) : IM_COL32(255, 255, 255, 255));

        ImVec2 p1(x, y - size);
        ImVec2 p2(x + size, y);
        ImVec2 p3(x, y + size);
        ImVec2 p4(x - size, y);
        draw_list->AddQuadFilled(p1, p2, p3, p4, fillColor);
        draw_list->AddQuad(p1, p2, p3, p4, outlineColor, 1.5f);
    }

    if (ctx.isDragging) {
        for (const auto& ghost : ctx.dragGhosts) {
            if (ghost.track != trackIdx) continue;

            float newTime = ctx.CalculateNewTime(ghost.origTime, clip->duration);
            float x = std::round(TimeToX(newTime, rc));
            float y = std::round(rc.Min.y + rc.GetHeight() * 0.5f);

            ImVec2 p1(x, y - size);
            ImVec2 p2(x + size, y);
            ImVec2 p3(x, y + size);
            ImVec2 p4(x - size, y);
            draw_list->AddQuadFilled(p1, p2, p3, p4, IM_COL32(255, 200, 50, 230));
            draw_list->AddQuad(p1, p2, p3, p4, IM_COL32(255, 255, 255, 220), 1.5f);
        }
    }

    if (isHoveringTrack && !ctx.isDragging && !uiCapturingMouse) {
        if (io.MouseClicked[0] || io.MouseClicked[1] || io.MouseDoubleClicked[0]) {
            ctx.selectedTrack = trackIdx;
        }

        if (io.MouseClicked[1] && hoveredIndex != -1) {
            float clickedTime = track.keyframes[hoveredIndex].time;
            if (!ctx.IsKeySelected(trackIdx, clickedTime)) {
                ctx.selectedKeys.clear();
                ctx.selectedKeys.push_back({ trackIdx, clickedTime });
            }
            ImGui::OpenPopup("KeyframeContextMenu");
        }

        if (io.MouseDoubleClicked[0]) {
            if (hoveredIndex == -1) {
                float newTime = std::clamp(XToTime(io.MousePos.x, rc), 0.0f, clip->duration);
                using ValueType = std::decay_t<decltype(track.Sample(0.0f))>;
                ValueType initialValue{};
                bool valueFetched = false;

                if (!track.keyframes.empty()) {
                    initialValue = track.Sample(newTime);
                    valueFetched = true;
                }
                else {
                    if constexpr (std::is_same_v<ValueType, glm::vec3>) {
                        if (trackIdx == 1 && registry->any_of<TransformComponent>(entity)) {
                            initialValue = registry->get<TransformComponent>(entity).GetPosition();
                            valueFetched = true;
                        }
                        else if (trackIdx == 3 && registry->any_of<TransformComponent>(entity)) {
                            initialValue = registry->get<TransformComponent>(entity).GetScale();
                            valueFetched = true;
                        }
                    }
                    else if constexpr (std::is_same_v<ValueType, glm::quat>) {
                        if (trackIdx == 2 && registry->any_of<TransformComponent>(entity)) {
                            initialValue = registry->get<TransformComponent>(entity).GetRotation();
                            valueFetched = true;
                        }
                    }

                    if (!valueFetched && trackIdx >= 4) {
                        size_t pIdx = trackIdx - 4;
                        if (pIdx < clip->propertyTracks.size()) {
                            auto pTrack = clip->propertyTracks[pIdx];
                            entt::entity target = ResolvePathStatic(*registry, entity, pTrack->targetPath);
                            if (target != entt::null && registry->valid(target)) {
                                if (pTrack->propertyName == "PointLightComponent.intensity" && registry->any_of<PointLightComponent>(target)) {
                                    if constexpr (std::is_same_v<ValueType, float>) { initialValue = registry->get<PointLightComponent>(target).intensity; valueFetched = true; }
                                }
                                else if (pTrack->propertyName == "PointLightComponent.color" && registry->any_of<PointLightComponent>(target)) {
                                    if constexpr (std::is_same_v<ValueType, glm::vec3>) { initialValue = registry->get<PointLightComponent>(target).color; valueFetched = true; }
                                }
                                else if (pTrack->propertyName == "PointLightComponent.radius" && registry->any_of<PointLightComponent>(target)) {
                                    if constexpr (std::is_same_v<ValueType, float>) { initialValue = registry->get<PointLightComponent>(target).radius; valueFetched = true; }
                                }
                            }
                        }
                    }
                }

                if (!valueFetched) {
                    if constexpr (std::is_same_v<ValueType, glm::vec3>) initialValue = (trackIdx == 3) ? glm::vec3(1.0f) : glm::vec3(0.0f);
                    else if constexpr (std::is_same_v<ValueType, glm::quat>) initialValue = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    else if constexpr (std::is_same_v<ValueType, float>) initialValue = 0.0f;
                    else if constexpr (std::is_same_v<ValueType, bool>) initialValue = false;
                }

                track.AddKeyframe(newTime, initialValue, Animation::EaseType::LINEAR);
                ctx.selectedKeys.clear();
                ctx.selectedKeys.push_back({ trackIdx, newTime });
            }
        }
        else if (io.MouseClicked[0]) {
            if (hoveredIndex != -1) {
                float clickedTime = track.keyframes[hoveredIndex].time;
                if (io.KeyShift || io.KeyCtrl) {
                    if (ctx.IsKeySelected(trackIdx, clickedTime)) {
                        ctx.selectedKeys.erase(std::remove(ctx.selectedKeys.begin(), ctx.selectedKeys.end(), SelectedKey{ trackIdx, clickedTime }), ctx.selectedKeys.end());
                    }
                    else {
                        ctx.selectedKeys.push_back({ trackIdx, clickedTime });
                    }
                }
                else {
                    if (!ctx.IsKeySelected(trackIdx, clickedTime)) {
                        ctx.selectedKeys.clear();
                        ctx.selectedKeys.push_back({ trackIdx, clickedTime });
                    }
                }
            }
            else {
                if (!io.KeyShift && !io.KeyCtrl) {
                    ctx.selectedKeys.clear();
                }
                ctx.isBoxSelecting = true;
                ctx.boxSelectStart = io.MousePos;
            }
        }

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f) && hoveredIndex != -1 && !ctx.isBoxSelecting && !ctx.isDragging) {
            float clickedTime = track.keyframes[hoveredIndex].time;
            if (ctx.IsKeySelected(trackIdx, clickedTime)) {
                ctx.isDragging = true;
                ctx.dragStartMouseTime = XToTime(io.MousePos.x, rc);
                ctx.currentDragDelta = 0.0f;
                SetupDragMode(clickedTime);
                ctx.dragGhosts.clear();

                auto extractGhosts = [&](auto& trk, int tIdx, auto assignFunc) {
                    for (const auto& sk : ctx.selectedKeys) {
                        if (sk.track == tIdx) {
                            for (size_t k = 0; k < trk.keyframes.size(); ++k) {
                                if (std::abs(trk.keyframes[k].time - sk.time) < 0.001f) {
                                    DragGhost g;
                                    g.track = tIdx;
                                    g.origTime = trk.keyframes[k].time;
                                    g.ease = trk.keyframes[k].ease;
                                    assignFunc(g, trk.keyframes[k].value);
                                    ctx.dragGhosts.push_back(g);
                                    break;
                                }
                            }
                        }
                    }
                    };

                extractGhosts(clip->positionTrack, 1, [](DragGhost& g, const glm::vec3& val) { g.v3 = val; });
                extractGhosts(clip->rotationTrack, 2, [](DragGhost& g, const glm::quat& val) { g.q = val; });
                extractGhosts(clip->scaleTrack, 3, [](DragGhost& g, const glm::vec3& val) { g.v3 = val; });

                for (size_t p = 0; p < clip->propertyTracks.size(); ++p) {
                    int pTrackIdx = 4 + static_cast<int>(p);
                    auto pTrack = clip->propertyTracks[p];
                    if (pTrack->GetType() == Animation::PropertyType::Float) {
                        auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(pTrack);
                        extractGhosts(t->track, pTrackIdx, [](DragGhost& g, float val) { g.fVal = val; });
                    }
                    else if (pTrack->GetType() == Animation::PropertyType::Vec3) {
                        auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack);
                        extractGhosts(t->track, pTrackIdx, [](DragGhost& g, const glm::vec3& val) { g.v3 = val; });
                    }
                    else if (pTrack->GetType() == Animation::PropertyType::Vec4) {
                        auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(pTrack);
                        extractGhosts(t->track, pTrackIdx, [](DragGhost& g, const glm::vec4& val) { g.v4 = val; });
                    }
                    else if (pTrack->GetType() == Animation::PropertyType::Quat) {
                        auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pTrack);
                        extractGhosts(t->track, pTrackIdx, [](DragGhost& g, const glm::quat& val) { g.q = val; });
                    }
                    else if (pTrack->GetType() == Animation::PropertyType::Bool) {
                        auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<bool>>(pTrack);
                        extractGhosts(t->track, pTrackIdx, [](DragGhost& g, bool val) { g.bVal = val; });
                    }
                }
            }
        }
    }
}

}