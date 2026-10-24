#include "SceneHierarchyPanel.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "AssetImporters/PrefabImporter.h"
#include "Core/Engine.h"
#include "Scene/Entity.h"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include "AssetBrowserPanel.h"
#include "Asset/AssetManager.h"
#include "Utils/ProjectManager.h"
#include "AssetImporters/ModelImporter.h"
#include "Utils/FileUtils.h"
#include "EditorSelection.h"
#include "EditorUtils.h"
#include "Commands/CommandHistory.h"
#include "Commands/EntityLifecycleCommands.h"
#include "Commands/ReparentCommand.h"
#include "Commands/PrefabCommands.h"
#include "PrefabEditManager.h"
#include "GUI/Style.h"
#include "Render/IconLibrary.h"
#include "Renderer/Texture.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/DirectionalLightComponent.h"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/SpotLightComponent.h"
#include "Scene/Components/AudioEmitterComponent.h"
#include "Scene/Components/ParticleEmitterComponent.h"
#include "Scene/Components/ScriptComponent.h"
#include "Scene/Components/AnimatorComponent.h"
#include "Scene/Components/SkeletonComponent.h"
#include "Scene/Components/SkeletalMeshComponent.h"
#include "Scene/Components/SkeletalMeshRendererComponent.h"
#include "Scene/Components/UIComponents.h"

using namespace rv;

struct EntityVisualInfo
{
    ImTextureID icon = 0;
    ImVec4 iconTint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
};

static EntityVisualInfo GetEntityVisualInfo(Scene& scene, entt::entity e, bool isPrefab)
{
    EntityVisualInfo info;
    auto& iconLib = IconLibrary::Get();

    if (isPrefab)
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::Prefab).GetID();
        info.iconTint = GetAccentColor();
        return info;
    }

    if (scene.HasComponent<CameraComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::Camera).GetID();
        info.iconTint = ImVec4(0.38f, 0.76f, 0.98f, 1.0f);
        return info;
    }

    if (scene.HasComponent<DirectionalLightComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::DirectionalLight).GetID();
        info.iconTint = ImVec4(0.98f, 0.82f, 0.28f, 1.0f);
        return info;
    }

    if (scene.HasComponent<PointLightComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::PointLight).GetID();
        info.iconTint = ImVec4(0.98f, 0.75f, 0.20f, 1.0f);
        return info;
    }

    if (scene.HasComponent<SpotLightComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::SpotLight).GetID();
        info.iconTint = ImVec4(0.98f, 0.70f, 0.15f, 1.0f);
        return info;
    }

    if (scene.HasComponent<AudioEmitterComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::AudioEmitter).GetID();
        info.iconTint = ImVec4(0.78f, 0.60f, 0.98f, 1.0f);
        return info;
    }

    if (scene.HasComponent<ParticleEmitterComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::ParticleEmitter).GetID();
        info.iconTint = ImVec4(0.98f, 0.55f, 0.35f, 1.0f);
        return info;
    }

    if (scene.HasComponent<MeshComponent>(e) || scene.HasComponent<MeshRendererComponent>(e) || scene.HasComponent<SkeletalMeshComponent>(e) || scene.HasComponent<SkeletalMeshRendererComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::Mesh).GetID();
        info.iconTint = ImVec4(0.55f, 0.75f, 0.90f, 1.0f);
        return info;
    }

    if (scene.HasComponent<AnimatorComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::AnimLib).GetID();
        info.iconTint = ImVec4(0.95f, 0.60f, 0.75f, 1.0f);
        return info;
    }

    if (scene.HasComponent<SkeletonComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::Skeleton).GetID();
        info.iconTint = ImVec4(0.85f, 0.70f, 0.95f, 1.0f);
        return info;
    }

    if (scene.HasComponent<UICanvasComponent>(e) || scene.HasComponent<RectTransformComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::Texture).GetID();
        info.iconTint = ImVec4(0.40f, 0.78f, 0.90f, 1.0f);
        return info;
    }

    if (scene.HasComponent<ScriptComponent>(e))
    {
        info.icon = (ImTextureID)(intptr_t)iconLib.GetIcon(EditorIcon::Script).GetID();
        info.iconTint = ImVec4(0.40f, 0.85f, 0.60f, 1.0f);
        return info;
    }

    info.icon = 0;
    return info;
}

static inline Entity LoadPrimitive(Scene& scene, const std::string& primitiveMeshName)
{
    return EditorUtils::LoadPrimitive(scene, primitiveMeshName);
}

