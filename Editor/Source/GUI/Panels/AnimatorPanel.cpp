#include "AnimatorPanel.h"
#include "Core/Engine.h"
#include "Scene/Components/AnimatorComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/SceneTreeComponent.h"
#include "Scene/Components/TagComponent.h"
#include "AssetImporters/AnimationLibrarySerializer.h"
#include "Asset/AssetManager.h"
#include "EditorUtils.h"
#include <algorithm>
#include <cmath>
#include <map>
#include "Render/IconLibrary.h"

namespace rv {

static const char* k_EaseNames[] = { "Linear", "Ease In", "Ease Out", "Ease In-Out", "Ease Out-In", "Zero" };

static void CollectSubtreePaths(entt::registry& reg, entt::entity current, const std::string& currentPath, std::vector<std::pair<std::string, entt::entity>>& outPaths)
{
    outPaths.push_back({ currentPath, current });

    if (reg.valid(current) && reg.any_of<SceneTreeComponent>(current)) {
        auto& tree = reg.get<SceneTreeComponent>(current);
        for (auto child : tree.children) {
            if (reg.valid(child) && reg.any_of<TagComponent>(child)) {
                std::string childTag = reg.get<TagComponent>(child).tag;
                std::string nextPath = currentPath.empty() ? childTag : (currentPath + "/" + childTag);
                CollectSubtreePaths(reg, child, nextPath, outPaths);
            }
        }
    }
}

void AnimatorPanel::Draw(Engine* engine, entt::entity& selectedEntity)
{
    ImGui::Begin("Animator");

    if (m_SeqContext.lastSelectedEntity != selectedEntity) {
        m_SeqContext.ResetSelectionState();
        m_SeqContext.lastSelectedEntity = selectedEntity;
    }

    auto& registry = engine->GetActiveScene().GetRegistry();

    if (selectedEntity == entt::null || !registry.valid(selectedEntity)) {
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y * 0.45f));
        const char* msg = "Select an entity to animate.";
        float textWidth = ImGui::CalcTextSize(msg).x;
        ImGui::SetCursorPosX((size.x - textWidth) * 0.5f);
        ImGui::TextDisabled("%s", msg);
        ImGui::End();
        return;
    }

    if (!registry.any_of<AnimatorComponent>(selectedEntity)) {
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

    if (!animator.library || !animator.libraryUUID.IsValid()) {
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

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
                std::string pathStr((const char*)payload->Data);
                if (pathStr.ends_with(".ranimlib") || pathStr.ends_with(".ranim")) {
                    AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                    if (uuid.IsValid()) {
                        animator.SetLibrary(uuid);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::End();
        return;
    }

    if (animator.library->GetClips().empty()) {
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y * 0.4f));

        const char* msg = "Animation Library contains no clips.";
        float textWidth = ImGui::CalcTextSize(msg).x;
        ImGui::SetCursorPosX((size.x - textWidth) * 0.5f);
        ImGui::TextDisabled("%s", msg);

        ImGui::Spacing();
        ImGui::SetCursorPosX((size.x - 140.0f) * 0.5f);
        if (ImGui::Button("Create Default Clip", ImVec2(140.0f, 26.0f))) {
            auto firstClip = std::make_shared<Animation::AnimationClip>("Default");
            firstClip->duration = 1.0f;
            animator.library->AddClip(firstClip);
            animator.SetClip("Default");
        }

        ImGui::End();
        return;
    }

    if (!animator.currentClip) {
        animator.SetClip(animator.library->GetClips().begin()->first);
    }

    auto& clip = animator.currentClip;
    ImGuiIO& io = ImGui::GetIO();

    auto ApplyCurrentTimeToScene = [&]() {
        if (registry.any_of<TransformComponent>(selectedEntity)) {
            auto& tc = registry.get<TransformComponent>(selectedEntity);
            if (!clip->positionTrack.keyframes.empty()) tc.SetPosition(clip->positionTrack.Sample(animator.currentTime));
            if (!clip->rotationTrack.keyframes.empty()) tc.SetRotation(clip->rotationTrack.Sample(animator.currentTime));
            if (!clip->scaleTrack.keyframes.empty()) tc.SetScale(clip->scaleTrack.Sample(animator.currentTime));
            tc.SetDirty();
        }

        Animation::PropertyBindingRegistry::Get().Init();
        for (const auto& propTrack : clip->propertyTracks) {
            entt::entity targetEntity = ResolvePathStatic(registry, selectedEntity, propTrack->targetPath);
            if (targetEntity != entt::null && registry.valid(targetEntity)) {
                propTrack->Apply(registry, targetEntity, animator.currentTime);
            }
        }
        };

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));

    if (ImGui::Button("Save Lib")) {
        auto path = AssetManager::Get().GetRegistry().GetPath(animator.libraryUUID);
        if (!path.empty()) {
            AnimationLibrarySerializer::Save(animator.library, path);
        }
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(140.0f);
    std::string comboPreview = animator.currentClipName.empty() ? (clip ? clip->name : "Select Clip") : animator.currentClipName;
    if (ImGui::BeginCombo("##ClipSelector", comboPreview.c_str())) {
        for (const auto& [name, c] : animator.library->GetClips()) {
            bool isSelected = (animator.currentClipName == name);
            if (ImGui::Selectable(name.c_str(), isSelected)) {
                animator.SetClip(name);
                m_SeqContext.selectedKeys.clear();
                m_SeqContext.selectedTrack = -1;
                ApplyCurrentTimeToScene();
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("+ Clip"))
        ImGui::OpenPopup("NewClipModal");

    if (ImGui::BeginPopup("NewClipModal")) {
        static char newClipNameBuf[64] = "NewClip";
        ImGui::InputText("Clip Name", newClipNameBuf, sizeof(newClipNameBuf));
        if (ImGui::Button("Create", ImVec2(100, 0))) {
            auto newClip = std::make_shared<Animation::AnimationClip>(newClipNameBuf);
            newClip->duration = 1.0f;
            animator.library->AddClip(newClip);
            animator.SetClip(newClipNameBuf);
            m_SeqContext.selectedKeys.clear();
            m_SeqContext.selectedTrack = -1;
            ApplyCurrentTimeToScene();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Edit Clip") && clip)
        ImGui::OpenPopup("EditClipPopup");

    if (ImGui::BeginPopup("EditClipPopup")) {
        ImGui::TextDisabled("Rename Clip");
        static char renameBuf[64] = "";
        static std::string lastEditingClip = "";

        if (lastEditingClip != animator.currentClipName) {
            snprintf(renameBuf, sizeof(renameBuf), "%s", animator.currentClipName.c_str());
            lastEditingClip = animator.currentClipName;
        }

        ImGui::InputText("##RenameClip", renameBuf, sizeof(renameBuf));
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            std::string newName = renameBuf;
            if (!newName.empty() && newName != animator.currentClipName) {
                std::string oldName = animator.currentClipName;
                clip->name = newName;
                animator.library->AddClip(clip);

                auto& clipsMap = const_cast<std::unordered_map<std::string, std::shared_ptr<Animation::AnimationClip>>&>(animator.library->GetClips());
                clipsMap.erase(oldName);

                animator.SetClip(newName);
                lastEditingClip = newName;
                m_SeqContext.selectedTrack = -1;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("Duplicate Clip");
        if (ImGui::Button("Duplicate", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            std::string dupName = animator.currentClipName + "_Copy";
            int counter = 1;
            while (animator.library->GetClips().find(dupName) != animator.library->GetClips().end()) {
                dupName = animator.currentClipName + "_Copy" + std::to_string(counter++);
            }

            auto duplicatedClip = std::make_shared<Animation::AnimationClip>(*clip);
            duplicatedClip->name = dupName;
            animator.library->AddClip(duplicatedClip);
            animator.SetClip(dupName);

            m_SeqContext.selectedKeys.clear();
            m_SeqContext.selectedTrack = -1;
            ApplyCurrentTimeToScene();
            ImGui::CloseCurrentPopup();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Delete Clip");
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.30f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("Delete", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            auto& clipsMap = const_cast<std::unordered_map<std::string, std::shared_ptr<Animation::AnimationClip>>&>(animator.library->GetClips());
            clipsMap.erase(animator.currentClipName);

            if (clipsMap.empty()) {
                animator.currentClip = nullptr;
                animator.currentClipName = "";
            }
            else {
                animator.SetClip(clipsMap.begin()->first);
            }

            m_SeqContext.selectedKeys.clear();
            m_SeqContext.selectedTrack = -1;
            ApplyCurrentTimeToScene();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("+ Track") && clip)
        ImGui::OpenPopup("AddPropertyTrackPopup");

    if (ImGui::BeginPopup("AddPropertyTrackPopup")) {
        ImGui::TextDisabled("Select Entity -> Component -> Property");
        ImGui::Separator();

        Animation::PropertyBindingRegistry::Get().Init();
        auto& regBinding = Animation::PropertyBindingRegistry::Get();

        std::vector<std::pair<std::string, entt::entity>> reachableEntities;
        CollectSubtreePaths(registry, selectedEntity, "", reachableEntities);

        for (const auto& [relPath, subEntity] : reachableEntities) {
            std::string headerLabel = relPath.empty() ? "Root Entity" : relPath;
            if (ImGui::BeginMenu(headerLabel.c_str())) {
                struct ValidProp { Animation::PropertyType type; std::string fullProp; };
                std::map<std::string, std::vector<ValidProp>> compMap;

                auto categorize = [&](const std::string& propStr, auto& binding, Animation::PropertyType t) {
                    if (binding.isValid(registry, subEntity)) {
                        if (relPath.empty() && propStr.find("TransformComponent.") == 0)
                            return;

                        size_t dot = propStr.find('.');
                        std::string cName = (dot != std::string::npos) ? propStr.substr(0, dot) : "General";
                        compMap[cName].push_back({ t, propStr });
                    }
                    };

                for (const auto& [p, b] : regBinding.GetBoolBindings()) categorize(p, b, Animation::PropertyType::Bool);
                for (const auto& [p, b] : regBinding.GetFloatBindings()) categorize(p, b, Animation::PropertyType::Float);
                for (const auto& [p, b] : regBinding.GetVec3Bindings()) categorize(p, b, Animation::PropertyType::Vec3);
                for (const auto& [p, b] : regBinding.GetVec4Bindings()) categorize(p, b, Animation::PropertyType::Vec4);
                for (const auto& [p, b] : regBinding.GetQuatBindings()) categorize(p, b, Animation::PropertyType::Quat);

                for (const auto& [compName, props] : compMap) {
                    if (ImGui::BeginMenu(compName.c_str())) {
                        for (const auto& prop : props) {
                            size_t dot = prop.fullProp.find('.');
                            std::string dispName = (dot != std::string::npos) ? prop.fullProp.substr(dot + 1) : prop.fullProp;

                            bool trackExists = false;
                            for (const auto& existing : clip->propertyTracks) {
                                if (existing->targetPath == relPath && existing->propertyName == prop.fullProp) {
                                    trackExists = true;
                                    break;
                                }
                            }

                            if (ImGui::MenuItem(dispName.c_str(), nullptr, false, !trackExists)) {
                                if (prop.type == Animation::PropertyType::Float) {
                                    auto track = std::make_shared<Animation::TypedPropertyTrack<float>>();
                                    track->targetPath = relPath; track->propertyName = prop.fullProp; clip->propertyTracks.push_back(track);
                                }
                                else if (prop.type == Animation::PropertyType::Vec3) {
                                    auto track = std::make_shared<Animation::TypedPropertyTrack<glm::vec3>>();
                                    track->targetPath = relPath; track->propertyName = prop.fullProp; clip->propertyTracks.push_back(track);
                                }
                                else if (prop.type == Animation::PropertyType::Vec4) {
                                    auto track = std::make_shared<Animation::TypedPropertyTrack<glm::vec4>>();
                                    track->targetPath = relPath; track->propertyName = prop.fullProp; clip->propertyTracks.push_back(track);
                                }
                                else if (prop.type == Animation::PropertyType::Quat) {
                                    auto track = std::make_shared<Animation::TypedPropertyTrack<glm::quat>>();
                                    track->targetPath = relPath; track->propertyName = prop.fullProp; clip->propertyTracks.push_back(track);
                                }
                                else if (prop.type == Animation::PropertyType::Bool) {
                                    auto track = std::make_shared<Animation::TypedPropertyTrack<bool>>();
                                    track->targetPath = relPath; track->propertyName = prop.fullProp; clip->propertyTracks.push_back(track);
                                }
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    bool canDeleteTrack = (m_SeqContext.selectedTrack >= 4 && clip && (m_SeqContext.selectedTrack - 4) < clip->propertyTracks.size());

    if (!canDeleteTrack) ImGui::BeginDisabled();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 0.8f));
    if (ImGui::Button("- Track")) {
        clip->propertyTracks.erase(clip->propertyTracks.begin() + (m_SeqContext.selectedTrack - 4));

        m_SeqContext.selectedKeys.erase(std::remove_if(m_SeqContext.selectedKeys.begin(), m_SeqContext.selectedKeys.end(),
            [&](const SelectedKey& sk) { return sk.track == m_SeqContext.selectedTrack; }),
            m_SeqContext.selectedKeys.end());

        for (auto& sk : m_SeqContext.selectedKeys) {
            if (sk.track > m_SeqContext.selectedTrack) {
                sk.track--;
            }
        }
        m_SeqContext.selectedTrack = -1;
    }
    ImGui::PopStyleColor();
    if (!canDeleteTrack) ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete selected custom property track");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImTextureID playIcon = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Play).GetID();
    ImTextureID pauseIcon = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Pause).GetID();
    ImTextureID stopIcon = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Stop).GetID();

    ImVec2 animBtnSize(46.0f, 22.0f);
    ImVec2 animIconSize(22.0f, 22.0f);
    float padX = std::floor((animBtnSize.x - animIconSize.x) * 0.5f);
    float padY = std::floor((animBtnSize.y - animIconSize.y) * 0.5f);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padX, padY));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

    const ImVec4 tintWhite = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    if (animator.isPlaying)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.30f, 0.15f, 1.0f));
        if (ImGui::ImageButton("##AnimPauseBtn", pauseIcon, animIconSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tintWhite))
        {
            animator.Pause();
        }
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.24f, 1.0f));
        if (ImGui::ImageButton("##AnimPlayBtn", playIcon, animIconSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tintWhite))
        {
            animator.Play();
        }
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(animator.isPlaying ? "Pause Animation" : "Play Animation");

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.24f, 1.0f));
    if (ImGui::ImageButton("##AnimStopBtn", stopIcon, animIconSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tintWhite))
    {
        animator.Stop();
        animator.currentTime = 0.0f;
        ApplyCurrentTimeToScene();
    }
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Stop & Rewind to Start");

    ImGui::PopStyleVar(2);

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
    float targetPosX = ImGui::GetWindowContentRegionMax().x - snapBlockWidth - ImGui::GetStyle().ScrollbarSize - 5.0f;
    if (targetPosX > currentPosX) ImGui::SameLine(targetPosX);
    else ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Display:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::Combo("##TimeMode", &m_SeqContext.timeMode, "Frames\0Seconds\0");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Snap:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    if (m_SeqContext.timeMode == 0) {
        if (m_SeqContext.snapFrames < 1) m_SeqContext.snapFrames = 1;
        ImGui::DragInt("##SnapFrames", &m_SeqContext.snapFrames, 1.0f, 1, 120, "%d f");
    }
    else {
        ImGui::DragFloat("##SnapSeconds", &m_SeqContext.snapSeconds, 0.01f, 0.01f, 10.0f, "%.2fs");
    }

    ImGui::SameLine();
    ImGui::Checkbox("Keyframes##SnapKF", &m_SeqContext.snapKeyframes);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snap keyframes while dragging");

    ImGui::SameLine();
    ImGui::Checkbox("Cursor##SnapCur", &m_SeqContext.snapCursor);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snap timeline cursor while scrubbing");

    ImGui::PopStyleVar();
    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 0));

    ImGui::AlignTextToFramePadding();
    if (m_SeqContext.timeMode == 0)
        ImGui::TextDisabled("%d f", (int)std::round(animator.currentTime * SEQUENCE_FPS));
    else
        ImGui::TextDisabled("%.2fs", animator.currentTime);

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.65f, 0.05f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.78f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.55f, 0.0f, 1.0f));
    if (ImGui::Button("+ Event")) {
        clip->AddEvent(animator.currentTime, "NewEvent", "");
        m_SeqContext.selectedKeys.clear();
        m_SeqContext.selectedKeys.push_back({ 0, animator.currentTime });
    }
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Animation Event at current playhead time");

    if (registry.any_of<TransformComponent>(selectedEntity)) {
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        auto& tc = registry.get<TransformComponent>(selectedEntity);

        auto RecordPos = [&]() {
            clip->positionTrack.AddKeyframe(animator.currentTime, tc.GetPosition());
            if (animator.currentTime > clip->duration) clip->duration = animator.currentTime;
            };
        auto RecordRot = [&]() {
            clip->rotationTrack.AddKeyframe(animator.currentTime, tc.GetRotation());
            if (animator.currentTime > clip->duration) clip->duration = animator.currentTime;
            };
        auto RecordScale = [&]() {
            clip->scaleTrack.AddKeyframe(animator.currentTime, tc.GetScale());
            if (animator.currentTime > clip->duration) clip->duration = animator.currentTime;
            };

        auto RecordAllProperties = [&]() {
            RecordPos(); RecordRot(); RecordScale();
            for (size_t p = 0; p < clip->propertyTracks.size(); ++p) {
                auto pTrack = clip->propertyTracks[p];
                entt::entity targetEntity = ResolvePathStatic(registry, selectedEntity, pTrack->targetPath);
                if (targetEntity == entt::null || !registry.valid(targetEntity)) continue;

                if (pTrack->propertyName == "TransformComponent.position" && registry.any_of<TransformComponent>(targetEntity)) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack);
                    t->track.AddKeyframe(animator.currentTime, registry.get<TransformComponent>(targetEntity).GetPosition());
                }
                else if (pTrack->propertyName == "TransformComponent.rotation" && registry.any_of<TransformComponent>(targetEntity)) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pTrack);
                    t->track.AddKeyframe(animator.currentTime, registry.get<TransformComponent>(targetEntity).GetRotation());
                }
                else if (pTrack->propertyName == "TransformComponent.scale" && registry.any_of<TransformComponent>(targetEntity)) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack);
                    t->track.AddKeyframe(animator.currentTime, registry.get<TransformComponent>(targetEntity).GetScale());
                }
            }
            if (animator.currentTime > clip->duration) clip->duration = animator.currentTime;
            };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.50f, 0.85f, 0.8f));
        if (ImGui::Button("Key All (K)")) {
            RecordAllProperties();
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Record all tracks at current playhead time (Hotkey: K)");

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

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_K)) {
            RecordAllProperties();
        }
    }

    auto SetSelectedEase = [&](Animation::EaseType newEase) {
        auto applyToTrack = [&](auto& track) {
            for (auto& kf : track.keyframes) {
                for (const auto& sk : m_SeqContext.selectedKeys) {
                    if (std::abs(kf.time - sk.time) < 0.001f) {
                        kf.ease = newEase;
                        break;
                    }
                }
            }
            };

        for (const auto& sk : m_SeqContext.selectedKeys) {
            if (sk.track == 1) applyToTrack(clip->positionTrack);
            else if (sk.track == 2) applyToTrack(clip->rotationTrack);
            else if (sk.track == 3) applyToTrack(clip->scaleTrack);
            else if (sk.track >= 4) {
                size_t propIdx = sk.track - 4;
                if (propIdx < clip->propertyTracks.size()) {
                    auto pTrack = clip->propertyTracks[propIdx];
                    if (pTrack->GetType() == Animation::PropertyType::Float) applyToTrack(std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(pTrack)->track);
                    else if (pTrack->GetType() == Animation::PropertyType::Vec3) applyToTrack(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack)->track);
                    else if (pTrack->GetType() == Animation::PropertyType::Vec4) applyToTrack(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(pTrack)->track);
                    else if (pTrack->GetType() == Animation::PropertyType::Quat) applyToTrack(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pTrack)->track);
                    else if (pTrack->GetType() == Animation::PropertyType::Bool) applyToTrack(std::static_pointer_cast<Animation::TypedPropertyTrack<bool>>(pTrack)->track);
                }
            }
        }
        ApplyCurrentTimeToScene();
        };

    bool hasEaseSelection = false;
    for (const auto& sk : m_SeqContext.selectedKeys) {
        if (sk.track >= 1) { hasEaseSelection = true; break; }
    }

    if (hasEaseSelection) {
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Easing:");
        ImGui::SameLine();

        Animation::EaseType currentEase = Animation::EaseType::Linear;
        for (const auto& sk : m_SeqContext.selectedKeys) {
            if (sk.track >= 1) {
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
                else {
                    size_t propIdx = sk.track - 4;
                    if (propIdx < clip->propertyTracks.size()) {
                        auto pTrack = clip->propertyTracks[propIdx];
                        if (pTrack->GetType() == Animation::PropertyType::Float) findCurrentEase(std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(pTrack)->track);
                        else if (pTrack->GetType() == Animation::PropertyType::Vec3) findCurrentEase(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack)->track);
                        else if (pTrack->GetType() == Animation::PropertyType::Vec4) findCurrentEase(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(pTrack)->track);
                        else if (pTrack->GetType() == Animation::PropertyType::Quat) findCurrentEase(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pTrack)->track);
                        else if (pTrack->GetType() == Animation::PropertyType::Bool) findCurrentEase(std::static_pointer_cast<Animation::TypedPropertyTrack<bool>>(pTrack)->track);
                    }
                }
                break;
            }
        }

        int selectedEaseInt = static_cast<int>(currentEase);
        ImGui::SetNextItemWidth(125.0f);
        if (ImGui::BeginCombo("##EasingSelector", k_EaseNames[selectedEaseInt])) {
            for (int i = 0; i < 6; i++) {
                bool isSelected = (selectedEaseInt == i);
                if (ImGui::Selectable(k_EaseNames[i], isSelected)) {
                    SetSelectedEase(static_cast<Animation::EaseType>(i));
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (m_SeqContext.selectedKeys.size() == 1) {
            auto& sk = m_SeqContext.selectedKeys[0];
            if (sk.track >= 1) {
                ImGui::SameLine();
                ImGui::TextDisabled("|");
                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Value:");
                ImGui::SameLine();

                ImGui::SetNextItemWidth(200.0f);

                auto applyValueEdit = [&](auto& track) {
                    for (auto& kf : track.keyframes) {
                        if (std::abs(kf.time - sk.time) < 0.001f) {
                            if constexpr (std::is_same_v<std::decay_t<decltype(kf.value)>, float>) {
                                if (ImGui::DragFloat("##ValF", &kf.value, 0.01f, 0.0f, 0.0f, "%.2f")) ApplyCurrentTimeToScene();
                            }
                            else if constexpr (std::is_same_v<std::decay_t<decltype(kf.value)>, glm::vec3>) {
                                if (ImGui::DragFloat3("##Val3", &kf.value.x, 0.01f, 0.0f, 0.0f, "%.2f")) ApplyCurrentTimeToScene();
                            }
                            else if constexpr (std::is_same_v<std::decay_t<decltype(kf.value)>, glm::vec4>) {
                                if (ImGui::DragFloat4("##Val4", &kf.value.x, 0.01f, 0.0f, 0.0f, "%.2f")) ApplyCurrentTimeToScene();
                            }
                            else if constexpr (std::is_same_v<std::decay_t<decltype(kf.value)>, glm::quat>) {
                                glm::vec3 euler = glm::degrees(glm::eulerAngles(kf.value));
                                if (ImGui::DragFloat3("##ValQ", &euler.x, 0.5f, 0.0f, 0.0f, "%.2f")) {
                                    kf.value = glm::quat(glm::radians(euler));
                                    ApplyCurrentTimeToScene();
                                }
                            }
                            else if constexpr (std::is_same_v<std::decay_t<decltype(kf.value)>, bool>) {
                                if (ImGui::Checkbox("##ValB", &kf.value)) ApplyCurrentTimeToScene();
                            }
                            break;
                        }
                    }
                    };

                if (sk.track == 1) applyValueEdit(clip->positionTrack);
                else if (sk.track == 2) applyValueEdit(clip->rotationTrack);
                else if (sk.track == 3) applyValueEdit(clip->scaleTrack);
                else {
                    size_t pIdx = sk.track - 4;
                    if (pIdx < clip->propertyTracks.size()) {
                        auto pt = clip->propertyTracks[pIdx];
                        if (pt->GetType() == Animation::PropertyType::Float) applyValueEdit(std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(pt)->track);
                        else if (pt->GetType() == Animation::PropertyType::Vec3) applyValueEdit(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pt)->track);
                        else if (pt->GetType() == Animation::PropertyType::Vec4) applyValueEdit(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(pt)->track);
                        else if (pt->GetType() == Animation::PropertyType::Quat) applyValueEdit(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pt)->track);
                        else if (pt->GetType() == Animation::PropertyType::Bool) applyValueEdit(std::static_pointer_cast<Animation::TypedPropertyTrack<bool>>(pt)->track);
                    }
                }
            }
        }
    }

    if (m_SeqContext.selectedKeys.size() == 1 && m_SeqContext.selectedKeys[0].track == 0) {
        float selectedEventTime = m_SeqContext.selectedKeys[0].time;
        Animation::AnimationEvent* selectedEv = nullptr;
        for (auto& ev : clip->eventTrack) {
            if (std::abs(ev.time - selectedEventTime) < 0.001f) {
                selectedEv = &ev;
                break;
            }
        }

        if (selectedEv) {
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
            if (ImGui::InputText("##QuickEvName", nameBuf, sizeof(nameBuf))) {
                selectedEv->name = nameBuf;
            }

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Param:");
            ImGui::SameLine();

            char paramBuf[64];
            snprintf(paramBuf, sizeof(paramBuf), "%s", selectedEv->parameter.c_str());
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputText("##QuickEvParam", paramBuf, sizeof(paramBuf))) {
                selectedEv->parameter = paramBuf;
            }
        }
    }

    ImGui::PopStyleVar(2);
    ImGui::Separator();

    if (ImGui::BeginPopup("KeyframeContextMenu")) {
        ImGui::TextDisabled("Set Easing (%zu selected)", m_SeqContext.selectedKeys.size());
        ImGui::Separator();
        for (int i = 0; i < 6; i++) {
            if (ImGui::MenuItem(k_EaseNames[i])) {
                SetSelectedEase(static_cast<Animation::EaseType>(i));
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("EventEditPopup")) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
        ImGui::Text("Edit Animation Event");
        ImGui::PopStyleColor();
        ImGui::Separator();

        if (!m_SeqContext.selectedKeys.empty() && m_SeqContext.selectedKeys[0].track == 0) {
            float selTime = m_SeqContext.selectedKeys[0].time;
            for (auto& ev : clip->eventTrack) {
                if (std::abs(ev.time - selTime) < 0.001f) {
                    char nameBuf[64];
                    snprintf(nameBuf, sizeof(nameBuf), "%s", ev.name.c_str());
                    if (ImGui::InputText("Function Name", nameBuf, sizeof(nameBuf)))
                        ev.name = nameBuf;

                    char paramBuf[128];
                    snprintf(paramBuf, sizeof(paramBuf), "%s", ev.parameter.c_str());
                    if (ImGui::InputText("Parameter", paramBuf, sizeof(paramBuf)))
                        ev.parameter = paramBuf;

                    ImGui::Spacing();
                    if (ImGui::Button("Delete Event", ImVec2(120, 0))) {
                        clip->eventTrack.erase(std::remove_if(clip->eventTrack.begin(), clip->eventTrack.end(),
                            [&](const Animation::AnimationEvent& e) { return std::abs(e.time - selTime) < 0.001f; }),
                            clip->eventTrack.end());
                        m_SeqContext.selectedKeys.clear();
                        ApplyCurrentTimeToScene();
                        ImGui::CloseCurrentPopup();
                    }
                    break;
                }
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !m_SeqContext.selectedKeys.empty()) {
            auto deleteFromTrack = [&](auto& trk, int tIdx) {
                trk.keyframes.erase(std::remove_if(trk.keyframes.begin(), trk.keyframes.end(),
                    [&](const auto& kf) {
                        for (const auto& sk : m_SeqContext.selectedKeys) {
                            if (sk.track == tIdx && std::abs(kf.time - sk.time) < 0.001f) {
                                return true;
                            }
                        }
                        return false;
                    }),
                    trk.keyframes.end());
                };

            deleteFromTrack(clip->positionTrack, 1);
            deleteFromTrack(clip->rotationTrack, 2);
            deleteFromTrack(clip->scaleTrack, 3);

            for (size_t p = 0; p < clip->propertyTracks.size(); ++p) {
                int pTrackIdx = 4 + static_cast<int>(p);
                auto pTrack = clip->propertyTracks[p];
                if (pTrack->GetType() == Animation::PropertyType::Float) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(pTrack);
                    deleteFromTrack(t->track, pTrackIdx);
                }
                else if (pTrack->GetType() == Animation::PropertyType::Vec3) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack);
                    deleteFromTrack(t->track, pTrackIdx);
                }
                else if (pTrack->GetType() == Animation::PropertyType::Vec4) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(pTrack);
                    deleteFromTrack(t->track, pTrackIdx);
                }
                else if (pTrack->GetType() == Animation::PropertyType::Quat) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pTrack);
                    deleteFromTrack(t->track, pTrackIdx);
                }
                else if (pTrack->GetType() == Animation::PropertyType::Bool) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<bool>>(pTrack);
                    deleteFromTrack(t->track, pTrackIdx);
                }
            }

            clip->eventTrack.erase(std::remove_if(clip->eventTrack.begin(), clip->eventTrack.end(),
                [&](const auto& ev) {
                    for (const auto& sk : m_SeqContext.selectedKeys) {
                        if (sk.track == 0 && std::abs(ev.time - sk.time) < 0.001f) {
                            return true;
                        }
                    }
                    return false;
                }),
                clip->eventTrack.end());

            m_SeqContext.selectedKeys.clear();
            ApplyCurrentTimeToScene();
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C) && !m_SeqContext.selectedKeys.empty()) {
            m_SeqContext.clipboard.clear();
            float minTime = FLT_MAX;
            for (const auto& sk : m_SeqContext.selectedKeys) {
                if (sk.time < minTime) minTime = sk.time;
            }

            auto copyToBoard = [&](auto& trk, int tIdx, auto assignFunc) {
                for (const auto& sk : m_SeqContext.selectedKeys) {
                    if (sk.track == tIdx) {
                        for (const auto& kf : trk.keyframes) {
                            if (std::abs(kf.time - sk.time) < 0.001f) {
                                ClipboardKey ck;
                                ck.track = tIdx;
                                ck.timeOffset = kf.time - minTime;
                                ck.ease = kf.ease;
                                assignFunc(ck, kf.value);
                                m_SeqContext.clipboard.push_back(ck);
                                break;
                            }
                        }
                    }
                }
                };

            copyToBoard(clip->positionTrack, 1, [](ClipboardKey& ck, const glm::vec3& val) { ck.v3 = val; });
            copyToBoard(clip->rotationTrack, 2, [](ClipboardKey& ck, const glm::quat& val) { ck.q = val; });
            copyToBoard(clip->scaleTrack, 3, [](ClipboardKey& ck, const glm::vec3& val) { ck.v3 = val; });

            for (size_t p = 0; p < clip->propertyTracks.size(); ++p) {
                int pTrackIdx = 4 + static_cast<int>(p);
                auto pTrack = clip->propertyTracks[p];
                if (pTrack->GetType() == Animation::PropertyType::Float) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(pTrack);
                    copyToBoard(t->track, pTrackIdx, [](ClipboardKey& ck, float val) { ck.fVal = val; });
                }
                else if (pTrack->GetType() == Animation::PropertyType::Vec3) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack);
                    copyToBoard(t->track, pTrackIdx, [](ClipboardKey& ck, const glm::vec3& val) { ck.v3 = val; });
                }
                else if (pTrack->GetType() == Animation::PropertyType::Vec4) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(pTrack);
                    copyToBoard(t->track, pTrackIdx, [](ClipboardKey& ck, const glm::vec4& val) { ck.v4 = val; });
                }
                else if (pTrack->GetType() == Animation::PropertyType::Quat) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pTrack);
                    copyToBoard(t->track, pTrackIdx, [](ClipboardKey& ck, const glm::quat& val) { ck.q = val; });
                }
                else if (pTrack->GetType() == Animation::PropertyType::Bool) {
                    auto t = std::static_pointer_cast<Animation::TypedPropertyTrack<bool>>(pTrack);
                    copyToBoard(t->track, pTrackIdx, [](ClipboardKey& ck, bool val) { ck.bVal = val; });
                }
            }

            for (const auto& sk : m_SeqContext.selectedKeys) {
                if (sk.track == 0) {
                    for (const auto& ev : clip->eventTrack) {
                        if (std::abs(ev.time - sk.time) < 0.001f) {
                            ClipboardKey ck;
                            ck.track = 0;
                            ck.timeOffset = ev.time - minTime;
                            ck.eventName = ev.name;
                            ck.eventParam = ev.parameter;
                            m_SeqContext.clipboard.push_back(ck);
                            break;
                        }
                    }
                }
            }
        }

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V) && !m_SeqContext.clipboard.empty()) {
            m_SeqContext.selectedKeys.clear();
            float pasteTime = animator.currentTime;

            for (auto& ck : m_SeqContext.clipboard) {
                float t = pasteTime + ck.timeOffset;
                if (ck.track == 0) clip->AddEvent(t, ck.eventName, ck.eventParam);
                else if (ck.track == 1) clip->positionTrack.AddKeyframe(t, ck.v3, ck.ease);
                else if (ck.track == 2) clip->rotationTrack.AddKeyframe(t, ck.q, ck.ease);
                else if (ck.track == 3) clip->scaleTrack.AddKeyframe(t, ck.v3, ck.ease);
                else {
                    size_t propIdx = ck.track - 4;
                    if (propIdx < clip->propertyTracks.size()) {
                        auto pTrack = clip->propertyTracks[propIdx];
                        if (pTrack->GetType() == Animation::PropertyType::Float) {
                            auto trk = std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(pTrack);
                            trk->track.AddKeyframe(t, ck.fVal, ck.ease);
                        }
                        else if (pTrack->GetType() == Animation::PropertyType::Vec3) {
                            auto trk = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack);
                            trk->track.AddKeyframe(t, ck.v3, ck.ease);
                        }
                        else if (pTrack->GetType() == Animation::PropertyType::Vec4) {
                            auto trk = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(pTrack);
                            trk->track.AddKeyframe(t, ck.v4, ck.ease);
                        }
                        else if (pTrack->GetType() == Animation::PropertyType::Quat) {
                            auto trk = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pTrack);
                            trk->track.AddKeyframe(t, ck.q, ck.ease);
                        }
                        else if (pTrack->GetType() == Animation::PropertyType::Bool) {
                            auto trk = std::static_pointer_cast<Animation::TypedPropertyTrack<bool>>(pTrack);
                            trk->track.AddKeyframe(t, ck.bVal, ck.ease);
                        }
                    }
                }

                if (t > clip->duration) clip->duration = t;
                m_SeqContext.selectedKeys.push_back({ ck.track, t });
            }
            ApplyCurrentTimeToScene();
        }
    }

    AnimationSequenceAdapter sequence(clip, &animator, &registry, selectedEntity, m_SeqContext);
    m_CurrentFrame = static_cast<int>(animator.currentTime * SEQUENCE_FPS);
    int oldFrame = m_CurrentFrame;

    int sequencerFlags = ImSequencer::SEQUENCER_EDIT_NONE;
    if (!ImGui::IsAnyItemActive()) {
        sequencerFlags |= ImSequencer::SEQUENCER_CHANGE_FRAME;
    }

    ImVec2 timelineRegionStart = ImGui::GetCursorScreenPos();

    ImSequencer::Sequencer(
        &sequence,
        &m_CurrentFrame,
        &m_Expanded,
        &m_SeqContext.selectedTrack,
        &m_FirstFrame,
        sequencerFlags
    );

    ImVec2 timelineRegionEnd = ImVec2(timelineRegionStart.x + ImGui::GetContentRegionAvail().x, ImGui::GetCursorScreenPos().y);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
        ImRect timelineRect(timelineRegionStart, timelineRegionEnd);
        if (!timelineRect.Contains(io.MousePos) && !ImGui::IsAnyItemHovered()) {
            m_SeqContext.selectedTrack = -1;
            m_SeqContext.selectedKeys.clear();
        }
    }

    if (m_SeqContext.isBoxSelecting) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 bMin = ImMin(m_SeqContext.boxSelectStart, io.MousePos);
        ImVec2 bMax = ImMax(m_SeqContext.boxSelectStart, io.MousePos);
        dl->AddRectFilled(bMin, bMax, IM_COL32(0, 150, 255, 60));
        dl->AddRect(bMin, bMax, IM_COL32(0, 200, 255, 220), 0.0f, 0, 1.5f);
    }

    if (m_CurrentFrame != oldFrame && !m_SeqContext.isDragging && !ImGui::IsAnyItemActive()) {
        float newTime = static_cast<float>(m_CurrentFrame) / SEQUENCE_FPS;
        if (m_SeqContext.snapCursor) {
            float step = (m_SeqContext.timeMode == 0) ? (static_cast<float>(m_SeqContext.snapFrames) / SEQUENCE_FPS) : m_SeqContext.snapSeconds;
            if (step > 0.0f)
                newTime = std::round(newTime / step) * step;
            newTime = std::clamp(newTime, 0.0f, clip->duration);
            m_CurrentFrame = static_cast<int>(newTime * SEQUENCE_FPS);
        }
        animator.currentTime = newTime;
        animator.Pause();
        ApplyCurrentTimeToScene();
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (m_SeqContext.isDragging) {
            auto eraseOldKey = [&](auto& trk, int tIdx, float origTime) {
                trk.keyframes.erase(std::remove_if(trk.keyframes.begin(), trk.keyframes.end(),
                    [&](const auto& kf) { return std::abs(kf.time - origTime) < 0.001f; }),
                    trk.keyframes.end());
                };

            for (const auto& ghost : m_SeqContext.dragGhosts) {
                if (ghost.track == 0) {
                    clip->eventTrack.erase(std::remove_if(clip->eventTrack.begin(), clip->eventTrack.end(),
                        [&](const auto& ev) { return std::abs(ev.time - ghost.origTime) < 0.001f; }),
                        clip->eventTrack.end());
                }
                else if (ghost.track == 1) eraseOldKey(clip->positionTrack, 1, ghost.origTime);
                else if (ghost.track == 2) eraseOldKey(clip->rotationTrack, 2, ghost.origTime);
                else if (ghost.track == 3) eraseOldKey(clip->scaleTrack, 3, ghost.origTime);
                else {
                    size_t propIdx = ghost.track - 4;
                    if (propIdx < clip->propertyTracks.size()) {
                        auto pTrack = clip->propertyTracks[propIdx];
                        if (pTrack->GetType() == Animation::PropertyType::Float) eraseOldKey(std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(pTrack)->track, ghost.track, ghost.origTime);
                        else if (pTrack->GetType() == Animation::PropertyType::Vec3) eraseOldKey(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack)->track, ghost.track, ghost.origTime);
                        else if (pTrack->GetType() == Animation::PropertyType::Vec4) eraseOldKey(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(pTrack)->track, ghost.track, ghost.origTime);
                        else if (pTrack->GetType() == Animation::PropertyType::Quat) eraseOldKey(std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pTrack)->track, ghost.track, ghost.origTime);
                        else if (pTrack->GetType() == Animation::PropertyType::Bool) eraseOldKey(std::static_pointer_cast<Animation::TypedPropertyTrack<bool>>(pTrack)->track, ghost.track, ghost.origTime);
                    }
                }
            }

            m_SeqContext.selectedKeys.clear();
            for (const auto& ghost : m_SeqContext.dragGhosts) {
                float newTime = m_SeqContext.CalculateNewTime(ghost.origTime, clip->duration);

                if (ghost.track == 0) clip->AddEvent(newTime, ghost.eventName, ghost.eventParam);
                else if (ghost.track == 1) clip->positionTrack.AddKeyframe(newTime, ghost.v3, ghost.ease);
                else if (ghost.track == 2) clip->rotationTrack.AddKeyframe(newTime, ghost.q, ghost.ease);
                else if (ghost.track == 3) clip->scaleTrack.AddKeyframe(newTime, ghost.v3, ghost.ease);
                else {
                    size_t propIdx = ghost.track - 4;
                    if (propIdx < clip->propertyTracks.size()) {
                        auto pTrack = clip->propertyTracks[propIdx];
                        if (pTrack->GetType() == Animation::PropertyType::Float) {
                            std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(pTrack)->track.AddKeyframe(newTime, ghost.fVal, ghost.ease);
                        }
                        else if (pTrack->GetType() == Animation::PropertyType::Vec3) {
                            std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(pTrack)->track.AddKeyframe(newTime, ghost.v3, ghost.ease);
                        }
                        else if (pTrack->GetType() == Animation::PropertyType::Vec4) {
                            std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(pTrack)->track.AddKeyframe(newTime, ghost.v4, ghost.ease);
                        }
                        else if (pTrack->GetType() == Animation::PropertyType::Quat) {
                            std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(pTrack)->track.AddKeyframe(newTime, ghost.q, ghost.ease);
                        }
                        else if (pTrack->GetType() == Animation::PropertyType::Bool) {
                            std::static_pointer_cast<Animation::TypedPropertyTrack<bool>>(pTrack)->track.AddKeyframe(newTime, ghost.bVal, ghost.ease);
                        }
                    }
                }

                m_SeqContext.selectedKeys.push_back({ ghost.track, newTime });
            }

            m_SeqContext.isDragging = false;
            m_SeqContext.currentDragDelta = 0.0f;
            m_SeqContext.dragGhosts.clear();
            m_SeqContext.dragMode = DragMode::Translate;
            ApplyCurrentTimeToScene();
        }

        if (m_SeqContext.isBoxSelecting) {
            m_SeqContext.isBoxSelecting = false;
        }
    }

    ImGui::End();
}

}