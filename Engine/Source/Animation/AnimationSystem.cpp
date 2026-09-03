#include "rvelapch.h"
#include "Animation/AnimationSystem.h"
#include "Scene/Components/AnimatorComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/ScriptComponent.h"
#include "Scene/Scene.h"
#include "Core/Time.h"
#include <cmath>

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

void AnimationSystem::Update()
{
    auto view = m_Scene.GetRegistry().view<AnimatorComponent, TransformComponent>();
    auto& reg = m_Scene.GetRegistry();

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
            m_EventQueue.push_back({ entity, Animation::EventType::STARTED, animator.currentClipName, "", "" });
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
        else // NONE
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
            m_EventQueue.push_back({ entity, Animation::EventType::LOOPED, animator.currentClipName, "", "" });
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
                        m_EventQueue.push_back({ entity, Animation::EventType::TRIGGERED, animator.currentClipName, ev.name, ev.parameter });
                    }
                }
            }
        }

        if (!reg.valid(entity))
            continue;

        if (animationJustEnded)
        {
            m_EventQueue.push_back({ entity, Animation::EventType::FINISHED, animator.currentClipName, "", "" });
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
    }
}

}