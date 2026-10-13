#pragma once
#include "Scene/Scene.h"
#include "Scene/SceneSerializer.h"
#include "Commands/CommandHistory.h"
#include "Commands/ComponentCommands.h"
#include <glm/glm.hpp>
#include <string>

namespace rv {

struct TransformClipboardData
{
	glm::vec3 position{ 0.0f };
	glm::vec3 rotation{ 0.0f };
	glm::vec3 scale{ 1.0f };
	bool hasTransform = false;

	glm::vec3 vec3Value{ 0.0f };
	bool hasVec3 = false;
};

class InspectorContext
{
public:
	static void SetScene(Scene* scene);
	static Scene* GetScene();

	static void SetCurrentEntity(entt::entity entity);
	static entt::entity GetCurrentEntity();

	static TransformClipboardData& GetTransformClipboard();

	template<typename Func>
	static void ApplyEntityChange(entt::entity entity, const std::string& actionName, Func&& func)
	{
		Scene* scene = GetScene();
		if (!scene || entity == entt::null || !scene->GetRegistry().valid(entity))
		{
			func();
			return;
		}

		json before = SceneSerializer::SerializeEntity(*scene, entity, false);
		func();
		json after = SceneSerializer::SerializeEntity(*scene, entity, false);

		if (before != after)
		{
			CommandHistory::Get().Execute(std::make_unique<EntitySnapshotCommand>(
				scene, entity, before, after, actionName));
		}
	}
};

} // namespace rv
