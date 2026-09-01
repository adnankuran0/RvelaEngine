#include "rvelapch.h"
#include "AnimatorComponent.h"

using namespace rv;

// TODO: delete this constructor, it is for just testing
AnimatorComponent::AnimatorComponent() 
{
	auto bounceClip = std::make_shared<Animation::AnimationClip>("BounceAndRotate");
	bounceClip->positionTrack.AddKeyframe(0.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	bounceClip->positionTrack.AddKeyframe(1.0f, glm::vec3(0.0f, 3.0f, 0.0f));
	bounceClip->positionTrack.AddKeyframe(2.0f, glm::vec3(0.0f, 0.0f, 0.0f));

	glm::quat rotStart = glm::angleAxis(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat rotEnd = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	bounceClip->rotationTrack.AddKeyframe(0.0f, rotStart);
	bounceClip->rotationTrack.AddKeyframe(2.0f, rotEnd);
	bounceClip->RecalculateDuration();

	SetClip(bounceClip);
	Play();
}

json AnimatorComponent::Serialize() const
{
	json j;
	return j;
   //TODO
}

void AnimatorComponent::Deserialize(const json& j)
{
	//TODO
}
