#include "InspectorPanel.h"
#include "ImGui/imgui.h"
#include "Core/Engine.h"
#include "Renderer/RenderLayer.h"
#include "EditorUtils.h"
#include "AssetImporters/MaterialSerializer.h"
#include "Audio/AudioManager.h"

using namespace rv;

void InspectorPanel::Draw(Engine* engine, entt::entity& selectedEntity)
{
    Scene& scene = engine->GetActiveScene();
    entt::registry& registry = scene.GetRegistry();
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

    if (selectedEntity != entt::null)
    {
        if (selectedEntity != entt::null)
        {
            if (registry.any_of<TagComponent>(selectedEntity))
            {
                auto& tag = registry.get<TagComponent>(selectedEntity);
                static char buffer[256];
                std::strncpy(buffer, tag.tag.c_str(), sizeof(buffer));

                static bool enabled = true; // placeholder
                ImGui::Checkbox("##enabled", &enabled);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 120.0f);
                if (ImGui::InputText("##tagname", buffer, sizeof(buffer)))
                    if (buffer[0] != '\0') tag.tag = buffer;
            }

            ImGui::SameLine();
            if (ImGui::Button("Add Component", ImVec2(110.0f, 0)))
                ImGui::OpenPopup("AddComponentPopup");

            ImGui::Separator();

            if (ImGui::BeginPopup("AddComponentPopup"))
            {
                if (!registry.any_of<ScriptComponent>(selectedEntity))
                    if (ImGui::MenuItem("Script")) { registry.emplace<ScriptComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<MaterialComponent>(selectedEntity))
                    if (ImGui::MenuItem("Material")) { registry.emplace<MaterialComponent>(selectedEntity, AssetUUID::FromString("ee3dde12-6263-4f11-bb1d-812b3e196ab7")); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<MeshComponent>(selectedEntity))
                    if (ImGui::MenuItem("Mesh")) { registry.emplace<MeshComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<MeshRendererComponent>(selectedEntity))
                    if (ImGui::MenuItem("Mesh renderer")) { registry.emplace<MeshRendererComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<PointLightComponent>(selectedEntity))
                    if (ImGui::MenuItem("Point light")) { registry.emplace<PointLightComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<DirectionalLightComponent>(selectedEntity))
                    if (ImGui::MenuItem("Directional light")) { registry.emplace<DirectionalLightComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<RigidbodyComponent>(selectedEntity))
                    if (ImGui::MenuItem("Rigidbody")) { registry.emplace<RigidbodyComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<CharacterBodyComponent>(selectedEntity))
                    if (ImGui::MenuItem("CharacterBody")) { registry.emplace<CharacterBodyComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<BoxColliderComponent>(selectedEntity))
                    if (ImGui::MenuItem("BoxCollider")) { registry.emplace<BoxColliderComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<SphereColliderComponent>(selectedEntity))
                    if (ImGui::MenuItem("SphereCollider")) { registry.emplace<SphereColliderComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<CapsuleColliderComponent>(selectedEntity))
                    if (ImGui::MenuItem("CapsuleCollider")) { registry.emplace<CapsuleColliderComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<CylinderColliderComponent>(selectedEntity))
                    if (ImGui::MenuItem("CylinderCollider")) { registry.emplace<CylinderColliderComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<MeshColliderComponent>(selectedEntity))
                    if (ImGui::MenuItem("MeshCollider")) { registry.emplace<MeshColliderComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<ConvexHullColliderComponent>(selectedEntity))
                    if (ImGui::MenuItem("ConvexHullCollider")) { registry.emplace<ConvexHullColliderComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                if (!registry.any_of<AudioEmitterComponent>(selectedEntity))
                    if (ImGui::MenuItem("AudioEmitter")) { registry.emplace<AudioEmitterComponent>(selectedEntity); ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }
        }

        bool isPrefab = registry.any_of<PrefabComponent>(selectedEntity);


        // ---------- Transform ----------
        if (registry.any_of<TransformComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Transform"))
            {
                auto& transform = registry.get<TransformComponent>(selectedEntity);

                glm::vec3 euler = transform.GetEulerRotation();
                bool isScaleRatioLocked = transform.IsScaleRatioLocked();

                glm::vec3 pos = transform.GetPosition();
                if (ImGui::DragFloat3("Position", &pos[0], 0.1f))
                {
                    transform.SetPosition(pos);
                    transform.SetDirty();
                }

                if (ImGui::DragFloat3("Rotation", &euler[0], 0.1f, -360, 360))
                {
                    transform.SetEulerRotation(euler);
                    transform.SetDirty();
                }

                ImGui::PushID("ScaleControls");
                glm::vec3 scale = transform.GetScale();
                if (ImGui::DragFloat3("Scale", &scale[0], 0.1f, 0.01f, 10.0f))
                {
                    transform.SetScale(scale);
                    transform.SetDirty();
                }
                if (ImGui::Checkbox("Lock Ratio", &isScaleRatioLocked))
                {
                    transform.SetLockScaleRatio(isScaleRatioLocked);
                }
                ImGui::PopID();

                if (ImGui::Button("Reset Transform"))
                {
                    transform.SetPosition({ 0.0f, 0.0f, 0.0f });
                    transform.SetEulerRotation({ 0.0f, 0.0f, 0.0f });
                    transform.SetScale({ 1.0f, 1.0f, 1.0f });
                }
            }
        }

        if (!isPrefab)
        {
            // ---------- Mesh ----------
            if (registry.any_of<MeshComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("Mesh");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("MeshComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<MeshComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    ImGui::Indent();
                    MeshComponent& mesh = registry.get<MeshComponent>(selectedEntity);

                    ImGui::Button("Mesh Slot", ImVec2(200, 20));
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                        {
                            const char* path = (const char*)payload->Data;
                            std::string pathStr(path);
                            if (pathStr.ends_with(".obj") || pathStr.ends_with(".fbx") ||
                                pathStr.ends_with(".gltf") || pathStr.ends_with(".glb") ||
                                pathStr.ends_with(".rmesh"))
                            {
                                AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                                if (uuid.IsValid())
                                    mesh.SetMesh(uuid);
                                else
                                    LOG_ERROR("Could not find UUID for mesh: {}", pathStr);
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }

                    ImGui::Unindent();
                }
            }

            // ---------- Camera ----------
            if (registry.any_of<CameraComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("Camera");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("CameraComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<CameraComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& camComp = registry.get<CameraComponent>(selectedEntity);
                    auto& camera = camComp.camera;

                    ImGui::Checkbox("Is Active", &camComp.isActive);

                    const char* projectionTypes[] = { "Perspective", "Orthographic" };
                    int current = static_cast<int>(camera.ProjectionType);

                    if (ImGui::Combo("Projection", &current, projectionTypes, 2))
                        camera.ProjectionType = static_cast<Camera::Projection>(current);

                    if (camera.ProjectionType == Camera::Projection::Perspective)
                        ImGui::SliderFloat("Fov", &camera.FOV, 1.0f, 120.0f);
                    else
                        ImGui::SliderFloat("Ortho Size", &camera.OrthoSize, 0.1f, 100.0f);

                    ImGui::SliderFloat("Near clip", &camera.NearClip, 0.001f, 10.0f);
                    ImGui::SliderFloat("Far clip", &camera.FarClip, 1.0f, 5000.0f);
                }
            }

            // ---------- Mesh Renderer ----------
            if (registry.any_of<MeshRendererComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("Mesh Renderer");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("MeshRendererComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<MeshRendererComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    ImGui::Indent();
                    MeshRendererComponent& meshRenderer = registry.get<MeshRendererComponent>(selectedEntity);
                    bool isCastShadow = meshRenderer.IsCastShadow();
                    if (ImGui::Checkbox("Cast Shadow", &isCastShadow))
                        meshRenderer.SetCastShadow(isCastShadow);

                    ImGui::Unindent();
                }
            }

            // ---------- Point Light ----------
            if (registry.any_of<PointLightComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("Point Light");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("PointLightComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<PointLightComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& pointLightComponent = registry.get<PointLightComponent>(selectedEntity);
                    glm::vec3 lightColor = pointLightComponent.color;
                    float color[3] = { lightColor.r, lightColor.g, lightColor.b };
                    if (ImGui::ColorPicker3("Light Color", color))
                        pointLightComponent.color = glm::vec3(color[0], color[1], color[2]);

                    ImGui::Checkbox("CastShadows", &pointLightComponent.castShadows);
                    ImGui::SliderFloat("Intensity", &pointLightComponent.intensity, 0.0f, 20.0f);
                    ImGui::SliderFloat("Radius", &pointLightComponent.radius, 0.1f, 50.0f);
                    ImGui::SliderFloat("Falloff", &pointLightComponent.falloff, 0.0f, 10.0f);
                    ImGui::SliderFloat("Shadow bias", &pointLightComponent.shadowBias, 0.0f, 0.1f);
                    ImGui::SliderFloat("Blur radius", &pointLightComponent.blurRadius, 0.0f, 0.1f);
                    ImGui::Checkbox("Reverse cull face", &pointLightComponent.reverseCullFace);
                }
            }

            // ---------- Directional Light ----------
            if (registry.any_of<DirectionalLightComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("Directional Light");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("DirectionalLightComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<DirectionalLightComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& directionalLightComponent = registry.get<DirectionalLightComponent>(selectedEntity);
                    glm::vec3 lightColor = directionalLightComponent.color;
                    float color[3] = { lightColor.r, lightColor.g, lightColor.b };
                    if (ImGui::ColorPicker3("Light Color", color))
                        directionalLightComponent.color = glm::vec3(color[0], color[1], color[2]);

                    ImGui::SliderFloat("Intensity", &directionalLightComponent.intensity, 0.0f, 10.0f);
                    ImGui::Checkbox("Cast Shadow", &directionalLightComponent.castShadows);
                    ImGui::SliderFloat("Shadow bias", &directionalLightComponent.shadowBias, 0.0f, 0.1f);
                    ImGui::SliderFloat("Blur radius", &directionalLightComponent.blurRadius, 0.0f, 2.0f);
                    ImGui::Checkbox("Reverse cull face", &directionalLightComponent.reverseCullFace);
                }
            }

            // ---------- Rigidbody ----------
            if (registry.any_of<RigidbodyComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("Rigidbody");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("RigidbodyComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<RigidbodyComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& rb = registry.get<RigidbodyComponent>(selectedEntity);

                    const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
                    int current = static_cast<int>(rb.bodyType);

                    if (ImGui::Combo("Body type", &current, bodyTypes, 3))
                        rb.bodyType = static_cast<Physics::MotionType>(current);

                    ImGui::Checkbox("Auto mass", &rb.autoCalculateMass);
                    if (!rb.autoCalculateMass)
                        ImGui::SliderFloat("Mass", &rb.mass, 0.001f, 1000.0f);
                    ImGui::SliderFloat("Friction", &rb.friction, 0.0f, 1.0f);
                    ImGui::SliderFloat("Restitution", &rb.restitution, 0.0f, 1.0f);
                    ImGui::SliderFloat("Linear Damping", &rb.linearDamping, 0.0f, 1.0f);
                    ImGui::SliderFloat("Angular Damping", &rb.angularDamping, 0.0f, 1.0f);
                    ImGui::SliderFloat("Max linear velocity", &rb.maxLinearVelocity, 0.0f, 1000.0f);
                    ImGui::SliderFloat("Max angular velocity", &rb.maxAngularVelocity, 0.0f, 100.0f);
                    ImGui::Checkbox("Allow sleeping", &rb.allowSleep);

                    if (ImGui::CollapsingHeader("Constraints"))
                    {
                        ImGui::Checkbox("Lock Rotation X", &rb.lockRotationX);
                        ImGui::Checkbox("Lock Rotation Y", &rb.lockRotationY);
                        ImGui::Checkbox("Lock Rotation Z", &rb.lockRotationZ);
                        ImGui::Checkbox("Lock Translation X", &rb.lockTranslationX);
                        ImGui::Checkbox("Lock Translation Y", &rb.lockTranslationY);
                        ImGui::Checkbox("Lock Translation Z", &rb.lockTranslationZ);
                    }

                    if (ImGui::CollapsingHeader("Collision Filter"))
                    {
                        auto& filter = rb.collisionFilter;

                        if (ImGui::TreeNode("Layer"))
                        {
                            for (int i = 0; i < 32; i++)
                            {
                                bool enabled = (filter.layer & (1u << i)) != 0;
                                if (enabled) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

                                std::string label = std::to_string(i + 1);
                                if (ImGui::Button(label.c_str(), ImVec2(22, 22)))
                                {
                                    if (enabled) filter.layer &= ~(1u << i);
                                    else        filter.layer |= (1u << i);
                                }

                                if (enabled) ImGui::PopStyleColor();
                                if ((i % 8) != 7) ImGui::SameLine();
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Mask"))
                        {
                            for (int i = 0; i < 32; i++)
                            {
                                bool enabled = (filter.mask & (1u << i)) != 0;
                                if (enabled) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

                                std::string label = std::to_string(i + 1);
                                if (ImGui::Button(label.c_str(), ImVec2(22, 22)))
                                {
                                    if (enabled) filter.mask &= ~(1u << i);
                                    else        filter.mask |= (1u << i);
                                }

                                if (enabled) ImGui::PopStyleColor();
                                if ((i % 8) != 7) ImGui::SameLine();
                            }
                            ImGui::TreePop();
                        }
                    }

                    ImGui::SliderFloat("Gravity factor", &rb.gravityFactor, 0.0f, 10.0f);
                    ImGui::Checkbox("Is sensor", &rb.isSensor);
                    ImGui::Checkbox("Use CCD", &rb.useCCD);
                }
            }

            // ---------- CharacterBody ----------
            if (registry.any_of<CharacterBodyComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("CharacterBody");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("CharacterBodyComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<CharacterBodyComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& cb = registry.get<CharacterBodyComponent>(selectedEntity);

                    ImGui::SliderFloat("Mass", &cb.mass, 0.001f, 1000.0f);
                    ImGui::SliderFloat("Max Strength", &cb.maxStrength, 0.0f, 200.0f);
                    ImGui::DragFloat3("Shape offset", &cb.shapeOffset[0], 0.001f, 1000.0f);
                    ImGui::SliderFloat("Predictive contact distance", &cb.predictiveContactDistance, 0.0f, 0.2f);
                    ImGui::SliderFloat("Max slope angle", &cb.maxSlopeAngle, 0.0f, 89.0f);

                    if (ImGui::CollapsingHeader("Collision Filter"))
                    {
                        auto& filter = cb.collisionFilter;

                        if (ImGui::TreeNode("Layer"))
                        {
                            for (int i = 0; i < 32; i++)
                            {
                                bool enabled = (filter.layer & (1u << i)) != 0;
                                if (enabled) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

                                std::string label = std::to_string(i + 1);
                                if (ImGui::Button(label.c_str(), ImVec2(22, 22)))
                                {
                                    if (enabled) filter.layer &= ~(1u << i);
                                    else        filter.layer |= (1u << i);
                                }

                                if (enabled) ImGui::PopStyleColor();
                                if ((i % 8) != 7) ImGui::SameLine();
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Mask"))
                        {
                            for (int i = 0; i < 32; i++)
                            {
                                bool enabled = (filter.mask & (1u << i)) != 0;
                                if (enabled) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

                                std::string label = std::to_string(i + 1);
                                if (ImGui::Button(label.c_str(), ImVec2(22, 22)))
                                {
                                    if (enabled) filter.mask &= ~(1u << i);
                                    else        filter.mask |= (1u << i);
                                }

                                if (enabled) ImGui::PopStyleColor();
                                if ((i % 8) != 7) ImGui::SameLine();
                            }
                            ImGui::TreePop();
                        }
                    }
                }
            }

            // ---------- BoxCollider ----------
            if (registry.any_of<BoxColliderComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("BoxCollider");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("BoxColliderComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<BoxColliderComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& bc = registry.get<BoxColliderComponent>(selectedEntity);
                    glm::vec3 size = bc.GetSize();
                    glm::vec3 offset = bc.GetOffset();
                    if (ImGui::DragFloat3("Size", &size[0], 0.001f, 1000.0f)) bc.SetSize(size);
                    if (ImGui::DragFloat3("Offset", &offset[0], 0.001f, 1000.0f)) bc.SetOffset(offset);
                }
            }

            // ---------- SphereCollider ----------
            if (registry.any_of<SphereColliderComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("SphereCollider");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("SphereColliderComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<SphereColliderComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& sc = registry.get<SphereColliderComponent>(selectedEntity);
                    float radius = sc.GetRadius();
                    glm::vec3 offset = sc.GetOffset();
                    if (ImGui::DragFloat("Radius", &radius, 0.001f, 1000.0f)) sc.SetRadius(radius);
                    if (ImGui::DragFloat3("Offset", &offset[0], 0.001f, 1000.0f)) sc.SetOffset(offset);
                }
            }

            // ---------- CapsuleCollider ----------
            if (registry.any_of<CapsuleColliderComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("CapsuleCollider");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("CapsuleColliderComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<CapsuleColliderComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& cc = registry.get<CapsuleColliderComponent>(selectedEntity);
                    float halfHeight = cc.GetHalfHeight();
                    float radius = cc.GetRadius();
                    glm::vec3 offset = cc.GetOffset();
                    if (ImGui::SliderFloat("Half height", &halfHeight, 0.001f, 100.0f)) cc.SetHalfHeight(halfHeight);
                    if (ImGui::SliderFloat("Radius", &radius, 0.001f, 100.0f)) cc.SetRadius(radius);
                    if (ImGui::DragFloat3("Offset", &offset[0], 0.001f, 1000.0f)) cc.SetOffset(offset);
                }
            }

            // ---------- CylinderCollider ----------
            if (registry.any_of<CylinderColliderComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("CylinderCollider");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("CylinderColliderComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<CylinderColliderComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& cc = registry.get<CylinderColliderComponent>(selectedEntity);
                    float halfHeight = cc.GetHalfHeight();
                    float radius = cc.GetRadius();
                    glm::vec3 offset = cc.GetOffset();
                    if (ImGui::SliderFloat("Half height", &halfHeight, 0.001f, 100.0f)) cc.SetHalfHeight(halfHeight);
                    if (ImGui::SliderFloat("Radius", &radius, 0.001f, 100.0f)) cc.SetRadius(radius);
                    if (ImGui::DragFloat3("Offset", &offset[0], 0.001f, 1000.0f)) cc.SetOffset(offset);
                }
            }

            // ---------- MeshCollider ----------
            if (registry.any_of<MeshColliderComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("MeshCollider");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("MeshColliderComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<MeshColliderComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& mc = registry.get<MeshColliderComponent>(selectedEntity);
                    int maxTris = mc.GetMaxTrianglesPerLeaf();
                    float edgeAngle = mc.GetActiveEdgeTresholdAngle();
                    glm::vec3 offset = mc.GetOffset();
                    if (ImGui::SliderInt("Max triangles per leaf", &maxTris, 1, 16)) mc.SetMaxTrianglesPerLeaf(maxTris);
                    if (ImGui::SliderFloat("Active edge treshold angle", &edgeAngle, 0.001f, 2.0f)) mc.SetActiveEdgeTresholdAngle(edgeAngle);
                    if (ImGui::DragFloat3("Offset", &offset[0], 0.001f, 1000.0f)) mc.SetOffset(offset);
                }
            }

            // ---------- ConvexHullCollider ----------
            if (registry.any_of<ConvexHullColliderComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("ConvexHullCollider");
                bool isRemoved = false;

                if (ImGui::BeginPopupContextItem("ConvexHullColliderComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<ConvexHullColliderComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }

                if (open && !isRemoved)
                {
                    auto& chc = registry.get<ConvexHullColliderComponent>(selectedEntity);
                    float maxConvexRadius = chc.GetMaxConvexRadius();
                    glm::vec3 offset = chc.GetOffset();
                    if (ImGui::SliderFloat("Max convex radius", &maxConvexRadius, 0.0f, 0.1f)) chc.SetMaxConvexRadius(maxConvexRadius);
                    if (ImGui::DragFloat3("Offset", &offset[0], 0.001f, 1000.0f)) chc.SetOffset(offset);
                }
            }

            // ---------- Material ----------
            if (registry.any_of<MaterialComponent>(selectedEntity))
            {
                bool open = ImGui::CollapsingHeader("Material");
                bool isRemoved = false;
                if (ImGui::BeginPopupContextItem("MaterialComponentContext"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                    {
                        registry.remove<MaterialComponent>(selectedEntity);
                        isRemoved = true;
                    }
                    ImGui::EndPopup();
                }
                if (open && !isRemoved)
                {
                    MaterialComponent& material = registry.get<MaterialComponent>(selectedEntity);
                    const MaterialInstance& inst = material.GetInstance();

                    ImGui::Button("Material Slot", ImVec2(200, 20));
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                        {
                            std::string pathStr((const char*)payload->Data);
                            if (pathStr.ends_with(".rmat"))
                            {
                                AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                                if (uuid.IsValid()) material.SetMaterial(uuid);
                                else LOG_ERROR("Could not find UUID for material: {}", pathStr);
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }

                    if (inst.GetSourceAsset())
                    {
                        ImGui::SameLine();
                        if (ImGui::Button("Save##matsave"))
                        {
                            auto asset = inst.GetSourceAsset();
                            if (inst.IsOverridden(MatField::AlbedoColor)) asset->albedoColor = material.GetAlbedoColor();
                            if (inst.IsOverridden(MatField::EmissiveColor)) asset->emissiveColor = material.GetEmissiveColor();
                            if (inst.IsOverridden(MatField::EmissiveIntensity)) asset->emissiveIntensity = material.GetEmissiveIntensity();
                            if (inst.IsOverridden(MatField::Metallic)) asset->metallic = material.GetMetallic();
                            if (inst.IsOverridden(MatField::Specular)) asset->specular = material.GetSpecular();
                            if (inst.IsOverridden(MatField::Roughness)) asset->roughness = material.GetRoughness();
                            if (inst.IsOverridden(MatField::AO)) asset->ao = material.GetAO();
                            if (inst.IsOverridden(MatField::NormalScale)) asset->normalScale = material.GetNormalScale();
                            if (inst.IsOverridden(MatField::HeightScale)) asset->heightScale = material.GetHeightScale();
                            if (inst.IsOverridden(MatField::UVScale)) asset->UVScale = material.GetUVScale();
                            if (inst.IsOverridden(MatField::UVOffset)) asset->UVOffset = material.GetUVOffset();

                            if (inst.IsOverridden(MatField::TransparencyMode)) asset->transparencyMode = material.GetTransparencyMode();
                            if (inst.IsOverridden(MatField::BlendMode)) asset->blendMode = material.GetBlendMode();
                            if (inst.IsOverridden(MatField::CullMode)) asset->cullMode = material.GetCullMode();
                            if (inst.IsOverridden(MatField::AlphaCutoff)) asset->alphaCutoff = material.GetAlphaCutoff();

                            if (inst.IsOverridden(MatField::AlbedoTex)) { asset->albedoTextureUUID = material.GetAlbedoTexture() ? material.GetAlbedoTexture()->GetUUID() : AssetUUID{}; asset->useAlbedoMap = material.IsUsingAlbedoMap(); }
                            if (inst.IsOverridden(MatField::NormalTex)) { asset->normalTextureUUID = material.GetNormalTexture() ? material.GetNormalTexture()->GetUUID() : AssetUUID{}; asset->useNormalMap = material.IsUsingNormalMap(); }
                            if (inst.IsOverridden(MatField::MetallicTex)) { asset->metallicTextureUUID = material.GetMetallicTexture() ? material.GetMetallicTexture()->GetUUID() : AssetUUID{}; asset->useMetallicMap = material.IsUsingMetallicMap(); }
                            if (inst.IsOverridden(MatField::RoughnessTex)) { asset->roughnessTextureUUID = material.GetRoughnessTexture() ? material.GetRoughnessTexture()->GetUUID() : AssetUUID{}; asset->useRoughnessMap = material.IsUsingRoughnessMap(); }
                            if (inst.IsOverridden(MatField::AOTex)) { asset->aoTextureUUID = material.GetAOTexture() ? material.GetAOTexture()->GetUUID() : AssetUUID{}; asset->useAOMap = material.IsUsingAOMap(); }
                            if (inst.IsOverridden(MatField::HeightTex)) { asset->heightTextureUUID = material.GetHeightTexture() ? material.GetHeightTexture()->GetUUID() : AssetUUID{}; asset->useHeightMap = material.IsUsingHeightMap(); }

                            auto path = AssetManager::Get().GetRegistry().GetPath(asset->GetUUID());
                            MaterialSerializer::Save(asset, path);
                            material.GetInstance().ClearAllOverrides();
                            material.GetInstance().RebuildCache();
                        }
                    }

                    auto RevertButton = [](const char* id, auto clearFn)
                        {
                            ImGui::SameLine();
                            if (ImGui::Button(id, ImVec2(20, 20))) clearFn();
                        };

                    auto TextureSlot = [&](const char* label, const char* idSuffix, bool usingMap, bool overridden,
                        auto setTexFn, auto clearTexFn)
                        {
                            ImGui::Button(label, ImVec2(200, 20));
                            if (ImGui::BeginDragDropTarget())
                            {
                                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                                {
                                    std::string pathStr((const char*)payload->Data);
                                    static constexpr std::array exts = { ".png", ".jpg", ".jpeg", ".tga", ".hdr", ".rtex" };
                                    bool isImage = std::any_of(exts.begin(), exts.end(), [&](const char* e) { return pathStr.ends_with(e); });
                                    if (isImage)
                                    {
                                        AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                                        if (uuid.IsValid()) setTexFn(uuid);
                                        else LOG_ERROR("Could not find UUID for texture: {}", pathStr);
                                    }
                                }
                                ImGui::EndDragDropTarget();
                            }

                            if (overridden)
                            {
                                ImGui::SameLine();
                                RevertButton((std::string("R##r") + idSuffix).c_str(), clearTexFn);
                            }
                        };

                    if (ImGui::CollapsingHeader("Transparency##transphdr"))
                    {
                        ImGui::Indent();

                        const char* transModes[] = { "Opaque", "Alpha", "Alpha Scissor", "Depth Prepass" };
                        int currentTransMode = static_cast<int>(material.GetTransparencyMode());
                        bool transOvr = inst.IsOverridden(MatField::TransparencyMode);
                        if (ImGui::Combo("Transparency Mode##tmode", &currentTransMode, transModes, 4))
                            material.SetTransparencyMode(static_cast<TransparencyMode>(currentTransMode));
                        if (transOvr) RevertButton("R##tmode", [&]() { material.ClearTransparencyMode(); });

                        if (material.GetTransparencyMode() == TransparencyMode::AlphaScissor)
                        {
                            float cutoff = material.GetAlphaCutoff();
                            bool cutoffOvr = inst.IsOverridden(MatField::AlphaCutoff);
                            if (ImGui::SliderFloat("Alpha Cutoff##acutoff", &cutoff, 0.0f, 1.0f))
                                material.SetAlphaCutoff(cutoff);
                            if (cutoffOvr) RevertButton("R##acutoff", [&]() { material.ClearAlphaCutoff(); });
                        }

                        if (material.GetTransparencyMode() == TransparencyMode::Alpha || material.GetTransparencyMode() == TransparencyMode::DepthPrepass)
                        {
                            const char* blendModes[] = { "Mix", "Add", "Subtract", "Multiply" };
                            int currentBlendMode = static_cast<int>(material.GetBlendMode());
                            bool blendOvr = inst.IsOverridden(MatField::BlendMode);
                            if (ImGui::Combo("Blend Mode##bmode", &currentBlendMode, blendModes, 4))
                                material.SetBlendMode(static_cast<BlendMode>(currentBlendMode));
                            if (blendOvr) RevertButton("R##bmode", [&]() { material.ClearBlendMode(); });
                        }

                        const char* cullModes[] = { "Back", "Front", "Disabled" };
                        int currentCullMode = static_cast<int>(material.GetCullMode());
                        bool cullOvr = inst.IsOverridden(MatField::CullMode);
                        if (ImGui::Combo("Cull Mode##cmode", &currentCullMode, cullModes, 3))
                            material.SetCullMode(static_cast<CullMode>(currentCullMode));
                        if (cullOvr) RevertButton("R##cmode", [&]() { material.ClearCullMode(); });

                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("Shading##shadinghdr"))
                    {
                        ImGui::Indent();

                        const char* shadingModes[] = { "Lit", "Unshaded" };
                        int currentShadingMode = static_cast<int>(material.GetShadingMode());
                        bool shadingOvr = inst.IsOverridden(MatField::ShadingMode);

                        if (ImGui::Combo("Shading Mode##smode", &currentShadingMode, shadingModes, 2))
                            material.SetShadingMode(static_cast<ShadingMode>(currentShadingMode));

                        if (shadingOvr)
                            RevertButton("R##smode", [&]() { material.ClearShadingMode(); });

                        if (material.GetShadingMode() == ShadingMode::Lit)
                        {
                            bool receiveShadows = material.GetReceiveShadows();
                            bool shadowsOvr = inst.IsOverridden(MatField::ReceiveShadows);

                            if (ImGui::Checkbox("Receive Shadows##rshadows", &receiveShadows))
                                material.SetReceiveShadows(receiveShadows);

                            if (shadowsOvr)
                                RevertButton("R##rshadows", [&]() { material.ClearReceiveShadows(); });
                        }

                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("Albedo"))
                    {
                        ImGui::Indent();
                        bool usingMap = material.IsUsingAlbedoMap();
                        bool texOvr = inst.IsOverridden(MatField::AlbedoTex);
                        bool colorOvr = inst.IsOverridden(MatField::AlbedoColor);
                        TextureSlot("Albedo Slot", "alb", usingMap, texOvr,
                            [&](AssetUUID uuid) { material.SetAlbedoTexture(uuid); },
                            [&]() { material.ClearAlbedoTexture(); });
                        glm::vec4 c = material.GetAlbedoColor();
                        float col[4] = { c.r, c.g, c.b, c.a };
                        ImGui::Text("Albedo Color");
                        ImGui::SameLine();
                        if (ImGui::ColorEdit4("##albedocol", col))
                            material.SetAlbedoColor({ col[0], col[1], col[2], col[3]});
                        if (colorOvr)
                            RevertButton("R##albcol", [&]() { material.ClearAlbedoColor(); });
                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("Normal"))
                    {
                        ImGui::Indent();
                        bool usingMap = material.IsUsingNormalMap();
                        bool texOvr = inst.IsOverridden(MatField::NormalTex);
                        bool scaleOvr = inst.IsOverridden(MatField::NormalScale);
                        TextureSlot("Normal Slot", "nrm", usingMap, texOvr,
                            [&](AssetUUID uuid) { material.SetNormalTexture(uuid); },
                            [&]() { material.ClearNormalTexture(); });
                        float normalScale = material.GetNormalScale();
                        if (ImGui::SliderFloat("Normal Scale##nrmscl", &normalScale, -10.0f, 10.0f))
                            material.SetNormalScale(normalScale);
                        if (scaleOvr)
                            RevertButton("R##nrmscl", [&]() { material.ClearNormalScale(); });
                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("Roughness##rhdr"))
                    {
                        ImGui::Indent();
                        bool usingMap = material.IsUsingRoughnessMap();
                        bool texOvr = inst.IsOverridden(MatField::RoughnessTex);
                        bool valOvr = inst.IsOverridden(MatField::Roughness);
                        TextureSlot("Roughness Slot", "rgh", usingMap, texOvr,
                            [&](AssetUUID uuid) { material.SetRoughnessTexture(uuid); },
                            [&]() { material.ClearRoughnessTexture(); });
                        float roughness = material.GetRoughness();
                        if (ImGui::SliderFloat("Roughness##rghval", &roughness, 0.0f, 1.0f))
                            material.SetRoughness(roughness);
                        if (valOvr)
                            RevertButton("R##rghval", [&]() { material.ClearRoughness(); });
                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("Metallic##mhdr"))
                    {
                        ImGui::Indent();
                        bool usingMap = material.IsUsingMetallicMap();
                        bool texOvr = inst.IsOverridden(MatField::MetallicTex);
                        bool mtlOvr = inst.IsOverridden(MatField::Metallic);
                        bool spcOvr = inst.IsOverridden(MatField::Specular);
                        TextureSlot("Metallic Slot", "mtl", usingMap, texOvr,
                            [&](AssetUUID uuid) { material.SetMetallicTexture(uuid); },
                            [&]() { material.ClearMetallicTexture(); });
                        float metallic = material.GetMetallic();
                        if (ImGui::SliderFloat("Metallic##mtlval", &metallic, 0.0f, 1.0f))
                            material.SetMetallic(metallic);
                        if (mtlOvr)
                            RevertButton("R##mtlval", [&]() { material.ClearMetallic(); });
                        float specular = material.GetSpecular();
                        if (ImGui::SliderFloat("Specular##spcval", &specular, 0.0f, 1.0f))
                            material.SetSpecular(specular);
                        if (spcOvr)
                            RevertButton("R##spcval", [&]() { material.ClearSpecular(); });
                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("Height"))
                    {
                        ImGui::Indent();
                        bool usingMap = material.IsUsingHeightMap();
                        bool texOvr = inst.IsOverridden(MatField::HeightTex);
                        bool scaleOvr = inst.IsOverridden(MatField::HeightScale);
                        TextureSlot("Height Slot", "hgt", usingMap, texOvr,
                            [&](AssetUUID uuid) { material.SetHeightTexture(uuid); },
                            [&]() { material.ClearHeightTexture(); });
                        float heightScale = material.GetHeightScale();
                        if (ImGui::SliderFloat("Height Scale##hgtscl", &heightScale, 0.0f, 1.0f))
                            material.SetHeightScale(heightScale);
                        if (scaleOvr)
                            RevertButton("R##hgtscl", [&]() { material.ClearHeightScale(); });
                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("AO##aohdr"))
                    {
                        ImGui::Indent();
                        bool usingMap = material.IsUsingAOMap();
                        bool texOvr = inst.IsOverridden(MatField::AOTex);
                        bool valOvr = inst.IsOverridden(MatField::AO);
                        TextureSlot("AO Slot", "ao", usingMap, texOvr,
                            [&](AssetUUID uuid) { material.SetAOTexture(uuid); },
                            [&]() { material.ClearAOTexture(); });
                        float ao = material.GetAO();
                        if (ImGui::SliderFloat("AO##aoval", &ao, 0.0f, 1.0f))
                            material.SetAO(ao);
                        if (valOvr)
                            RevertButton("R##aoval", [&]() { material.ClearAO(); });
                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("Emissive"))
                    {
                        ImGui::Indent();
                        glm::vec3 ec = material.GetEmissiveColor();
                        float col[3] = { ec.r, ec.g, ec.b };
                        bool ecOvr = inst.IsOverridden(MatField::EmissiveColor);
                        bool eiOvr = inst.IsOverridden(MatField::EmissiveIntensity);
                        if (ImGui::ColorEdit3("##emcol", col))
                            material.SetEmissiveColor({ col[0], col[1], col[2] });
                        if (ecOvr)
                            RevertButton("R##emcol", [&]() { material.ClearEmissiveColor(); });
                        float intensity = material.GetEmissiveIntensity();
                        if (ImGui::SliderFloat("Intensity##emint", &intensity, 0.0f, 20.0f, "%.1f"))
                            material.SetEmissiveIntensity(intensity);
                        if (eiOvr)
                            RevertButton("R##emint", [&]() { material.ClearEmissiveIntensity(); });
                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("Sampler"))
                    {
                        ImGui::Indent();
                        const char* minFilterTypes[] = { "Nearest", "Linear", "Nearest Mipmap", "Linear Mipmap" };
                        const char* magFilterTypes[] = { "Nearest", "Linear" };
                        const char* wrapTypes[] = { "Repeat", "Mirror Repeat", "Clamp to edge", "Clamp to border", "Mirror clamp to edge" };
                        const char* anisoLevels[] = { "1x", "2x", "4x", "8x", "16x" };
                        SamplerDesc desc = material.GetSampler().GetDesc();
                        int minF = static_cast<uint8_t>(desc.minFilter);
                        int magF = static_cast<uint8_t>(desc.magFilter);
                        int wrap = static_cast<uint8_t>(desc.wrap);
                        int aniso = desc.anisotropy > 8.0f ? 4 : desc.anisotropy > 4.0f ? 3 : desc.anisotropy > 2.0f ? 2 : desc.anisotropy > 1.0f ? 1 : 0;
                        bool dirty = false;
                        if (ImGui::Combo("Min Filter", &minF, minFilterTypes, 4)) { desc.minFilter = static_cast<MinFilter>(minF); dirty = true; }
                        if (ImGui::Combo("Mag Filter", &magF, magFilterTypes, 2)) { desc.magFilter = static_cast<MagFilter>(magF); dirty = true; }
                        if (ImGui::Combo("Wrap", &wrap, wrapTypes, 5)) { desc.wrap = static_cast<Wrap>(wrap);     dirty = true; }
                        if (ImGui::Combo("Anisotropy", &aniso, anisoLevels, 5))
                        {
                            constexpr float values[] = { 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
                            desc.anisotropy = values[aniso];
                            dirty = true;
                        }
                        if (dirty) material.GetSampler().Init(desc);
                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("Billboard##billboardhdr"))
                    {
                        ImGui::Indent();

                        const char* billboardModes[] = { "Disabled", "Spherical", "Cylindrical"};
                        int currentBillboardMode = static_cast<int>(material.GetBillboardMode());
                        bool billboardOvr = inst.IsOverridden(MatField::BillboardMode);

                        if (ImGui::Combo("Billboard Mode##bmode", &currentBillboardMode, billboardModes, 3))
                            material.SetBillboardMode(static_cast<BillboardMode>(currentBillboardMode));

                        if (billboardOvr)
                            RevertButton("R##bmode", [&]() { material.ClearBillboardMode(); });

                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("UV##uvhdr"))
                    {
                        ImGui::Indent();

                        glm::vec2 uvOffset = material.GetUVOffset();
                        glm::vec2 uvScale = material.GetUVScale();
                        bool uvOffOvr = inst.IsOverridden(MatField::UVOffset);
                        bool uvSclOvr = inst.IsOverridden(MatField::UVScale);
                        if (ImGui::DragFloat2("UV Offset##uvoff", &uvOffset[0], 0.05f)) material.SetUVOffset(uvOffset);
                        if (uvOffOvr) RevertButton("R##uvoff", [&]() { material.ClearUVOffset(); });
                        if (ImGui::DragFloat2("UV Scale##uvscl", &uvScale[0], 0.05f)) material.SetUVScale(uvScale);
                        if (uvSclOvr) RevertButton("R##uvscl", [&]() { material.ClearUVScale(); });

                        ImGui::Unindent();
                    }
                }
            }
        } // !isPrefab

        // ---------- SceneTree (Parent) ----------
        if (registry.any_of<SceneTreeComponent>(selectedEntity))
        {
            auto& selectedNode = registry.get<SceneTreeComponent>(selectedEntity);

            if (ImGui::CollapsingHeader("Hierarchy"))
            {
                std::string parentName;
                if (selectedNode.parent == entt::null)
                    parentName = "None";
                else
                    parentName = registry.get<TagComponent>(selectedNode.parent).tag + "##" + std::to_string((uint32_t)selectedNode.parent);

                if (ImGui::BeginCombo("Parent", parentName.c_str()))
                {
                    if (ImGui::Selectable("None", selectedNode.parent == entt::null))
                        scene.RemoveParent(selectedEntity);

                    registry.view<SceneTreeComponent, TagComponent>().each([&](entt::entity entity, SceneTreeComponent&, TagComponent& tag)
                        {
                            if (entity != selectedEntity)
                            {
                                std::string displayName = tag.tag + "##" + std::to_string((uint32_t)entity);
                                if (ImGui::Selectable(displayName.c_str(), selectedNode.parent == entity))
                                    scene.SetParent(selectedEntity, entity);
                            }
                        });

                    ImGui::EndCombo();
                }
            }
        }

        // ---------- Script ----------
        if (registry.any_of<ScriptComponent>(selectedEntity))
        {
            bool open = ImGui::CollapsingHeader("Script");
            bool isRemoved = false;

            if (ImGui::BeginPopupContextItem("ScriptComponentContext"))
            {
                if (ImGui::MenuItem("Remove Component"))
                {
                    registry.remove<ScriptComponent>(selectedEntity);
                    isRemoved = true;
                }
                ImGui::EndPopup();
            }

            if (open && !isRemoved)
            {
                auto& scriptComp = registry.get<ScriptComponent>(selectedEntity);

                ImGui::Button("Drop Lua Script Here", ImVec2(200, 20));
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                    {
                        const char* path = (const char*)payload->Data;
                        std::string pathStr(path);
                        if (pathStr.ends_with(".lua"))
                        {
                            AssetUUID scriptUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
                            scriptComp.scriptAssetUUID = scriptUUID;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            }
        }

        // ---------- AudioEmitter ----------
        if (registry.any_of<AudioEmitterComponent>(selectedEntity))
        {
            bool open = ImGui::CollapsingHeader("AudioEmitter");
            bool isRemoved = false;

            if (ImGui::BeginPopupContextItem("AudioEmitterComponent"))
            {
                if (ImGui::MenuItem("Remove Component"))
                {
                    registry.remove<AudioEmitterComponent>(selectedEntity);
                    isRemoved = true;
                }
                ImGui::EndPopup();
            }

            if (open && !isRemoved)
            {
                auto& emitter = registry.get<AudioEmitterComponent>(selectedEntity);
                auto& audio = AudioManager::Get();

                ImGui::Button("Audio Clip", ImVec2(200, 20));
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                    {
                        std::string pathStr((const char*)payload->Data);
                        if (pathStr.ends_with(".wav") || pathStr.ends_with(".mp3"))
                        {
                            AssetUUID audioClipUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
                            emitter.audioClipUUID = audioClipUUID;

                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::Separator();

                // bus selection start

                auto& busses = audio.GetBusses();
                AudioBus* currentBus = audio.GetBus(emitter.busID);

                if (!currentBus)
                {
                    audio.SetBus(&emitter, 0); // the bus is no longer valid so assign it to the master bus
                    currentBus = audio.GetBus(emitter.busID);
                }

                std::string currentBusName = currentBus ? currentBus->GetName() : "Master";

                for (const auto& bus : busses)
                {
                    if (bus->GetID() == emitter.busID)
                    {
                        currentBusName = bus->GetName();
                        break;
                    }
                }

                if (ImGui::BeginCombo("Output Bus", currentBusName.c_str()))
                {
                    for (const auto& bus : busses)
                    {
                        bool isSelected = (emitter.busID == bus->GetID());
                        if (ImGui::Selectable(bus->GetName().c_str(), isSelected))
                        {
                            audio.SetBus(&emitter, bus->GetID());
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // bus selection end


                ImGui::Separator();

                if (ImGui::Button("Play"))   audio.Play(&emitter);
                ImGui::SameLine();
                if (ImGui::Button("Pause"))  audio.Pause(&emitter);
                ImGui::SameLine();
                if (ImGui::Button("Stop"))   audio.Stop(&emitter);


                ImGui::Separator();

                float volume = emitter.volume;
                if (ImGui::SliderFloat("Volume", &volume, 0.0f, 2.0f, "%.2f"))
                    audio.SetVolume(&emitter, volume);

                float pitch = emitter.pitch;
                if (ImGui::SliderFloat("Pitch", &pitch, 0.01f, 4.0f, "%.2f"))
                    audio.SetPitch(&emitter, pitch);

                bool loop = emitter.loop;
                if (ImGui::Checkbox("Loop", &loop))
                    audio.SetLoop(&emitter, loop);

                ImGui::Checkbox("Play On Create", &emitter.playOnCreate);

                ImGui::Separator();

                bool spatial = emitter.spatial;
                if (ImGui::Checkbox("Spatial 3D", &spatial))
                    audio.SetSpatial(&emitter, spatial);

                if (emitter.spatial)
                {
                    ImGui::Indent();

                    if (ImGui::CollapsingHeader("Attenuation"))
                    {
                        ImGui::Indent();

                        const char* attenuationModels[] = { "None", "Inverse", "Linear", "Exponential" };
                        int currentModel = static_cast<int>(emitter.attenuationModel);
                        if (ImGui::Combo("Model", &currentModel, attenuationModels, 4))
                        {
                            audio.SetAttenuationModel(&emitter, static_cast<AttenuationModel>(currentModel));
                        }

                        if (emitter.attenuationModel != AttenuationModel::None)
                        {
                            float minDist = emitter.minDistance;
                            if (ImGui::DragFloat("Min Distance", &minDist, 0.1f, 0.01f, emitter.maxDistance - 0.01f, "%.2f"))
                                audio.SetMinDistance(&emitter, minDist);

                            float maxDist = emitter.maxDistance;
                            if (ImGui::DragFloat("Max Distance", &maxDist, 0.5f, emitter.minDistance + 0.01f, 10000.0f, "%.2f"))
                                audio.SetMaxDistance(&emitter, maxDist);

                            if (emitter.attenuationModel == AttenuationModel::Inverse ||
                                emitter.attenuationModel == AttenuationModel::Exponential)
                            {
                                float rolloff = emitter.rolloff;
                                if (ImGui::SliderFloat("Rolloff", &rolloff, 0.0f, 10.0f, "%.2f"))
                                    audio.SetRolloff(&emitter, rolloff);
                            }
                        }

                        ImGui::Unindent();
                    }

                    if (ImGui::CollapsingHeader("Doppler"))
                    {
                        ImGui::Indent();

                        bool dopplerEnabled = emitter.doppler;
                        if (ImGui::Checkbox("Enabled##doppler", &dopplerEnabled))
                        {
                            emitter.doppler = dopplerEnabled;
                            audio.SetDopplerFactor(&emitter, dopplerEnabled ? emitter.dopplerFactor : 0.0f);
                        }

                        if (emitter.doppler)
                        {
                            float dopplerFactor = emitter.dopplerFactor;
                            if (ImGui::SliderFloat("Factor", &dopplerFactor, 0.0f, 5.0f, "%.2f"))
                                audio.SetDopplerFactor(&emitter, dopplerFactor);
                        }

                        ImGui::Unindent();
                    }

                    ImGui::Unindent();
                }
            }
        }
    }
    else
    {
        ImGui::Text("No entity selected.");
    }

    ImGui::End();
}