#include "rvelapch.h"
#include "Entity.h"

using namespace rv;

Entity::Entity(entt::entity handle, Scene* scene)
	: m_EntityHandle(handle), m_Scene(scene)
{
}