void SceneHierarchyPanel::Draw(Engine* engine)
{
    Scene& scene = engine->GetActiveScene();
    entt::entity rootEntity = scene.GetRootEntity();
    entt::registry& registry = scene.GetRegistry();

    std::vector<entt::entity> visibleOrder;

    auto isDescendantOfAny = [&](entt::entity e, const std::vector<entt::entity>& entityList) -> bool {
        entt::entity cur = e;
        while (cur != entt::null && registry.valid(cur) && scene.HasComponent<SceneTreeComponent>(cur)) {
            entt::entity parent = scene.GetComponent<SceneTreeComponent>(cur).parent;
            if (parent == entt::null || parent == rootEntity) break;
            if (std::find(entityList.begin(), entityList.end(), parent) != entityList.end()) {
                return true;
            }
            cur = parent;
        }
        return false;
    };

    auto selectSingle = [&](entt::entity e) {
        EditorSelection::Get().Select(e);
    };

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

    static char searchFilter[128] = "";

    float quickAddBtnWidth = 26.0f;
    float searchWidth = ImGui::GetContentRegionAvail().x - quickAddBtnWidth - 6.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));

    ImGui::SetNextItemWidth(searchWidth);
    ImGui::InputTextWithHint("##HierarchySearch", "Search Entities...", searchFilter, sizeof(searchFilter));

    if (searchFilter[0] != '\0')
    {
        ImGui::SameLine(searchWidth - 20.0f);
        if (ImGui::SmallButton("x##ClearSearch"))
            searchFilter[0] = '\0';
    }

    ImGui::SameLine(0, 6.0f);
    if (ImGui::Button("+##QuickAddEntity", ImVec2(quickAddBtnWidth, 0.0f)))
    {
        ImGui::OpenPopup("HierarchyQuickAddPopup");
    }

    ImGui::PopStyleVar(2); 

    auto registerCreatedEntity = [&](entt::entity handle) {
        if (handle != entt::null && registry.valid(handle))
        {
            if (PrefabEditManager::Get().IsInPrefabMode())
            {
                entt::entity pRoot = PrefabEditManager::Get().GetPrefabRootEntity();
                if (pRoot != entt::null && registry.valid(pRoot) && handle != pRoot && scene.GetParent(handle) == rootEntity)
                {
                    scene.SetParent(handle, pRoot);
                }
            }
            CommandHistory::Get().Execute(std::make_unique<CreateEntityCommand>(&scene, handle));
            selectSingle(handle);
        }
    };

    if (ImGui::BeginPopup("HierarchyQuickAddPopup"))
    {
        if (ImGui::MenuItem("Create Empty Entity"))
        {
            registerCreatedEntity(scene.CreateEntity("New Entity"));
        }
        if (ImGui::MenuItem("Camera"))
        {
            entt::entity cam = scene.CreateEntity("Camera");
            scene.AddComponent<CameraComponent>(cam);
            registerCreatedEntity(cam);
        }
        if (ImGui::BeginMenu("3D Objects"))
        {
            if (ImGui::MenuItem("Cube")) registerCreatedEntity(LoadPrimitive(scene, "Cube").GetHandle());
            if (ImGui::MenuItem("Sphere")) registerCreatedEntity(LoadPrimitive(scene, "Sphere").GetHandle());
            if (ImGui::MenuItem("Cylinder")) registerCreatedEntity(LoadPrimitive(scene, "Cylinder").GetHandle());
            if (ImGui::MenuItem("Quad")) registerCreatedEntity(LoadPrimitive(scene, "Quad").GetHandle());
            if (ImGui::MenuItem("Capsule")) registerCreatedEntity(LoadPrimitive(scene, "Capsule").GetHandle());
            if (ImGui::MenuItem("Plane")) registerCreatedEntity(LoadPrimitive(scene, "Plane").GetHandle());
            if (ImGui::MenuItem("Torus")) registerCreatedEntity(LoadPrimitive(scene, "Torus").GetHandle());
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Lights"))
        {
            if (ImGui::MenuItem("Directional Light"))
            {
                entt::entity l = scene.CreateEntity("Directional Light");
                scene.AddComponent<DirectionalLightComponent>(l);
                registerCreatedEntity(l);
            }
            if (ImGui::MenuItem("Point Light"))
            {
                entt::entity l = scene.CreateEntity("Point Light");
                scene.AddComponent<PointLightComponent>(l);
                registerCreatedEntity(l);
            }
            if (ImGui::MenuItem("Spot Light"))
            {
                entt::entity l = scene.CreateEntity("Spot Light");
                scene.AddComponent<SpotLightComponent>(l);
                registerCreatedEntity(l);
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Particle Emitter"))
        {
            Entity emitterEntity = LoadPrimitive(scene, "Quad");
            if (emitterEntity)
            {
                if (emitterEntity.HasComponent<RigidbodyComponent>())
                    emitterEntity.RemoveComponent<RigidbodyComponent>();
                if (emitterEntity.HasComponent<ConvexHullColliderComponent>())
                    emitterEntity.RemoveComponent<ConvexHullColliderComponent>();
                emitterEntity.GetComponent<TagComponent>().tag = "Particle Emitter";
                emitterEntity.AddComponent<ParticleEmitterComponent>();
                registerCreatedEntity(emitterEntity.GetHandle());
            }
        }
        ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto getChildren = [&scene, &registry](entt::entity parent) -> const std::vector<entt::entity>&{
        static const std::vector<entt::entity> empty;
        if (parent != entt::null && registry.valid(parent) && scene.HasComponent<SceneTreeComponent>(parent)) {
            return scene.GetComponent<SceneTreeComponent>(parent).children;
        }
        return empty;
        };

    auto matchesFilter = [&](entt::entity entity, auto& self) -> bool {
        if (searchFilter[0] == '\0') return true;
        if (!scene.HasComponent<TagComponent>(entity)) return false;
        auto& tagComp = scene.GetComponent<TagComponent>(entity);
        std::string nameLower = tagComp.tag;
        std::string filterLower = searchFilter;
        auto toLower = [](std::string& s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            };
        toLower(nameLower);
        toLower(filterLower);
        if (nameLower.find(filterLower) != std::string::npos) return true;

        for (auto child : getChildren(entity)) {
            if (self(child, self)) return true;
        }
        return false;
        };

    auto isDescendantOf = [&](entt::entity childCandidate, entt::entity parentCandidate, auto& self) -> bool {
        if (childCandidate == parentCandidate) return true;
        for (auto subChild : getChildren(childCandidate)) {
            if (self(subChild, parentCandidate, self)) return true;
        }
        return false;
        };

    auto getPrefabRoot = [&](entt::entity e) -> entt::entity {
        entt::entity curr = e;
        while (curr != entt::null && curr != rootEntity)
        {
            if (scene.HasComponent<PrefabComponent>(curr))
                return curr;
            curr = scene.GetParent(curr);
        }
        return entt::null;
    };

    ImVec4 unselectedHover = ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered);
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_Header));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, unselectedHover);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(GetAccentColor().x, GetAccentColor().y, GetAccentColor().z, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 3.0f));

    if (m_RenamingEntity != entt::null && !registry.valid(m_RenamingEntity))
        m_RenamingEntity = entt::null;
    if (m_SelectionAnchor != entt::null && !registry.valid(m_SelectionAnchor))
        m_SelectionAnchor = entt::null;
    if (m_PendingSingleSelect != entt::null && !registry.valid(m_PendingSingleSelect))
        m_PendingSingleSelect = entt::null;
    if (m_PendingShiftSelect != entt::null && !registry.valid(m_PendingShiftSelect))
        m_PendingShiftSelect = entt::null;

    ImGuiIO& hierarchyIO = ImGui::GetIO();
    if (ImGui::IsWindowFocused() && !hierarchyIO.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F2))
    {
        entt::entity selected = EditorSelection::Get().GetPrimary();
        if (selected != entt::null && selected != rootEntity && registry.valid(selected) && scene.HasComponent<TagComponent>(selected))
        {
            m_RenamingEntity = selected;
            strncpy_s(m_RenameBuffer, sizeof(m_RenameBuffer), scene.GetComponent<TagComponent>(selected).tag.c_str(), _TRUNCATE);
            m_FocusRenameInput = true;
        }
    }

    int rowIndex = 0;

    std::function<void(entt::entity)> DrawEntityNode = [&](entt::entity entity)
        {
            if (!registry.valid(entity)) return;

            if (registry.any_of<TagComponent>(entity) && registry.get<TagComponent>(entity).tag == "__PrefabEditorLight__")
                return;

            if (searchFilter[0] != '\0' && !matchesFilter(entity, matchesFilter))
                return;

            bool isRoot = (entity == rootEntity);
            const auto& children = getChildren(entity);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

            if (isRoot)
            {
                flags |= ImGuiTreeNodeFlags_DefaultOpen;
                flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }
            else if (children.empty())
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            if (!isRoot && searchFilter[0] != '\0' && !children.empty())
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);

            if (!isRoot)
                visibleOrder.push_back(entity);

            bool isSelected = (!isRoot && EditorSelection::Get().IsSelected(entity));
            if (isSelected)
                flags |= ImGuiTreeNodeFlags_Selected;

            if (!scene.HasComponent<TagComponent>(entity))
                scene.AddComponent<TagComponent>(entity, "Entity");

            auto& tagComponent = scene.GetComponent<TagComponent>(entity);
            std::string nodeId = tagComponent.tag + "##" + std::to_string((uint32_t)entity);

            entt::entity prefabRoot = getPrefabRoot(entity);
            bool isPrefab = (prefabRoot != entt::null);
            bool isActiveInHierarchy = scene.IsEntityActive(entity);
            const bool isRenaming = (m_RenamingEntity == entity);

            int colorPushes = 0;
            bool isPrimary = (isSelected && EditorSelection::Get().IsPrimary(entity) && EditorSelection::Get().GetCount() > 1);

            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, GetAccentColor());
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, GetAccentHoverColor());
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, GetAccentActiveColor());
                colorPushes += 3;

                if (isPrimary)
                {
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.28f, 0.48f, 0.82f, 0.95f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.88f, 0.45f, 1.0f));
                    colorPushes += 2;
                }
                else if (!isRenaming)
                {
                    ImVec4 selTextColor = GetSelectionTextColor();
                    ImGui::PushStyleColor(ImGuiCol_Text, selTextColor);
                    colorPushes++;
                }
            }
            else
            {
                if (!isActiveInHierarchy)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, GetMutedTextColor());
                    colorPushes++;
                }
                else if (isPrefab)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, GetAccentColor());
                    colorPushes++;
                }
            }

            std::string nodeTitle = tagComponent.tag;

            EntityVisualInfo vInfo = GetEntityVisualInfo(scene, entity, isPrefab);

            std::string labelText = "    " + nodeTitle;

            bool nodeOpen = isRenaming
                ? ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "##EntityRenameNode")
                : ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", labelText.c_str());

            ImVec2 itemMin = ImGui::GetItemRectMin();
            ImVec2 itemMax = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            if (!isSelected && (rowIndex % 2 == 1))
            {
                drawList->AddRectFilled(itemMin, itemMax, IM_COL32(255, 255, 255, 4));
            }
            rowIndex++;

            if (isSelected)
            {
                drawList->AddRectFilled(
                    ImVec2(itemMin.x, itemMin.y + 1.0f),
                    ImVec2(itemMin.x + 3.0f, itemMax.y - 1.0f),
                    IsAccentBright() ? IM_COL32(20, 20, 32, 255) : ImGui::GetColorU32(GetAccentColor()),
                    1.5f
                );
            }

            if (vInfo.icon != 0)
            {
                float arrowSpace = ImGui::GetTreeNodeToLabelSpacing();
                float iconX = itemMin.x + arrowSpace + 1.0f;
                float iconY = itemMin.y + (itemMax.y - itemMin.y - 14.0f) * 0.5f;

                ImU32 iconColor;
                if (isSelected && IsAccentBright())
                {
                    iconColor = IM_COL32(20, 20, 32, 240);
                }
                else if (!isActiveInHierarchy)
                {
                    iconColor = IM_COL32(130, 130, 140, 130);
                }
                else
                {
                    iconColor = ImGui::ColorConvertFloat4ToU32(vInfo.iconTint);
                }

                drawList->AddImage(
                    vInfo.icon,
                    ImVec2(iconX, iconY),
                    ImVec2(iconX + 14.0f, iconY + 14.0f),
                    ImVec2(0, 0), ImVec2(1, 1),
                    iconColor
                );
            }
            else
            {
                float arrowSpace = ImGui::GetTreeNodeToLabelSpacing();
                float boxX = itemMin.x + arrowSpace + 2.0f;
                float boxY = itemMin.y + (itemMax.y - itemMin.y - 10.0f) * 0.5f;
                ImU32 boxCol;
                if (isSelected && IsAccentBright())
                    boxCol = IM_COL32(20, 20, 32, 240);
                else if (!isActiveInHierarchy)
                    boxCol = IM_COL32(100, 100, 110, 110);
                else
                    boxCol = IM_COL32(145, 150, 165, 170);
                drawList->AddRect(ImVec2(boxX, boxY), ImVec2(boxX + 10.0f, boxY + 10.0f), boxCol, 2.0f, 0, 1.2f);
            }

            bool toggleHovered = false;
            if (!isRoot && !isRenaming)
            {
                float toggleSize = 14.0f;
                float toggleX = itemMax.x - 22.0f;
                float toggleY = itemMin.y + (itemMax.y - itemMin.y - toggleSize) * 0.5f;

                bool isSelfActive = scene.IsEntitySelfActive(entity);

                toggleHovered = ImGui::IsMouseHoveringRect(
                    ImVec2(toggleX - 3.0f, toggleY - 3.0f),
                    ImVec2(toggleX + toggleSize + 3.0f, toggleY + toggleSize + 3.0f)
                );

                if (toggleHovered)
                {
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        scene.SetEntityActive(entity, !isSelfActive);
                    }
                }

                ImVec2 dotCenter(toggleX + toggleSize * 0.5f, toggleY + toggleSize * 0.5f);
                if (isSelfActive && isActiveInHierarchy)
                {
                    drawList->AddCircleFilled(dotCenter, 3.5f, IM_COL32(16, 185, 129, 230));
                }
                else if (isSelfActive && !isActiveInHierarchy)
                {
                    drawList->AddCircle(dotCenter, 3.5f, IM_COL32(245, 158, 11, 220), 0, 1.5f);
                }
                else
                {
                    drawList->AddCircle(dotCenter, 3.5f, IM_COL32(140, 140, 150, 140), 0, 1.2f);
                }

            }

            if (isRenaming)
            {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (m_FocusRenameInput)
                {
                    ImGui::SetKeyboardFocusHere();
                    m_FocusRenameInput = false;
                }

                const bool commitRename = ImGui::InputText("##EntityRenameInput", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                if (commitRename)
                {
                    if (m_RenameBuffer[0] != '\0' && tagComponent.tag != m_RenameBuffer)
                    {
                        CommandHistory::Get().Execute(std::make_unique<RenameEntityCommand>(&scene, entity, tagComponent.tag, m_RenameBuffer));
                        tagComponent.tag = m_RenameBuffer;
                    }
                    m_RenamingEntity = entt::null;
                }
                else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    m_RenamingEntity = entt::null;
                }
            }

            if (colorPushes > 0)
                ImGui::PopStyleColor(colorPushes);

            if (!isRoot && !isRenaming && !toggleHovered)
            {
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
                {
                    ImGuiIO& io = ImGui::GetIO();
                    if (io.KeyCtrl)
                    {
                        EditorSelection::Get().Toggle(entity);
                        m_SelectionAnchor = entity;
                        m_PendingSingleSelect = entt::null;
                    }
                    else if (io.KeyShift)
                    {
                        m_PendingShiftSelect = entity;
                        m_PendingSingleSelect = entt::null;
                    }
                    else
                    {
                        if (!EditorSelection::Get().IsSelected(entity))
                        {
                            m_PendingSingleSelect = entity;
                        }
                        else if (EditorSelection::Get().GetCount() > 1)
                        {
                            m_PendingSingleSelect = entity;
                        }
                        else
                        {
                            m_SelectionAnchor = entity;
                            m_PendingSingleSelect = entt::null;
                        }
                    }
                }
                else if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    if (!EditorSelection::Get().IsSelected(entity))
                    {
                        EditorSelection::Get().Select(entity);
                        m_SelectionAnchor = entity;
                        m_PendingSingleSelect = entt::null;
                    }
                }
            }

            if (!isRoot && ImGui::BeginPopupContextItem())
            {
                if (!EditorSelection::Get().IsSelected(entity))
                {
                    EditorSelection::Get().Select(entity);
                    m_SelectionAnchor = entity;
                    m_PendingSingleSelect = entt::null;
                }

                if (ImGui::BeginMenu("Create Child Entity"))
                {
                    auto registerChild = [&](entt::entity child) {
                        if (child != entt::null && registry.valid(child))
                        {
                            scene.SetParent(child, entity);
                            CommandHistory::Get().Execute(std::make_unique<CreateEntityCommand>(&scene, child));
                            selectSingle(child);
                        }
                    };

                    if (ImGui::MenuItem("Empty Entity"))
                    {
                        registerChild(scene.CreateEntity("New Entity"));
                    }

                    if (ImGui::MenuItem("Camera"))
                    {
                        entt::entity cam = scene.CreateEntity("Camera");
                        scene.AddComponent<CameraComponent>(cam);
                        registerChild(cam);
                    }

                    if (ImGui::MenuItem("Particle Emitter"))
                    {
                        Entity emitterEntity = LoadPrimitive(scene, "Quad");
                        if (emitterEntity)
                        {
                            if (emitterEntity.HasComponent<RigidbodyComponent>())
                                emitterEntity.RemoveComponent<RigidbodyComponent>();
                            if (emitterEntity.HasComponent<ConvexHullColliderComponent>())
                                emitterEntity.RemoveComponent<ConvexHullColliderComponent>();

                            emitterEntity.GetComponent<TagComponent>().tag = "Particle Emitter";
                            emitterEntity.AddComponent<ParticleEmitterComponent>();
                            registerChild(emitterEntity.GetHandle());
                        }
                    }

                    if (ImGui::MenuItem("Audio Emitter"))
                    {
                        entt::entity audio = scene.CreateEntity("AudioEmitter");
                        scene.AddComponent<AudioEmitterComponent>(audio);
                        registerChild(audio);
                    }

                    if (ImGui::MenuItem("Decal"))
                    {
                        entt::entity decalEnt = scene.CreateEntity("Decal");
                        scene.GetComponent<TransformComponent>(decalEnt).SetEulerRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
                        scene.AddComponent<DecalComponent>(decalEnt);
                        registerChild(decalEnt);
                    }

                    if (ImGui::BeginMenu("3D Objects"))
                    {
                        if (ImGui::MenuItem("Cube")) registerChild(LoadPrimitive(scene, "Cube").GetHandle());
                        if (ImGui::MenuItem("Sphere")) registerChild(LoadPrimitive(scene, "Sphere").GetHandle());
                        if (ImGui::MenuItem("Cylinder")) registerChild(LoadPrimitive(scene, "Cylinder").GetHandle());
                        if (ImGui::MenuItem("Quad")) registerChild(LoadPrimitive(scene, "Quad").GetHandle());
                        if (ImGui::MenuItem("Cone")) registerChild(LoadPrimitive(scene, "Cone").GetHandle());
                        if (ImGui::MenuItem("Capsule")) registerChild(LoadPrimitive(scene, "Capsule").GetHandle());
                        if (ImGui::MenuItem("Torus")) registerChild(LoadPrimitive(scene, "Torus").GetHandle());
                        if (ImGui::MenuItem("Plane")) registerChild(LoadPrimitive(scene, "Plane").GetHandle());
                        if (ImGui::MenuItem("Monkey")) registerChild(LoadPrimitive(scene, "Monkey").GetHandle());
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Lights"))
                    {
                        if (ImGui::MenuItem("Directional Light"))
                        {
                            entt::entity light = scene.CreateEntity("DirectionalLight");
                            scene.GetComponent<TransformComponent>(light).SetEulerRotation(glm::vec3(-60.0f, -90.0f, 0.0f));
                            scene.AddComponent<DirectionalLightComponent>(light);
                            registerChild(light);
                        }
                        if (ImGui::MenuItem("Point Light"))
                        {
                            entt::entity light = scene.CreateEntity("PointLight");
                            scene.AddComponent<PointLightComponent>(light);
                            registerChild(light);
                        }
                        if (ImGui::MenuItem("Spot Light"))
                        {
                            entt::entity light = scene.CreateEntity("SpotLight");
                            scene.GetComponent<TransformComponent>(light).SetEulerRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
                            scene.AddComponent<SpotLightComponent>(light);
                            registerChild(light);
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("UI"))
                    {
                        if (ImGui::MenuItem("UI Canvas"))
                        {
                            entt::entity canvas = scene.CreateEntity("Canvas");
                            scene.AddComponent<UICanvasComponent>(canvas);
                            scene.AddComponent<RectTransformComponent>(canvas);
                            registerChild(canvas);
                        }
                        if (ImGui::MenuItem("UI Text"))
                        {
                            entt::entity text = scene.CreateEntity("UI Text");
                            scene.AddComponent<RectTransformComponent>(text);
                            scene.AddComponent<UITextComponent>(text);
                            registerChild(text);
                        }
                        if (ImGui::MenuItem("UI Button"))
                        {
                            entt::entity button = scene.CreateEntity("UI Button");
                            scene.AddComponent<RectTransformComponent>(button);
                            scene.AddComponent<UIButtonComponent>(button);
                            registerChild(button);
                        }
                        if (ImGui::MenuItem("UI Image"))
                        {
                            entt::entity img = scene.CreateEntity("UI Image");
                            scene.AddComponent<RectTransformComponent>(img);
                            scene.AddComponent<UIImageComponent>(img);
                            registerChild(img);
                        }
                        if (ImGui::MenuItem("UI Slider"))
                        {
                            entt::entity slider = scene.CreateEntity("UI Slider");
                            scene.AddComponent<RectTransformComponent>(slider);
                            scene.AddComponent<UISliderComponent>(slider);
                            registerChild(slider);
                        }
                        if (ImGui::MenuItem("UI Progress Bar"))
                        {
                            entt::entity pbar = scene.CreateEntity("UI Progress Bar");
                            scene.AddComponent<RectTransformComponent>(pbar);
                            scene.AddComponent<UIProgressBarComponent>(pbar);
                            registerChild(pbar);
                        }
                        if (ImGui::MenuItem("UI Checkbox"))
                        {
                            entt::entity cb = scene.CreateEntity("UI Checkbox");
                            scene.AddComponent<RectTransformComponent>(cb);
                            scene.AddComponent<UICheckboxComponent>(cb);
                            registerChild(cb);
                        }
                        if (ImGui::MenuItem("UI Panel"))
                        {
                            entt::entity panel = scene.CreateEntity("UI Panel");
                            auto& rect = scene.AddComponent<RectTransformComponent>(panel);
                            rect.size = { 300.0f, 200.0f };
                            scene.AddComponent<UIPanelComponent>(panel);
                            registerChild(panel);
                        }
                        if (ImGui::MenuItem("UI Vertical Container"))
                        {
                            entt::entity vbox = scene.CreateEntity("UI Vertical Container");
                            auto& rect = scene.AddComponent<RectTransformComponent>(vbox);
                            rect.size = { 200.0f, 300.0f };
                            auto& lg = scene.AddComponent<UILayoutGroupComponent>(vbox);
                            lg.type = UILayoutType::Vertical;
                            registerChild(vbox);
                        }
                        if (ImGui::MenuItem("UI Horizontal Container"))
                        {
                            entt::entity hbox = scene.CreateEntity("UI Horizontal Container");
                            auto& rect = scene.AddComponent<RectTransformComponent>(hbox);
                            rect.size = { 400.0f, 60.0f };
                            auto& lg = scene.AddComponent<UILayoutGroupComponent>(hbox);
                            lg.type = UILayoutType::Horizontal;
                            registerChild(hbox);
                        }
                        if (ImGui::MenuItem("UI Text Input"))
                        {
                            entt::entity input = scene.CreateEntity("UI Text Input");
                            auto& rect = scene.AddComponent<RectTransformComponent>(input);
                            rect.size = { 200.0f, 36.0f };
                            scene.AddComponent<UITextInputComponent>(input);
                            registerChild(input);
                        }
                        if (ImGui::MenuItem("UI Scroll View"))
                        {
                            entt::entity sv = scene.CreateEntity("UI Scroll View");
                            auto& rect = scene.AddComponent<RectTransformComponent>(sv);
                            rect.size = { 250.0f, 200.0f };
                            scene.AddComponent<UIScrollRectComponent>(sv);
                            registerChild(sv);
                        }
                        if (ImGui::MenuItem("UI Tab Container"))
                        {
                            entt::entity tabs = scene.CreateEntity("UI Tab Container");
                            auto& rect = scene.AddComponent<RectTransformComponent>(tabs);
                            rect.size = { 350.0f, 250.0f };
                            scene.AddComponent<UITabContainerComponent>(tabs);
                            registerChild(tabs);
                        }
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Save as prefab"))
                {
                    std::string entityName = scene.HasComponent<TagComponent>(entity)
                        ? scene.GetComponent<TagComponent>(entity).tag
                        : "prefab";
                    for (char& c : entityName)
                        if (std::string("\\/:*?\"<>|").find(c) != std::string::npos) c = '_';
                    if (entityName.empty()) entityName = "prefab";

                    std::filesystem::path prefabsFolder = ProjectManager::GetAssetDirectory() / "Prefabs";
                    std::error_code ec;
                    if (!std::filesystem::exists(prefabsFolder, ec))
                        std::filesystem::create_directories(prefabsFolder, ec);

                    std::filesystem::path prefabPath = prefabsFolder / (entityName + ".rprefab");
                    for (int i = 1; std::filesystem::exists(prefabPath); ++i)
                        prefabPath = prefabsFolder / (entityName + std::to_string(i) + ".rprefab");

                    AssetRegistry& reg = AssetManager::Get().GetRegistry();
                    AssetMeta meta = reg.GetOrCreateMeta(prefabPath);

                    Ref<PrefabAsset> prefab = PrefabImporter::CreatePrefabAsset(prefabPath, meta.uuid, scene, entity);
                    if (prefab)
                    {
                        meta.importerID = "PrefabImporter";
                        reg.SaveMeta(prefabPath, meta);

                        auto assetDir = reg.GetAssetDir();
                        if (!assetDir.empty() && std::filesystem::exists(assetDir))
                            reg.Scan(assetDir);

                        if (!scene.HasComponent<PrefabComponent>(entity))
                            scene.AddComponent<PrefabComponent>(entity, meta.uuid);
                        else
                            scene.GetComponent<PrefabComponent>(entity).SetPrefabID(meta.uuid);

                        //LOG_INFO("Prefab saved: {}", prefabPath.string());
                        AssetBrowserPanel::RequestRename(prefabPath);
                    }
                }

                if (prefabRoot != entt::null && !PrefabEditManager::Get().IsInPrefabMode())
                {
                    if (ImGui::MenuItem("Open in Prefab Mode"))
                    {
                        AssetUUID pUUID = scene.GetComponent<PrefabComponent>(prefabRoot).GetPrefabID();
                        if (pUUID.IsValid())
                        {
                            PrefabEditManager::Get().OpenPrefab(*engine, pUUID);
                        }
                    }

                    if (prefabRoot == entity)
                    {
                        if (ImGui::MenuItem("Apply to Prefab"))
                        {
                            auto cmd = std::make_unique<ApplyPrefabCommand>(&scene, entity);
                            PrefabImporter::ApplyPrefab(scene, entity);
                            CommandHistory::Get().Execute(std::move(cmd));
                        }

                        if (ImGui::MenuItem("Revert to Prefab"))
                        {
                            auto cmd = std::make_unique<RevertPrefabCommand>(&scene, entity);
                            PrefabImporter::RevertPrefab(scene, entity);
                            cmd->CaptureAfter(EditorSelection::Get().GetPrimary());
                            CommandHistory::Get().Execute(std::move(cmd));
                        }

                        if (ImGui::MenuItem("Unpack Prefab (Make Local)"))
                        {
                            auto cmd = std::make_unique<UnpackPrefabCommand>(&scene, entity);
                            scene.RemoveComponent<PrefabComponent>(entity);
                            CommandHistory::Get().Execute(std::move(cmd));
                        }
                    }
                    else
                    {
                        if (ImGui::MenuItem("Apply to Prefab Root"))
                        {
                            auto cmd = std::make_unique<ApplyPrefabCommand>(&scene, prefabRoot);
                            PrefabImporter::ApplyPrefab(scene, prefabRoot);
                            CommandHistory::Get().Execute(std::move(cmd));
                        }

                        if (ImGui::MenuItem("Revert to Prefab Root"))
                        {
                            auto cmd = std::make_unique<RevertPrefabCommand>(&scene, prefabRoot);
                            PrefabImporter::RevertPrefab(scene, prefabRoot);
                            cmd->CaptureAfter(EditorSelection::Get().GetPrimary());
                            CommandHistory::Get().Execute(std::move(cmd));
                        }

                        if (ImGui::MenuItem("Select Prefab Root"))
                        {
                            selectSingle(prefabRoot);
                        }
                    }

                    ImGui::Separator();
                }

                if (ImGui::MenuItem("Detach from parent"))
                {
                    auto compoundCmd = std::make_unique<CompoundCommand>("Detach from Parent");
                    const auto& selected = EditorSelection::Get().GetSelectedEntities();
                    for (auto e : selected)
                    {
                        if (e != entt::null && registry.valid(e) && !isDescendantOfAny(e, selected))
                        {
                            if (scene.GetParent(e) != rootEntity)
                            {
                                compoundCmd->Add(std::make_unique<ReparentCommand>(&scene, e, rootEntity));
                            }
                        }
                    }
                    if (!compoundCmd->IsEmpty())
                    {
                        CommandHistory::Get().Execute(std::move(compoundCmd));
                    }
                }

                if (EditorSelection::Get().GetCount() > 1)
                {
                    if (ImGui::MenuItem("Duplicate Selected Entities", "Ctrl+D"))
                    {
                        const auto& selected = EditorSelection::Get().GetSelectedEntities();
                        entt::entity primary = EditorSelection::Get().GetPrimary();
                        std::vector<entt::entity> newSelection;
                        entt::entity newPrimary = entt::null;
                        for (auto e : selected)
                        {
                            if (e != entt::null && registry.valid(e) && !isDescendantOfAny(e, selected))
                            {
                                Entity duplicated = scene.DuplicateEntity(e);
                                if (duplicated.GetHandle() != entt::null)
                                {
                                    newSelection.push_back(duplicated.GetHandle());
                                    if (e == primary)
                                        newPrimary = duplicated.GetHandle();
                                }
                            }
                        }
                        if (!newSelection.empty())
                        {
                            CommandHistory::Get().Execute(std::make_unique<DuplicateEntityCommand>(&scene, newSelection));
                            EditorSelection::Get().SetSelection(newSelection, newPrimary);
                        }
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Duplicate Entity", "Ctrl+D"))
                    {
                        Entity duplicated = scene.DuplicateEntity(entity);
                        if (duplicated.GetHandle() != entt::null)
                        {
                            CommandHistory::Get().Execute(std::make_unique<DuplicateEntityCommand>(&scene, std::vector<entt::entity>{ duplicated.GetHandle() }));
                            selectSingle(duplicated.GetHandle());
                        }
                    }
                }

                ImGui::Separator();

                if (EditorSelection::Get().GetCount() > 1)
                {
                    if (ImGui::MenuItem("Delete Selected Entities", "Del"))
                    {
                        auto selected = EditorSelection::Get().GetSelectedEntities();
                        if (!selected.empty())
                        {
                            auto cmd = std::make_unique<DeleteEntityCommand>(&scene, selected);
                            CommandHistory::Get().Execute(std::move(cmd));
                        }
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Delete Entity", "Del"))
                    {
                        auto cmd = std::make_unique<DeleteEntityCommand>(&scene, std::vector<entt::entity>{ entity });
                        CommandHistory::Get().Execute(std::move(cmd));
                    }
                }
                ImGui::EndPopup();
            }

            if (!isRoot && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                m_PendingSingleSelect = entt::null;
                entt::entity e = entity;
                ImGui::SetDragDropPayload("ENTITY_DRAG", &e, sizeof(entt::entity));
                bool isSelected = EditorSelection::Get().IsSelected(entity);
                size_t count = isSelected ? EditorSelection::Get().GetCount() : 1;
                if (count > 1)
                {
                    ImGui::Text("%s (+ %d others)", tagComponent.tag.c_str(), (int)(count - 1));
                }
                else
                {
                    ImGui::Text("%s", tagComponent.tag.c_str());
                }
                ImGui::EndDragDropSource();
            }

            if (!isRoot && ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG", ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
                {
                    ImVec2 itemMin = ImGui::GetItemRectMin();
                    ImVec2 itemMax = ImGui::GetItemRectMax();
                    float itemHeight = itemMax.y - itemMin.y;
                    float mousePosY = ImGui::GetMousePos().y;

                    float topBorder = itemMin.y + (itemHeight * 0.25f);
                    float bottomBorder = itemMax.y - (itemHeight * 0.25f);

                    ImDrawList* drawList = ImGui::GetWindowDrawList();

                    entt::entity dragged = *(entt::entity*)payload->Data;
                    bool draggedInSelection = EditorSelection::Get().IsSelected(dragged);
                    std::vector<entt::entity> toMove = (draggedInSelection && EditorSelection::Get().GetCount() > 1)
                        ? EditorSelection::Get().GetSelectedEntities()
                        : std::vector<entt::entity>{ dragged };

                    if (mousePosY < topBorder)
                    {
                        bool canReorder = false;
                        for (auto targetMove : toMove)
                        {
                            if (targetMove != entt::null && registry.valid(targetMove) && targetMove != entity)
                            {
                                if (!isDescendantOf(targetMove, entity, isDescendantOf) && !isDescendantOfAny(targetMove, toMove))
                                {
                                    canReorder = true;
                                    break;
                                }
                            }
                        }

                        if (canReorder)
                        {
                            drawList->AddLine(ImVec2(itemMin.x, itemMin.y), ImVec2(itemMax.x, itemMin.y), IM_COL32(255, 204, 0, 255), 2.0f);
                            if (payload->IsDelivery())
                            {
                                auto compoundCmd = std::make_unique<CompoundCommand>("Reorder Entities");
                                for (auto targetMove : toMove)
                                {
                                    if (targetMove != entt::null && registry.valid(targetMove) && targetMove != entity)
                                    {
                                        if (!isDescendantOf(targetMove, entity, isDescendantOf) && !isDescendantOfAny(targetMove, toMove))
                                        {
                                            auto cmd = std::make_unique<ReorderEntityCommand>(&scene, targetMove, entity, true);
                                            scene.MoveChildOrder(targetMove, entity, true);
                                            compoundCmd->Add(std::move(cmd));
                                        }
                                    }
                                }
                                if (!compoundCmd->IsEmpty())
                                {
                                    CommandHistory::Get().Execute(std::move(compoundCmd));
                                }
                            }
                        }
                    }
                    else if (mousePosY > bottomBorder)
                    {
                        bool canReorder = false;
                        for (auto targetMove : toMove)
                        {
                            if (targetMove != entt::null && registry.valid(targetMove) && targetMove != entity)
                            {
                                if (!isDescendantOf(targetMove, entity, isDescendantOf) && !isDescendantOfAny(targetMove, toMove))
                                {
                                    canReorder = true;
                                    break;
                                }
                            }
                        }

                        if (canReorder)
                        {
                            drawList->AddLine(ImVec2(itemMin.x, itemMax.y), ImVec2(itemMax.x, itemMax.y), IM_COL32(255, 204, 0, 255), 2.0f);
                            if (payload->IsDelivery())
                            {
                                auto compoundCmd = std::make_unique<CompoundCommand>("Reorder Entities");
                                for (auto it = toMove.rbegin(); it != toMove.rend(); ++it)
                                {
                                    auto targetMove = *it;
                                    if (targetMove != entt::null && registry.valid(targetMove) && targetMove != entity)
                                    {
                                        if (!isDescendantOf(targetMove, entity, isDescendantOf) && !isDescendantOfAny(targetMove, toMove))
                                        {
                                            auto cmd = std::make_unique<ReorderEntityCommand>(&scene, targetMove, entity, false);
                                            scene.MoveChildOrder(targetMove, entity, false);
                                            compoundCmd->Add(std::move(cmd));
                                        }
                                    }
                                }
                                if (!compoundCmd->IsEmpty())
                                {
                                    CommandHistory::Get().Execute(std::move(compoundCmd));
                                }
                            }
                        }
                    }
                    else
                    {
                        bool canReparent = false;
                        for (auto targetMove : toMove)
                        {
                            if (targetMove != entt::null && registry.valid(targetMove) && targetMove != entity)
                            {
                                if (!isDescendantOf(targetMove, entity, isDescendantOf) && !isDescendantOfAny(targetMove, toMove))
                                {
                                    if (scene.GetParent(targetMove) != entity)
                                    {
                                        canReparent = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if (canReparent)
                        {
                            drawList->AddRect(itemMin, itemMax, IM_COL32(0, 180, 255, 255), 0.0f, 0, 1.5f);
                            if (payload->IsDelivery())
                            {
                                auto compoundCmd = std::make_unique<CompoundCommand>("Reparent Entities");
                                for (auto targetMove : toMove)
                                {
                                    if (targetMove != entt::null && registry.valid(targetMove) && targetMove != entity)
                                    {
                                        if (!isDescendantOf(targetMove, entity, isDescendantOf) && !isDescendantOfAny(targetMove, toMove))
                                        {
                                            if (scene.GetParent(targetMove) != entity)
                                            {
                                                compoundCmd->Add(std::make_unique<ReparentCommand>(&scene, targetMove, entity));
                                            }
                                        }
                                    }
                                }
                                if (!compoundCmd->IsEmpty())
                                {
                                    CommandHistory::Get().Execute(std::move(compoundCmd));
                                }
                            }
                        }
                    }
                }

                if (const ImGuiPayload* assetPayload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    std::string pathStr((const char*)assetPayload->Data);
                    if (pathStr.ends_with(".rprefab"))
                    {
                        AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                        if (uuid.IsValid())
                        {
                            Entity inst = scene.Instantiate(uuid, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), entity);
                            if (inst.GetHandle() != entt::null)
                                EditorSelection::Get().Select(inst.GetHandle());
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (isRoot)
            {
                auto childrenCopy = children;
                for (auto child : childrenCopy)
                    DrawEntityNode(child);
            }
            else if (nodeOpen)
            {
                auto childrenCopy = children;
                for (auto child : childrenCopy)
                    DrawEntityNode(child);

                ImGui::TreePop();
            }
        };

    auto rootChildren = getChildren(rootEntity);
    for (auto entity : rootChildren)
    {
        DrawEntityNode(entity);
    }

    if (m_PendingSingleSelect != entt::null)
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f) || ImGui::IsDragDropActive())
        {
            m_PendingSingleSelect = entt::null;
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            EditorSelection::Get().Select(m_PendingSingleSelect);
            m_SelectionAnchor = m_PendingSingleSelect;
            m_PendingSingleSelect = entt::null;
        }
    }

    if (m_PendingShiftSelect != entt::null)
    {
        entt::entity anchor = m_SelectionAnchor;
        if (anchor == entt::null || !registry.valid(anchor) || !EditorSelection::Get().IsSelected(anchor))
        {
            anchor = EditorSelection::Get().GetPrimary();
        }
        if (anchor == entt::null || !registry.valid(anchor))
        {
            anchor = m_PendingShiftSelect;
        }

        auto itA = std::find(visibleOrder.begin(), visibleOrder.end(), anchor);
        auto itB = std::find(visibleOrder.begin(), visibleOrder.end(), m_PendingShiftSelect);

        if (itA != visibleOrder.end() && itB != visibleOrder.end())
        {
            size_t idxA = std::distance(visibleOrder.begin(), itA);
            size_t idxB = std::distance(visibleOrder.begin(), itB);
            size_t start = std::min(idxA, idxB);
            size_t end = std::max(idxA, idxB);

            std::vector<entt::entity> range;
            range.reserve(end - start + 1);
            for (size_t i = start; i <= end; ++i)
            {
                range.push_back(visibleOrder[i]);
            }
            EditorSelection::Get().SelectRange(range, m_PendingShiftSelect);
            m_SelectionAnchor = anchor;
        }
        else
        {
            EditorSelection::Get().Select(m_PendingShiftSelect);
            m_SelectionAnchor = m_PendingShiftSelect;
        }
        m_PendingShiftSelect = entt::null;
    }

    m_VisibleEntities = visibleOrder;

    if (searchFilter[0] != '\0' && visibleOrder.empty())
        ImGui::TextDisabled("No entities found");

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->Rect(), ImGui::GetID("SceneHierarchyEmptyDrop")))
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG"))
        {
            entt::entity dragged = *(entt::entity*)payload->Data;
            bool draggedInSelection = EditorSelection::Get().IsSelected(dragged);
            std::vector<entt::entity> toMove = (draggedInSelection && EditorSelection::Get().GetCount() > 1) ? EditorSelection::Get().GetSelectedEntities() : std::vector<entt::entity>{ dragged };
            auto compoundCmd = std::make_unique<CompoundCommand>("Detach from Parent");
            for (auto targetMove : toMove)
            {
                if (targetMove != entt::null && registry.valid(targetMove) && !isDescendantOfAny(targetMove, toMove))
                {
                    if (scene.GetParent(targetMove) != rootEntity)
                    {
                        compoundCmd->Add(std::make_unique<ReparentCommand>(&scene, targetMove, rootEntity));
                    }
                }
            }
            if (!compoundCmd->IsEmpty())
            {
                CommandHistory::Get().Execute(std::move(compoundCmd));
            }
        }

        if (const ImGuiPayload* assetPayload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
        {
            std::string pathStr((const char*)assetPayload->Data);
            if (pathStr.ends_with(".rprefab"))
            {
                AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                if (uuid.IsValid())
                {
                    Entity inst = scene.Instantiate(uuid, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), rootEntity);
                    if (inst.GetHandle() != entt::null)
                        EditorSelection::Get().Select(inst.GetHandle());
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (!hierarchyIO.KeyCtrl && !hierarchyIO.KeyShift && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && !ImGui::IsAnyItemHovered())
    {
        EditorSelection::Get().Clear();
        m_SelectionAnchor = entt::null;
        m_PendingSingleSelect = entt::null;
    }

    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Entity"))
        {
            registerCreatedEntity(scene.CreateEntity("New Entity"));
        }

        if (ImGui::MenuItem("Camera"))
        {
            entt::entity cam = scene.CreateEntity("Camera");
            scene.AddComponent<CameraComponent>(cam);
            registerCreatedEntity(cam);
        }

        if (ImGui::MenuItem("Particle Emitter"))
        {
            Entity emitterEntity = LoadPrimitive(scene, "Quad");
            if (emitterEntity)
            {
                if (emitterEntity.HasComponent<RigidbodyComponent>())
                    emitterEntity.RemoveComponent<RigidbodyComponent>();
                if (emitterEntity.HasComponent<ConvexHullColliderComponent>())
                    emitterEntity.RemoveComponent<ConvexHullColliderComponent>();

                emitterEntity.GetComponent<TagComponent>().tag = "Particle Emitter";
                emitterEntity.AddComponent<ParticleEmitterComponent>();
                registerCreatedEntity(emitterEntity.GetHandle());
            }
        }

        if (ImGui::MenuItem("Audio Emitter"))
        {
            entt::entity audio = scene.CreateEntity("AudioEmitter");
            scene.AddComponent<AudioEmitterComponent>(audio);
            registerCreatedEntity(audio);
        }

        if (ImGui::MenuItem("Decal"))
        {
            entt::entity decalEnt = scene.CreateEntity("Decal");
            scene.GetComponent<TransformComponent>(decalEnt).SetEulerRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
            scene.AddComponent<DecalComponent>(decalEnt);
            registerCreatedEntity(decalEnt);
        }

        if (ImGui::BeginMenu("Primitives"))
        {
            if (ImGui::MenuItem("Cube")) registerCreatedEntity(LoadPrimitive(scene, "Cube").GetHandle());
            if (ImGui::MenuItem("Sphere")) registerCreatedEntity(LoadPrimitive(scene, "Sphere").GetHandle());
            if (ImGui::MenuItem("Cylinder")) registerCreatedEntity(LoadPrimitive(scene, "Cylinder").GetHandle());
            if (ImGui::MenuItem("Quad")) registerCreatedEntity(LoadPrimitive(scene, "Quad").GetHandle());
            if (ImGui::MenuItem("Cone")) registerCreatedEntity(LoadPrimitive(scene, "Cone").GetHandle());
            if (ImGui::MenuItem("Capsule")) registerCreatedEntity(LoadPrimitive(scene, "Capsule").GetHandle());
            if (ImGui::MenuItem("Torus")) registerCreatedEntity(LoadPrimitive(scene, "Torus").GetHandle());
            if (ImGui::MenuItem("Plane")) registerCreatedEntity(LoadPrimitive(scene, "Plane").GetHandle());
            if (ImGui::MenuItem("Monkey")) registerCreatedEntity(LoadPrimitive(scene, "Monkey").GetHandle());
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Lights"))
        {
            if (ImGui::MenuItem("Directional Light"))
            {
                entt::entity light = scene.CreateEntity("DirectionalLight");
                scene.GetComponent<TransformComponent>(light).SetEulerRotation(glm::vec3(-60.0f, -90.0f, 0.0f));
                scene.AddComponent<DirectionalLightComponent>(light);
                registerCreatedEntity(light);
            }
            if (ImGui::MenuItem("Point Light"))
            {
                entt::entity light = scene.CreateEntity("PointLight");
                scene.AddComponent<PointLightComponent>(light);
                registerCreatedEntity(light);
            }
            if (ImGui::MenuItem("Spot Light"))
            {
                entt::entity light = scene.CreateEntity("SpotLight");
                scene.GetComponent<TransformComponent>(light).SetEulerRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
                scene.AddComponent<SpotLightComponent>(light);
                registerCreatedEntity(light);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("UI"))
        {
            if (ImGui::MenuItem("UI Canvas"))
            {
                entt::entity canvas = scene.CreateEntity("Canvas");
                scene.AddComponent<UICanvasComponent>(canvas);
                scene.AddComponent<RectTransformComponent>(canvas);
                registerCreatedEntity(canvas);
            }
            if (ImGui::MenuItem("UI Text"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity text = scene.CreateEntity("UI Text");
                scene.AddComponent<RectTransformComponent>(text);
                scene.AddComponent<UITextComponent>(text);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(text, parent);
                }
                registerCreatedEntity(text);
            }
            if (ImGui::MenuItem("UI Button"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity button = scene.CreateEntity("UI Button");
                scene.AddComponent<RectTransformComponent>(button);
                scene.AddComponent<UIButtonComponent>(button);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(button, parent);
                }
                registerCreatedEntity(button);
            }
            if (ImGui::MenuItem("UI Image"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity img = scene.CreateEntity("UI Image");
                scene.AddComponent<RectTransformComponent>(img);
                scene.AddComponent<UIImageComponent>(img);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(img, parent);
                }
                registerCreatedEntity(img);
            }
            if (ImGui::MenuItem("UI Slider"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity slider = scene.CreateEntity("UI Slider");
                scene.AddComponent<RectTransformComponent>(slider);
                scene.AddComponent<UISliderComponent>(slider);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(slider, parent);
                }
                registerCreatedEntity(slider);
            }
            if (ImGui::MenuItem("UI Progress Bar"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity pbar = scene.CreateEntity("UI Progress Bar");
                scene.AddComponent<RectTransformComponent>(pbar);
                scene.AddComponent<UIProgressBarComponent>(pbar);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(pbar, parent);
                }
                registerCreatedEntity(pbar);
            }
            if (ImGui::MenuItem("UI Checkbox"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity cb = scene.CreateEntity("UI Checkbox");
                scene.AddComponent<RectTransformComponent>(cb);
                scene.AddComponent<UICheckboxComponent>(cb);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(cb, parent);
                }
                registerCreatedEntity(cb);
            }
            if (ImGui::MenuItem("UI Panel"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity panel = scene.CreateEntity("UI Panel");
                auto& rect = scene.AddComponent<RectTransformComponent>(panel);
                rect.size = { 300.0f, 200.0f };
                scene.AddComponent<UIPanelComponent>(panel);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(panel, parent);
                }
                registerCreatedEntity(panel);
            }
            if (ImGui::MenuItem("UI Vertical Container"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity vbox = scene.CreateEntity("UI Vertical Container");
                auto& rect = scene.AddComponent<RectTransformComponent>(vbox);
                rect.size = { 200.0f, 300.0f };
                auto& lg = scene.AddComponent<UILayoutGroupComponent>(vbox);
                lg.type = UILayoutType::Vertical;
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(vbox, parent);
                }
                registerCreatedEntity(vbox);
            }
            if (ImGui::MenuItem("UI Horizontal Container"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity hbox = scene.CreateEntity("UI Horizontal Container");
                auto& rect = scene.AddComponent<RectTransformComponent>(hbox);
                rect.size = { 400.0f, 60.0f };
                auto& lg = scene.AddComponent<UILayoutGroupComponent>(hbox);
                lg.type = UILayoutType::Horizontal;
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(hbox, parent);
                }
                registerCreatedEntity(hbox);
            }
            if (ImGui::MenuItem("UI Text Input"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity input = scene.CreateEntity("UI Text Input");
                auto& rect = scene.AddComponent<RectTransformComponent>(input);
                rect.size = { 200.0f, 36.0f };
                scene.AddComponent<UITextInputComponent>(input);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(input, parent);
                }
                registerCreatedEntity(input);
            }
            if (ImGui::MenuItem("UI Scroll View"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity sv = scene.CreateEntity("UI Scroll View");
                auto& rect = scene.AddComponent<RectTransformComponent>(sv);
                rect.size = { 250.0f, 200.0f };
                scene.AddComponent<UIScrollRectComponent>(sv);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(sv, parent);
                }
                registerCreatedEntity(sv);
            }
            if (ImGui::MenuItem("UI Tab Container"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity tabs = scene.CreateEntity("UI Tab Container");
                auto& rect = scene.AddComponent<RectTransformComponent>(tabs);
                rect.size = { 350.0f, 250.0f };
                scene.AddComponent<UITabContainerComponent>(tabs);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(tabs, parent);
                }
                registerCreatedEntity(tabs);
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    if ((ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) && GImGui && GImGui->ActiveIdHasBeenEditedThisFrame)
    {
        scene.SetDirty(true);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

