#include "rvelapch.h"
#include "Entity.h"

using namespace rv;

Entity::Entity(entt::entity handle, Scene* scene)
	: m_EntityHandle(handle), m_Scene(scene)
{
}

Entity EntityHandle::Resolve(Scene& scene) const
{
	if (!IsValid())
		return Entity{};

	auto& uuidMap = scene.GetUUIDEntityMap();
	auto it = uuidMap.find(uuid);
	if (it == uuidMap.end() || !scene.GetRegistry().valid(it->second))
		return Entity{};

	return Entity(it->second, &scene);
}
