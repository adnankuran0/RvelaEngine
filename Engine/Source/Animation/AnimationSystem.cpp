#include "rvelapch.h"
#include "Animation/AnimationSystem.h"
#include "Animation/PropertyBindingRegistry.h"
#include "Animation/BonePropertyTrack.h"
#include "Scene/Components/AnimatorComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/SkeletonComponent.h"
#include "Scene/Components/ScriptComponent.h"
#include "Scene/Components/SceneTreeComponent.h"
#include "Scene/Components/TagComponent.h"
#include "Scene/Scene.h"
#include "Core/Time.h"
#include "Math/RvelaMath.h"
#include <glm/gtx/transform.hpp>
#include <cmath>
#include <sstream>

namespace rv {

void AnimationSystem::OnStart()
{
    auto view = m_Scene.GetRegistry().view<AnimatorComponent>();
    for (auto entity : view)
    {
        auto& animator = view.get<AnimatorComponent>(entity);
        if (animator.autoplay)
            animator.Play();
    }
}

entt::entity AnimationSystem::ResolveAnimPath(entt::entity root, const std::string& path)
{
    if (path.empty()) return root;

    entt::entity current = root;
    std::istringstream stream(path);
    std::string childName;
    auto& reg = m_Scene.GetRegistry();

    while (std::getline(stream, childName, '/'))
    {
        if (!reg.valid(current) || !reg.any_of<SceneTreeComponent>(current))
            return entt::null;

        auto& tree = reg.get<SceneTreeComponent>(current);
        bool found = false;

        for (auto child : tree.children)
        {
            if (reg.valid(child) && reg.any_of<TagComponent>(child))
            {
                if (reg.get<TagComponent>(child).tag == childName)
                {
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

void AnimationSystem::UpdateSkeletonBones(SkeletonComponent& skel)
{
    if (!skel.isInitialized)
        skel.InitFromAsset();

    auto skeletonAsset = skel.GetSkeleton();
    if (!skeletonAsset || !skeletonAsset->IsValid())
        return;

    uint32_t boneCount = skeletonAsset->GetBoneCount();
    if (boneCount == 0)
        return;

    if (skel.modelSpaceMatrices.size() != boneCount) skel.modelSpaceMatrices.resize(boneCount);
    if (skel.skinningPalette.size() != boneCount) skel.skinningPalette.resize(boneCount);

    for (uint32_t i = 0; i < boneCount; ++i)
    {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), skel.localPositions[i]);
        glm::mat4 rotation = glm::mat4_cast(skel.localRotations[i]);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), skel.localScales[i]);

        glm::mat4 localTransform = translation * rotation * scale;

        int32_t parentIndex = skeletonAsset->GetParentIndex(i);
        if (parentIndex >= 0 && parentIndex < static_cast<int32_t>(i))
        {
            skel.modelSpaceMatrices[i] = skel.modelSpaceMatrices[parentIndex] * localTransform;
        }
        else
        {
            skel.modelSpaceMatrices[i] = localTransform;
        }

        skel.skinningPalette[i] = skel.modelSpaceMatrices[i] * skeletonAsset->GetInverseBindMatrix(i);
    }
}

void AnimationSystem::Update()
{
    auto view = m_Scene.GetRegistry().view<AnimatorComponent, TransformComponent>();
    auto& reg = m_Scene.GetRegistry();

    Animation::PropertyBindingRegistry::Get().Init();

    for (auto entity : view)
    {
        if (!reg.valid(entity))
            continue;

        auto& animator = view.get<AnimatorComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        if (!animator.isPlaying || !animator.currentClip || animator.currentClip->duration <= 0.0f)
            continue;

        const auto& clip = animator.currentClip;
        float duration = clip->duration;
        float dt = Time::GetDeltaTime() * animator.playbackSpeed;

        if (!animator.isStarted)
        {
            animator.isStarted = true;
            m_EventQueue.push_back({ entity, Animation::EventType::Started, animator.currentClipName, "", "" });
        }

        float prevTime = animator.currentTime;
        animator.currentTime += dt;
        float currTime = animator.currentTime;

        Animation::LoopMode loopMode = clip->loopMode;
        bool looped = false;
        bool animationJustEnded = false;

        if (loopMode == Animation::LoopMode::LINEAR)
        {
            if (animator.currentTime >= duration)
            {
                looped = true;
                animator.currentTime = std::fmod(animator.currentTime, duration);
            }
            else if (animator.currentTime < 0.0f)
            {
                looped = true;
                animator.currentTime = duration + std::fmod(animator.currentTime, duration);
            }
        }
        else if (loopMode == Animation::LoopMode::PINGPONG)
        {
            if (animator.playbackSpeed > 0.0f && animator.currentTime >= duration)
            {
                animator.currentTime = duration - (animator.currentTime - duration);
                animator.playbackSpeed = -animator.playbackSpeed;
                looped = true;
            }
            else if (animator.playbackSpeed < 0.0f && animator.currentTime <= 0.0f)
            {
                animator.currentTime = -animator.currentTime;
                animator.playbackSpeed = -animator.playbackSpeed;
                looped = true;
            }

            animator.currentTime = std::clamp(animator.currentTime, 0.0f, duration);
        }
        else
        {
            if (animator.currentTime >= duration || (animator.playbackSpeed < 0.0f && animator.currentTime <= 0.0f))
            {
                animator.currentTime = std::clamp(animator.currentTime, 0.0f, duration);
                animator.isPlaying = false;
                animationJustEnded = true;
            }
        }

        if (looped)
        {
            m_EventQueue.push_back({ entity, Animation::EventType::Looped, animator.currentClipName, "", "" });
        }

        if (!clip->eventTrack.empty() && reg.any_of<ScriptComponent>(entity))
        {
            auto& sc = reg.get<ScriptComponent>(entity);
            if (sc.luaInstance.valid() && sc.OnAnimationEvent.valid())
            {
                for (const auto& ev : clip->eventTrack)
                {
                    bool triggered = false;

                    if (!looped)
                    {
                        if (dt >= 0.0f)
                            triggered = (ev.time > prevTime && ev.time <= currTime);
                        else
                            triggered = (ev.time < prevTime && ev.time >= currTime);
                    }
                    else
                    {
                        triggered = (ev.time > prevTime || ev.time <= animator.currentTime);
                    }

                    if (triggered)
                    {
                        m_EventQueue.push_back({ entity, Animation::EventType::Triggered, animator.currentClipName, ev.name, ev.parameter });
                    }
                }
            }
        }

        if (!reg.valid(entity))
            continue;

        if (animationJustEnded)
        {
            m_EventQueue.push_back({ entity, Animation::EventType::Finished, animator.currentClipName, "", "" });
        }

        float sampleTime = animator.currentTime;

        if (!clip->positionTrack.keyframes.empty())
        {
            transform.SetPosition(clip->positionTrack.Sample(sampleTime));
            transform.SetDirty();
        }

        if (!clip->rotationTrack.keyframes.empty())
        {
            glm::quat sampledRot = clip->rotationTrack.Sample(sampleTime);
            transform.SetRotation(sampledRot);
            transform.SetDirty();
        }

        if (!clip->scaleTrack.keyframes.empty())
        {
            transform.SetScale(clip->scaleTrack.Sample(sampleTime));
            transform.SetDirty();
        }

        for (const auto& propTrack : clip->propertyTracks)
        {
            auto type = propTrack->GetType();

            if (type == Animation::PropertyType::BoneVec3 || type == Animation::PropertyType::BoneQuat)
            {
                propTrack->Apply(reg, entity, sampleTime);
            }
            else
            {
                entt::entity targetEntity = ResolveAnimPath(entity, propTrack->targetPath);
                if (targetEntity != entt::null && reg.valid(targetEntity))
                {
                    propTrack->Apply(reg, targetEntity, sampleTime);
                }
            }
        }
    }

    auto skeletonView = reg.view<SkeletonComponent>();
    for (auto entity : skeletonView)
    {
        auto& skel = skeletonView.get<SkeletonComponent>(entity);
        UpdateSkeletonBones(skel);
    }
}

}