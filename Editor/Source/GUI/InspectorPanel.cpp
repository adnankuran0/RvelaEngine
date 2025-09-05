#include "InspectorPanel.h"
#include "ImGui/imgui.h"

void InspectorPanel::Draw(Scene* scene, entt::entity& selectedEntity)
{

    entt::registry& registry = scene->GetRegistry();
    ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

    
    if (ImGui::BeginTabBar("PropertiesTabs"))
    {
    if (ImGui::BeginTabItem("Inspector"))
    {
        

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

        if (registry.any_of<TransformComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Transform"))
            {
                auto& transform = registry.get<TransformComponent>(selectedEntity);


                glm::vec3 euler = transform.GetEulerRotation();
                bool isScaleRatioLocked = transform.IsScaleRatioLocked();

                glm::vec3 pos = transform.GetPosition();
                if (ImGui::DragFloat3("Position", &pos[0], 0.1f)) {
                    transform.SetPosition(pos);
                    transform.SetDirty();
                }

                if (ImGui::DragFloat3("Rotation", &euler[0], 0.1f, -360, 360)) {
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




                if (ImGui::Button("Reset Transform")) {
                    transform.SetPosition({ 0.0f, 0.0f, 0.0f });
                    transform.SetEulerRotation({ 0.0f, 0.0f, 0.0f });
                    transform.SetScale({ 1.0f, 1.0f, 1.0f });
                }
            }
        }

        if (registry.any_of<MeshComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Mesh"))
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
                        if (pathStr.ends_with(".rmesh"))
                        {
                            std::ifstream inFile(pathStr, std::ios::binary);
                            if (!inFile.is_open()) {
                                LOG_ERROR("file not opened");
                                return;
                            }
                            AssetHeader header = AssetLoader::ReadHeader(inFile, MAGIC_MESH);
                            std::unique_ptr<MeshMeta> meta = AssetLoader::ReadMeta<MeshMeta>(inFile, header);
                            mesh.SetMesh(meta->uuid);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::Unindent();
            }
        }

        if (registry.any_of<MeshRendererComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Mesh Renderer"))
            {
                ImGui::Indent();
                MeshRendererComponent& meshRenderer = registry.get<MeshRendererComponent>(selectedEntity);
                bool isCastShadow = meshRenderer.IsCastShadow();
                if (ImGui::Checkbox("Cast Shadow",&isCastShadow))
                {
                    meshRenderer.SetCastShadow(isCastShadow);
                }

                bool isDoubleSided = meshRenderer.IsDoubleSided();
                if (ImGui::Checkbox("Double Sided", &isDoubleSided))
                {
                    meshRenderer.SetDoubleSided(isDoubleSided);
                }

                ImGui::Unindent();
            }
        }

        if (registry.any_of<PointLightComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("PointLight"))
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
            if (ImGui::CollapsingHeader("DirectionalLight"))
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
            if (ImGui::CollapsingHeader("Material"))
            {
                ImGui::Indent();
                MaterialComponent& material = registry.get<MaterialComponent>(selectedEntity);

                ImGui::Button("Material Slot", ImVec2(200, 20));
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                    {
                        const char* path = (const char*)payload->Data;
                        std::string pathStr(path);
                        if (pathStr.ends_with(".rmat"))
                        {
                            std::ifstream inFile(pathStr, std::ios::binary);
                            if (!inFile.is_open()) {
                                LOG_ERROR("file not opened");
                                return;
                            }
                            AssetHeader header = AssetLoader::ReadHeader(inFile, MAGIC_MATERIAL);
                            std::unique_ptr<MaterialMeta> meta = AssetLoader::ReadMeta<MaterialMeta>(inFile, header);
                            material.SetMaterial(meta->uuid);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }




                if (ImGui::CollapsingHeader("Albedo"))
                {
                    ImGui::Indent();
                    glm::vec3 albedoColor = material.GetAlbedoColor();
                    float color[3] = { albedoColor.r, albedoColor.g, albedoColor.b };

                    ImGui::Button("Albedo Slot", ImVec2(200, 20));
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                        {
                            const char* path = (const char*)payload->Data;
                            std::string pathStr(path);
                            if (pathStr.ends_with(".rtex"))
                            {
                                std::ifstream inFile(pathStr, std::ios::binary);
                                if (!inFile.is_open()) {
                                    LOG_ERROR("file not opened");
                                    return;
                                }
                                AssetHeader header = AssetLoader::ReadHeader(inFile, MAGIC_TEXTURE);
                                std::unique_ptr<TextureMeta> meta = AssetLoader::ReadMeta<TextureMeta>(inFile, header);
                                material.SetAlbedoTexture(meta->uuid);
                                material.SetUseAlbedoMap(true);
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("X##alb", ImVec2(20, 20)))
                    {
                        material.SetUseAlbedoMap(!material.IsUsingAlbedoMap());
                    }


                    if (ImGui::ColorEdit3("Albedo", color))
                    {
                        material.SetAlbedoColor(glm::vec3(color[0], color[1], color[2]));
                        //Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                    }
                    ImGui::Unindent();
                }
                if (ImGui::CollapsingHeader("Normal"))
                {
                    ImGui::Indent();
                    ImGui::Button("Normal Slot", ImVec2(200, 20));
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                        {
                            const char* path = (const char*)payload->Data;
                            std::string pathStr(path);
                            if (pathStr.ends_with(".rtex"))
                            {
                                std::ifstream inFile(pathStr, std::ios::binary);
                                if (!inFile.is_open()) {
                                    LOG_ERROR("file not opened");
                                    return;
                                }
                                AssetHeader header = AssetLoader::ReadHeader(inFile, MAGIC_TEXTURE);
                                std::unique_ptr<TextureMeta> meta = AssetLoader::ReadMeta<TextureMeta>(inFile, header);
                                material.SetNormalTexture(meta->uuid);
                                material.SetUseNormalMap(true);
                            }
                        }
                        ImGui::EndDragDropTarget();


                    }
                    ImGui::SameLine();
                    if (ImGui::Button("X##nrm", ImVec2(20, 20)))
                    {
                        material.SetUseNormalMap(!material.IsUsingNormalMap());
                    }
                    float normalScale = material.GetNormalScale();
                    if (ImGui::SliderFloat("Normal Scale", &normalScale, -10.0f, 10.0f))
                    {
                        material.SetNormalScale(normalScale);
                    }
                    ImGui::Unindent();
                }

                if (ImGui::CollapsingHeader("Roughness##31"))
                {
                    ImGui::Indent();
                    ImGui::Button("Rougness Slot", ImVec2(200, 20));
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                        {
                            const char* path = (const char*)payload->Data;
                            std::string pathStr(path);
                            if (pathStr.ends_with(".rtex"))
                            {
                                std::ifstream inFile(pathStr, std::ios::binary);
                                if (!inFile.is_open()) {
                                    LOG_ERROR("file not opened");
                                    return;
                                }
                                AssetHeader header = AssetLoader::ReadHeader(inFile, MAGIC_TEXTURE);
                                std::unique_ptr<TextureMeta> meta = AssetLoader::ReadMeta<TextureMeta>(inFile, header);
                                material.SetRoughnessTexture(meta->uuid);
                                material.SetUseRoughnessMap(true);
                            }
                        }
                        ImGui::EndDragDropTarget();


                    }
                    ImGui::SameLine();
                    if (ImGui::Button("X##rgh", ImVec2(20, 20)))
                    {
                        material.SetUseRoughnessMap(!material.IsUsingRoughnessMap());
                    }
                    float roughness = material.GetRoughness();
                    if (ImGui::SliderFloat("Roughness##32", &roughness, 0.0f, 1.0f))
                    {
                        material.SetRoughness(roughness);
                    }
                    ImGui::Unindent();
                    ;
                }

                if (ImGui::CollapsingHeader("Metallic##33"))
                {
                    ImGui::Indent();
                    ImGui::Button("Metallic Slot", ImVec2(200, 20));
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                        {
                            const char* path = (const char*)payload->Data;
                            std::string pathStr(path);
                            if (pathStr.ends_with(".png") || pathStr.ends_with(".jpeg") || pathStr.ends_with(".jpg") || pathStr.ends_with(".tga"))
                            {
                                // .rmaterial dosyası bırakıldı, işle
                                // Örneğin: materialComponent->LoadFromFile(pathStr);
                                //materialComponent.material->metallicMapPath = ABS_PATH(pathStr);
                            }
                        }
                        ImGui::EndDragDropTarget();


                    }
                    ImGui::SameLine();
                    if (ImGui::Button("X##mtl", ImVec2(20, 20)))
                    {
                        //materialComponent.material->metallicMapPath = ABS_PATH("");
                    }

                    float metallic = material.GetMetallic();
                    if (ImGui::SliderFloat("Metallic##34", &metallic, 0.0f, 1.0f))
                    {
                        material.SetMetallic(metallic);
                        //Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                    }
                    ImGui::Unindent();
                }

                if (ImGui::CollapsingHeader("AO##35"))
                {
                    ImGui::Indent();
                    ImGui::Button("AO Slot", ImVec2(200, 20));
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                        {
                            const char* path = (const char*)payload->Data;
                            std::string pathStr(path);
                            if (pathStr.ends_with(".png") || pathStr.ends_with(".jpeg") || pathStr.ends_with(".jpg") || pathStr.ends_with(".tga"))
                            {
                                // .rmaterial dosyası bırakıldı, işle
                                // Örneğin: materialComponent->LoadFromFile(pathStr);
                                //
                                //materialComponent.material->aoMapPath = ABS_PATH(pathStr);
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("X##ao", ImVec2(20, 20)))
                    {
                        //materialComponent.material->aoMapPath = ABS_PATH("");
                    }
                    float ao = material.GetAO();
                    if (ImGui::SliderFloat("AO##36", &ao, 0.0f, 1.0f))
                    {
                        material.SetAO(ao);
                        //Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                    }
                    ImGui::Unindent();
                }


                glm::vec2 uvOffset = material.GetUVOffset();
                glm::vec2 uvScale = material.GetUVScale();

                if (ImGui::DragFloat2("UV Offset", &uvOffset[0], 0.05f))
                {
                    material.SetUVOffset(uvOffset);
                }
                if (ImGui::DragFloat2("UV Scale", &uvScale[0], 0.05f)) {
                    material.SetUVScale(uvScale);
                }
                ImGui::Unindent();
            }
        }

        if (registry.any_of<SceneTreeComponent>(selectedEntity))
        {
            auto& selectedNode = registry.get<SceneTreeComponent>(selectedEntity);

            if (ImGui::CollapsingHeader("Hierarchy"))
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
            }
        }


    }
    else {
        ImGui::Text("No entity selected.");
    }
    ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Environment"))
    {
        ImGui::Text("Environment settings go here.");
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    }
    ImGui::End();
}
