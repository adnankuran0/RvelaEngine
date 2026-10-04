#include "EditorSelection.h"

using namespace rv;

EditorSelection& EditorSelection::Get()
{
    static EditorSelection s_Instance;
    return s_Instance;
}

void EditorSelection::Select(entt::entity entity)
{
    m_SelectedEntities.clear();
    if (entity != entt::null)
    {
        m_SelectedEntities.push_back(entity);
        m_PrimaryEntity = entity;
    }
    else
    {
        m_PrimaryEntity = entt::null;
    }
}

void EditorSelection::Toggle(entt::entity entity)
{
    if (entity == entt::null) return;

    auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
    if (it != m_SelectedEntities.end())
    {
        m_SelectedEntities.erase(it);
        if (m_PrimaryEntity == entity)
        {
            m_PrimaryEntity = m_SelectedEntities.empty() ? entt::null : m_SelectedEntities.back();
        }
    }
    else
    {
        m_SelectedEntities.push_back(entity);
        m_PrimaryEntity = entity;
    }
}

void EditorSelection::Add(entt::entity entity)
{
    if (entity == entt::null) return;

    auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
    if (it == m_SelectedEntities.end())
    {
        m_SelectedEntities.push_back(entity);
    }
    m_PrimaryEntity = entity;
}

void EditorSelection::Remove(entt::entity entity)
{
    if (entity == entt::null) return;

    auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
    if (it != m_SelectedEntities.end())
    {
        m_SelectedEntities.erase(it);
        if (m_PrimaryEntity == entity)
        {
            m_PrimaryEntity = m_SelectedEntities.empty() ? entt::null : m_SelectedEntities.back();
        }
    }
}

void EditorSelection::SetSelection(const std::vector<entt::entity>& entities, entt::entity primary)
{
    m_SelectedEntities.clear();
    for (auto e : entities)
    {
        if (e != entt::null && std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), e) == m_SelectedEntities.end())
        {
            m_SelectedEntities.push_back(e);
        }
    }

    if (primary != entt::null && std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), primary) != m_SelectedEntities.end())
    {
        m_PrimaryEntity = primary;
    }
    else
    {
        m_PrimaryEntity = m_SelectedEntities.empty() ? entt::null : m_SelectedEntities.back();
    }
}

void EditorSelection::SelectRange(const std::vector<entt::entity>& range, entt::entity primary)
{
    SetSelection(range, primary);
}

void EditorSelection::Clear()
{
    m_SelectedEntities.clear();
    m_PrimaryEntity = entt::null;
}

void EditorSelection::SetPrimary(entt::entity entity)
{
    if (entity != entt::null && std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity) != m_SelectedEntities.end())
    {
        m_PrimaryEntity = entity;
    }
}

bool EditorSelection::IsSelected(entt::entity entity) const
{
    if (entity == entt::null) return false;
    return std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity) != m_SelectedEntities.end();
}

bool EditorSelection::IsPrimary(entt::entity entity) const
{
    return entity != entt::null && m_PrimaryEntity == entity;
}

void EditorSelection::Validate(const entt::registry& registry)
{
    m_SelectedEntities.erase(
        std::remove_if(m_SelectedEntities.begin(), m_SelectedEntities.end(),
            [&registry](entt::entity e) {
                return e == entt::null || !registry.valid(e);
            }),
        m_SelectedEntities.end()
    );

    if (m_PrimaryEntity != entt::null && !registry.valid(m_PrimaryEntity))
    {
        m_PrimaryEntity = m_SelectedEntities.empty() ? entt::null : m_SelectedEntities.back();
    }
}
