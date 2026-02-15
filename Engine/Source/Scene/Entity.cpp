#include "rvelapch.h"
#include "Entity.h"

namespace rv {

Entity::Entity(entt::entity handle, Scene* scene)
	: m_EntityHandle(handle), m_Scene(scene)
{
}

}