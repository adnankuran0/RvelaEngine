#include "rvelapch.h"
#include "Animation/AnimationSystem.h"
#include "Scene/Components/AnimatorComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"
#include "Core/Time.h"

namespace rv {

void AnimationSystem::Update()
{
    auto view = m_Scene.GetRegistry().view<AnimatorComponent, TransformComponent>();

    for (auto entity : view)
    {
        auto& animator = view.get<AnimatorComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        if (!animator.isPlaying || !animator.currentClip || animator.currentClip->duration <= 0.0f)
            continue;

        animator.currentTime += Time::GetDeltaTime() * animator.playbackSpeed;

        float duration = animator.currentClip->duration;
        Animation::LoopMode loopMode = animator.currentClip->loopMode;
        float sampleTime = animator.currentTime;

        if (loopMode == Animation::LoopMode::LINEAR)
        {
            animator.currentTime = std::fmod(animator.currentTime, duration);
            if (animator.currentTime < 0.0f)
                animator.currentTime += duration;

            sampleTime = animator.currentTime;
        }
        else if (loopMode == Animation::LoopMode::PINGPONG)
        {
            sampleTime = Animation::PingPong(animator.currentTime, duration);
        }
        else //NONE
        {
            animator.currentTime = std::clamp(animator.currentTime, 0.0f, duration);
            sampleTime = animator.currentTime;

            if (animator.currentTime >= duration || (animator.playbackSpeed < 0.0f && animator.currentTime <= 0.0f))
            {
                animator.isPlaying = false;
            }
        }

        const auto& clip = animator.currentClip;

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