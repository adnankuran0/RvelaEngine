#pragma once

#include "Scene/Scene.h"
#include "entt/entt.h"

namespace rv {

class Entity
{
public:
	Entity() = default;
	Entity(entt::entity handle, Scene* scene);
	Entity(const Entity& other) = default;

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		T& component = m_Scene->GetRegistry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		return component;
	}

	template<typename T, typename... Args>
	T& AddOrReplaceComponent(Args&&... args)
	{
		T& component = m_Scene->GetRegistry().emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
		return component;
	}

	template<typename T>
	T& GetComponent()
	{
		return m_Scene->GetRegistry().get<T>(m_EntityHandle);
	}

	template<typename T>
	bool HasComponent()
	{
		return m_Scene->GetRegistry().try_get<T>(m_EntityHandle) != nullptr;
	}

	template<typename T>
	void RemoveComponent()
	{
		m_Scene->GetRegistry().remove<T>(m_EntityHandle);
	}

	EntityUUID GetUUID()
	{
		return GetComponent<UUIDComponent>().uuid;
	}

	operator bool() const { return m_EntityHandle != entt::null; }
	operator entt::entity() const { return m_EntityHandle; }
	operator uint32_t() const { return (uint32_t)m_EntityHandle; }

	const std::string& GetName() { return GetComponent<TagComponent>().tag; }
	void SetName(const std::string& name) { GetComponent<TagComponent>().tag = name; }
	const entt::entity GetHandle() { return m_EntityHandle; }


	bool operator==(const Entity& other) const
	{
		return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
	}

	bool operator!=(const Entity& other) const
	{
		return !(*this == other);
	}
private:
	entt::entity m_EntityHandle{ entt::null };
	Scene* m_Scene = nullptr;
};

}