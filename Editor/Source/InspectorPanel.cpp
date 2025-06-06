#include "InspectorPanel.h"
#include "ImGui/imgui.h"

void InspectorPanel::Draw(Scene* scene, entt::entity& selectedEntity)
{

    entt::registry& registry = scene->GetRegistry();
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

    if (selectedEntity != entt::null) {
        ImGui::Text("Selected Entity: %u", (uint32_t)selectedEntity);

        if (registry.any_of<TagComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Name Tag", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& tag = registry.get<TagComponent>(selectedEntity);

                static char buffer[256]; // Geçici bir buffer
                std::strncpy(buffer, tag.tag.c_str(), sizeof(buffer)); // Başlangıçta std::string içeriğini kopyala

                if (ImGui::InputText("##hidden", buffer, sizeof(buffer))) {
                    if (buffer != "")
                        tag.tag = buffer; // Kullanıcı değişiklik yaptığında string'i güncelle
                }
            }
        }

        if (registry.any_of<UUIDComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("UUID")) {
                auto& uuid = registry.get<UUIDComponent>(selectedEntity);
                ImGui::Text("UUID: %s", std::to_string(uuid.uuid).c_str());
            }
        }

        if (registry.any_of<TransformComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& transform = registry.get<TransformComponent>(selectedEntity);


                glm::vec3 euler = transform.GetEulerRotation();
                bool isScaleRatioLocked = transform.IsScaleRatioLocked();

                glm::vec3 pos = transform.GetPosition();
                if (ImGui::DragFloat3("Position", &pos[0], 0.1f)) {
                    transform.SetPosition(pos);
                }

                if (ImGui::DragFloat3("Rotation", &euler[0], 0.1f, -360, 360)) {
                    transform.SetEulerRotation(euler);
                }

                ImGui::PushID("ScaleControls");
                glm::vec3 scale = transform.GetScale();
                if (ImGui::DragFloat3("Scale", &scale[0], 0.1f, 0.01f, 10.0f))
                {
                    transform.SetScale(scale);
                }
                if (ImGui::Checkbox("Lock Ratio", &isScaleRatioLocked))
                {
                    transform.SetLockScaleRatio(isScaleRatioLocked);
                }
                ImGui::PopID();




                if (ImGui::Button("Reset Transform")) {
                    transform.SetPosition({ 0.0f, 0.0f, 0.0f });
                    transform.SetEulerRotation({ 0.0f, 0.0f, 0.0f });
                    transform.SetScale({ 1.0f, 1.0f, 1.0f });
                }
            }
        }

        if (registry.any_of<MeshComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& meshComponent = registry.get<MeshComponent>(selectedEntity);
                ImGui::Text("Model Path: %s", meshComponent.modelPath.GetAbsoluteStr().c_str());
                ImGui::Text("Mesh Index: %s", std::to_string(meshComponent.meshIndex));
            }
        }

        if (registry.any_of<PointLightComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("PointLight", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& pointLightComponent = registry.get<PointLightComponent>(selectedEntity);
                glm::vec3 lightColor = pointLightComponent.color;
                float color[3] = { lightColor.r, lightColor.g, lightColor.b };
                if (ImGui::ColorPicker3("Light Color", color)) {
                    pointLightComponent.color = glm::vec3(color[0], color[1], color[2]);

                }

                if (ImGui::Checkbox("CastShadows", &pointLightComponent.castShadows)) {}

                if (ImGui::SliderFloat("Intensity", &pointLightComponent.intensity, 0.0f, 20.0f)) {}
                if (ImGui::SliderFloat("Radius", &pointLightComponent.radius, 0.1f, 50.0f)) {}
                if (ImGui::SliderFloat("Falloff", &pointLightComponent.falloff, 0.0f, 10.0f)) {}
                if (ImGui::SliderFloat("Shadow bias", &pointLightComponent.shadowBias, 0.0f, 0.1f)) {}
                if (ImGui::SliderFloat("Blur radius", &pointLightComponent.blurRadius, 0.0f, 0.1f)) {}
                if (ImGui::Checkbox("Reverse cull face", &pointLightComponent.reverseCullFace));


            }
        }

        if (registry.any_of<DirectionalLightComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("DirectionalLight", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& directionalLightComponent = registry.get<DirectionalLightComponent>(selectedEntity);
                glm::vec3 lightColor = directionalLightComponent.color;
                float color[3] = { lightColor.r, lightColor.g, lightColor.b };
                if (ImGui::ColorPicker3("Light Color", color)) {
                    directionalLightComponent.color = glm::vec3(color[0], color[1], color[2]);

                }

                if (ImGui::SliderFloat("Intensity", &directionalLightComponent.intensity, 0.0f, 10.0f)) {}
                if (ImGui::Checkbox("Cast Shadow", &directionalLightComponent.castShadows));
                if (ImGui::SliderFloat("Shadow bias", &directionalLightComponent.shadowBias, 0.0f, 0.1f)) {}
                if (ImGui::SliderFloat("Blur radius", &directionalLightComponent.blurRadius, 0.0f, 2.0f)) {}
                if (ImGui::Checkbox("Reverse cull face", &directionalLightComponent.reverseCullFace));

            }
        }

        if (registry.any_of<MaterialComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                MaterialComponent& materialComponent = registry.get<MaterialComponent>(selectedEntity);
                const ISerializable* materialSerializable = std::static_pointer_cast<const ISerializable>(materialComponent.material).get();

                ImGui::Text("Material Path: %s", materialComponent.GetMaterialPath().GetAbsoluteStr().c_str());

                glm::vec3 albedoColor = materialComponent.material->albedoColor;
                float color[3] = { albedoColor.r, albedoColor.g, albedoColor.b };

                if (ImGui::ColorPicker3("Albedo", color)) {
                    materialComponent.material->albedoColor = glm::vec3(color[0], color[1], color[2]);

                    //Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                }

                if (ImGui::SliderFloat("AO", &materialComponent.material->ao, 0.0f, 1.0f)) {
                    //Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                }

                if (ImGui::SliderFloat("Metallic", &materialComponent.material->metallic, 0.0f, 1.0f)) {
                    //Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                }

                if (ImGui::SliderFloat("Roughness", &materialComponent.material->roughness, 0.0f, 1.0f)) {
                    //Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                }

                if (ImGui::SliderFloat("Normal Scale", &materialComponent.material->normalScale, 0.0f, 10.0f)) {
                    //Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                }

            }
        }

        if (registry.any_of<SceneTreeComponent>(selectedEntity))
        {
            auto& selectedNode = registry.get<SceneTreeComponent>(selectedEntity);

            if (ImGui::CollapsingHeader("Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
            {
                std::string parentName;
                if (selectedNode.parent == entt::null)
                {
                    parentName = "None";
                }
                else
                {
                    parentName = registry.get<TagComponent>(selectedNode.parent).tag;
                    parentName += "##" + std::to_string((uint32_t)selectedNode.parent);
                }

                if (ImGui::BeginCombo("Parent", parentName.c_str()))
                {
                    if (ImGui::Selectable("None", selectedNode.parent == entt::null))
                    {
                        scene->RemoveParent(selectedEntity);
                    }

                    registry.view<SceneTreeComponent, TagComponent>().each([&](entt::entity entity, SceneTreeComponent&, TagComponent& tag)
                        {
                            if (entity != selectedEntity)
                            {
                                std::string displayName = tag.tag + "##" + std::to_string((uint32_t)entity);
                                if (ImGui::Selectable(displayName.c_str(), selectedNode.parent == entity))
                                {
                                    scene->SetParent(selectedEntity, entity);
                                }
                            }
                        });

                    ImGui::EndCombo();
                }

                auto& uuid = selectedNode.parentUUID;
                ImGui::Text("Parent UUID: %s", std::to_string(uuid).c_str());
                ImGui::Text("Children UUIDs:");
                for (const auto& childUUID : selectedNode.childrenUUIDs)
                {
                    ImGui::Text("- %s", std::to_string(childUUID).c_str());
                }
            }
        }


    }
    else {
        ImGui::Text("No entity selected.");
    }

    ImGui::End();
}