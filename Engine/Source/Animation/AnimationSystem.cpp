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

    for (auto entity : view)
    {
        auto& animator = view.get<AnimatorComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        if (!animator.isPlaying || !animator.currentClip || animator.currentClip->duration <= 0.0f)
            continue;

        const auto& clip = animator.currentClip;
        float duration = clip->duration;
        float dt = Time::GetDeltaTime() * animator.playbackSpeed;

        float prevTime = animator.currentTime;
        animator.currentTime += dt;
        float currTime = animator.currentTime;

        Animation::LoopMode loopMode = clip->loopMode;
        bool looped = false;

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
            animator.currentTime = Animation::PingPong(animator.currentTime, duration);
        }
        else // NONE
        {
            if (animator.currentTime >= duration || (animator.playbackSpeed < 0.0f && animator.currentTime <= 0.0f))
            {
                animator.currentTime = std::clamp(animator.currentTime, 0.0f, duration);
                animator.isPlaying = false;
            }
        }

        if (!clip->eventTrack.empty() && m_Scene.GetRegistry().any_of<ScriptComponent>(entity))
        {
            auto& sc = m_Scene.GetRegistry().get<ScriptComponent>(entity);
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
                        sol::protected_function_result result = sc.OnAnimationEvent(sc.luaInstance, ev.name, ev.parameter);
                        if (!result.valid())
                        {
                            sol::error err = result;
                            LOG_ERROR("Lua OnAnimationEvent error: {}", err.what());
                        }
                    }
                }
            }
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