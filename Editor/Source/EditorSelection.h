#pragma once

#include <vector>
#include <algorithm>
#include <entt/entt.h>

namespace rv {

class EditorSelection
{
public:
    static EditorSelection& Get();

    void Select(entt::entity entity);
    void Toggle(entt::entity entity);
    void Add(entt::entity entity);
    void Remove(entt::entity entity);
    void SetSelection(const std::vector<entt::entity>& entities, entt::entity primary = entt::null);
    void SelectRange(const std::vector<entt::entity>& range, entt::entity primary = entt::null);
    void Clear();

    void SetPrimary(entt::entity entity);
    entt::entity GetPrimary() const { return m_PrimaryEntity; }

    const std::vector<entt::entity>& GetSelectedEntities() const { return m_SelectedEntities; }
    size_t GetCount() const { return m_SelectedEntities.size(); }
    bool IsEmpty() const { return m_SelectedEntities.empty(); }

    bool IsSelected(entt::entity entity) const;
    bool IsPrimary(entt::entity entity) const;

    void Validate(const entt::registry& registry);

private:
    EditorSelection() = default;

    std::vector<entt::entity> m_SelectedEntities;
    entt::entity m_PrimaryEntity = entt::null;
};

}
