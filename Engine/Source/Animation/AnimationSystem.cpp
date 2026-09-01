#include "rvelapch.h"
#include "Animation/AnimationSystem.h"
#include "Scene/Components/AnimatorComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"

namespace rv {

    void AnimationSystem::Update()
    {
        auto view = m_Scene.GetRegistry().view<AnimatorComponent, TransformComponent>();
        for (auto entity : view)
        {
            AnimatorComponent& animator = m_Scene.GetComponent<AnimatorComponent>(entity);
            TransformComponent& transform = m_Scene.GetComponent<TransformComponent>(entity);

            if (!animator.isPlaying || !animator.currentClip || animator.currentClip->duration <= 0.0f)
                continue;

            animator.currentTime += Time::GetDeltaTime() * animator.playbackSpeed;

            if (animator.isLooping)
            {
                animator.currentTime = std::fmod(animator.currentTime, animator.currentClip->duration);
                if (animator.currentTime < 0.0f)
                    animator.currentTime += animator.currentClip->duration;
            }
            else
            {
                animator.currentTime = std::clamp(animator.currentTime, 0.0f, animator.currentClip->duration);
                if (animator.currentTime >= animator.currentClip->duration)
                {
                    animator.isPlaying = false;
                }
            }

            const auto& clip = animator.currentClip;

            if (!clip->positionTrack.keyframes.empty())
            {
                transform.SetPosition(clip->positionTrack.Sample(animator.currentTime));
            }

            if (!clip->rotationTrack.keyframes.empty())
            {
                glm::quat sampledRot = clip->rotationTrack.Sample(animator.currentTime);
                transform.SetRotation(sampledRot);
            }

            if (!clip->scaleTrack.keyframes.empty())
            {
                transform.SetScale(clip->scaleTrack.Sample(animator.currentTime));
            }
        }
    }

}