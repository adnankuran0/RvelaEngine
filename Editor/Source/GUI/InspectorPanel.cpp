#include "InspectorPanel.h"
#include "ImGui/imgui.h"
#include "Core/Engine.h"
#include "Rendering/RenderLayer.h"

using namespace rv;

void InspectorPanel::Draw(Engine* engine, entt::entity& selectedEntity)
{
    Scene& scene = engine->GetActiveScene();
    entt::registry& registry = scene.GetRegistry();
    ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

    
    if (ImGui::BeginTabBar("PropertiesTabs"))
    {
    if (ImGui::BeginTabItem("Inspector"))
    {
        

    if (selectedEntity != entt::null) {
        ImGui::Text("Selected Entity: %u", (uint32_t)selectedEntity);

        bool isPrefab = registry.any_of<PrefabComponent>(selectedEntity);

        
        if (registry.any_of<TagComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Name Tag", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& tag = registry.get<TagComponent>(selectedEntity);

                static char buffer[256]; 
                std::strncpy(buffer, tag.tag.c_str(), sizeof(buffer)); 

                if (ImGui::InputText("##hidden", buffer, sizeof(buffer))) {
                    if (buffer != "")
                        tag.tag = buffer; 
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

        // show only transform component if entity is a prefab instance 
        if(!isPrefab)
        {
        if (registry.any_of<MeshComponent>(selectedEntity))
        {
            bool open = ImGui::CollapsingHeader("Mesh");

            if (ImGui::BeginPopupContextItem("MeshComponentContext")) {
                if (ImGui::MenuItem("Remove Component")) {
                    registry.remove<MeshComponent>(selectedEntity);
                }
                ImGui::EndPopup();
            }

            if (open)
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

        if (registry.any_of<CameraComponent>(selectedEntity))
        {
            bool open = ImGui::CollapsingHeader("Camera");

            if (ImGui::BeginPopupContextItem("CameraComponentContext")) {
                if (ImGui::MenuItem("Remove Component")) {
                    registry.remove<CameraComponent>(selectedEntity);
                }
                ImGui::EndPopup();
            }

            if (open)
            {
                auto& camComp = registry.get<CameraComponent>(selectedEntity);
                auto& camera = camComp.camera;

                ImGui::Checkbox("Is Active", &camComp.isActive);

                const char* projectionTypes[] = { "Perspective", "Orthographic" };
                int current = static_cast<int>(camera.ProjectionType);

                if (ImGui::Combo("Projection", &current, projectionTypes, 2))
                {
                    camera.ProjectionType = static_cast<Camera::Projection>(current);
                }

                if (camera.ProjectionType == Camera::Projection::Perspective)
                {
                    ImGui::SliderFloat("Fov", &camera.FOV, 1.0f, 120.0f);
                }
                else
                {
                    ImGui::SliderFloat("Ortho Size", &camera.OrthoSize, 0.1f, 100.0f);
                }

                ImGui::SliderFloat("Near clip", &camera.NearClip, 0.001f, 10.0f);
                ImGui::SliderFloat("Far clip", &camera.FarClip, 1.0f, 5000.0f);
            }
        }

        if (registry.any_of<MeshRendererComponent>(selectedEntity))
        {
            bool open = ImGui::CollapsingHeader("Mesh Renderer");

            if (ImGui::BeginPopupContextItem("MeshRendererComponentContext")) {
                if (ImGui::MenuItem("Remove Component")) {
                    registry.remove<MeshRendererComponent>(selectedEntity);
                }
                ImGui::EndPopup();
            }
            if (open)
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
            bool open = ImGui::CollapsingHeader("Point Light");

            if (ImGui::BeginPopupContextItem("PointLightComponentContext")) {
                if (ImGui::MenuItem("Remove Component")) {
                    registry.remove<PointLightComponent>(selectedEntity);
                }
                ImGui::EndPopup();
            }

            if (open)
            {
                auto& pointLightComponent = registry.get<PointLightComponent>(selectedEntity);
                glm::vec3 lightColor = pointLightComponent.color;
                float color[3] = { lightColor.r, lightColor.g, lightColor.b };
                if (ImGui::ColorPicker3("Light Color", color)) {
                    pointLightComponent.color = glm::vec3(color[0], color[1], color[2]);

                }

                ImGui::Checkbox("CastShadows", &pointLightComponent.castShadows);

                ImGui::SliderFloat("Intensity", &pointLightComponent.intensity, 0.0f, 20.0f);
                ImGui::SliderFloat("Radius", &pointLightComponent.radius, 0.1f, 50.0f);
                ImGui::SliderFloat("Falloff", &pointLightComponent.falloff, 0.0f, 10.0f);
                ImGui::SliderFloat("Shadow bias", &pointLightComponent.shadowBias, 0.0f, 0.1f);
                ImGui::SliderFloat("Blur radius", &pointLightComponent.blurRadius, 0.0f, 0.1f);
                ImGui::Checkbox("Reverse cull face", &pointLightComponent.reverseCullFace);


            }
        }

        if (registry.any_of<DirectionalLightComponent>(selectedEntity))
        {
            bool open = ImGui::CollapsingHeader("Directional Light");

            if (ImGui::BeginPopupContextItem("DirectionalLightComponentContext")) {
                if (ImGui::MenuItem("Remove Component")) {
                    registry.remove<DirectionalLightComponent>(selectedEntity);
                }
                ImGui::EndPopup();
            }

            if (open)
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

        if (registry.any_of<RigidbodyComponent>(selectedEntity))
        {
            bool open = ImGui::CollapsingHeader("Rigidbody");

            if (ImGui::BeginPopupContextItem("RigidbodyComponentContext")) {
                if (ImGui::MenuItem("Remove Component")) {
                    registry.remove<RigidbodyComponent>(selectedEntity);
                }

                ImGui::EndPopup();
            }

            if (open)
            {
                auto& rb = registry.get<RigidbodyComponent>(selectedEntity);

                const char* bodyTypes[] = { "Static", "Kinematic","Dynamic" };
                int current = static_cast<int>(rb.bodyType);

                if (ImGui::Combo("Body type", &current, bodyTypes, 3))
                {
                    rb.bodyType= static_cast<Physics::BodyType>(current);
                }
                ImGui::SliderFloat("Mass", &rb.mass, 0.001f, 1000.0f);
                ImGui::SliderFloat("Friction", &rb.friction, 0.0f, 1.0f);
                ImGui::SliderFloat("Restitution", &rb.restitution, 0.0f, 1.0f);
                ImGui::SliderFloat("Linear Damping", &rb.linearDamping, 0.0f, 1.0f);
                ImGui::SliderFloat("Angular Damping", &rb.angularDamping, 0.0f, 1.0f);

                ImGui::Checkbox("Lock Rotation X", &rb.lockRotationX);
                ImGui::Checkbox("Lock Rotation Y", &rb.lockRotationY);
                ImGui::Checkbox("Lock Rotation Z", &rb.lockRotationZ);
                ImGui::Checkbox("Lock Translation X", &rb.lockTranslationX);
                ImGui::Checkbox("Lock Translation Y", &rb.lockTranslationY);
                ImGui::Checkbox("Lock Translation Z", &rb.lockTranslationZ);
               
                ImGui::SliderFloat("Gravity factor", &rb.gravityFactor, 0.0f, 10.0f);

                ImGui::Checkbox("Is sensor", &rb.isSensor);
                ImGui::Checkbox("Use CCD", &rb.useCCD);


            }
        }

        if (registry.any_of<MaterialComponent>(selectedEntity)) {
            bool open = ImGui::CollapsingHeader("Material");

            if (ImGui::BeginPopupContextItem("MaterialComponentContext")) {
                if (ImGui::MenuItem("Remove Component")) {
                    registry.remove<MaterialComponent>(selectedEntity);
                }
                ImGui::EndPopup();
            }

            if (open)
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
                            if (pathStr.ends_with(".rtex"))
                            {
                                std::ifstream inFile(pathStr, std::ios::binary);
                                if (!inFile.is_open()) {
                                    LOG_ERROR("file not opened");
                                    return;
                                }
                                AssetHeader header = AssetLoader::ReadHeader(inFile, MAGIC_TEXTURE);
                                std::unique_ptr<TextureMeta> meta = AssetLoader::ReadMeta<TextureMeta>(inFile, header);
                                material.SetMetallicTexture(meta->uuid);
                                material.SetUseMetallicMap(true);
                            }
                        }
                        ImGui::EndDragDropTarget();


                    }
                    ImGui::SameLine();
                    if (ImGui::Button("X##mtl", ImVec2(20, 20)))
                    {
                        material.SetUseMetallicMap(!material.IsUsingMetallicMap());
                    }

                    float metallic = material.GetMetallic();
                    if (ImGui::SliderFloat("Metallic##34", &metallic, 0.0f, 1.0f))
                    {
                        material.SetMetallic(metallic);
                    }
                    float specular = material.GetSpecular();
                    if (ImGui::SliderFloat("Specular##34", &specular, 0.0f, 1.0f))
                    {
                        material.SetSpecular(specular);
                    }
                    ImGui::Unindent();
                }
                if (ImGui::CollapsingHeader("Height"))
                {
                    ImGui::Indent();
                    ImGui::Button("Height Slot", ImVec2(200, 20));
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
                                material.SetHeightTexture(meta->uuid);
                                material.SetUseHeightMap(true);
                            }
                        }
                        ImGui::EndDragDropTarget();


                    }
                    ImGui::SameLine();
                    if (ImGui::Button("X##hgt", ImVec2(20, 20)))
                    {
                        material.SetUseHeightMap(!material.IsUsingHeightMap());
                    }
                    float heightScale = material.GetHeightScale();
                    if (ImGui::SliderFloat("Height Scale", &heightScale, 0.0f, 1.0f))
                    {
                        material.SetHeightScale(heightScale);
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
                            if (pathStr.ends_with(".rtex"))
                            {
                                std::ifstream inFile(pathStr, std::ios::binary);
                                if (!inFile.is_open()) {
                                    LOG_ERROR("file not opened");
                                    return;
                                }
                                AssetHeader header = AssetLoader::ReadHeader(inFile, MAGIC_TEXTURE);
                                std::unique_ptr<TextureMeta> meta = AssetLoader::ReadMeta<TextureMeta>(inFile, header);
                                material.SetAOTexture(meta->uuid);
                                material.SetUseAOMap(true);
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("X##ao", ImVec2(20, 20)))
                    {
                        material.SetUseAOMap(!material.IsUsingAOMap());
                    }
                    float ao = material.GetAO();
                    if (ImGui::SliderFloat("AO##36", &ao, 0.0f, 1.0f))
                    {
                        material.SetAO(ao);
                        //Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                    }
                    ImGui::Unindent();
                }

                if (ImGui::CollapsingHeader("Emmisive"))
                {
                    ImGui::Indent();
                    glm::vec3 emmisiveColor = material.GetEmmisiveColor();
                    float intensity = material.GetEmmisiveIntensity();
                    float color[3] = { emmisiveColor.r, emmisiveColor.g, emmisiveColor.b };

                    if (ImGui::ColorEdit3("Emmisive Color", color))
                    {
                        material.SetEmmisiveColor(glm::vec3(color[0], color[1], color[2]));
                        //Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                    }
                    if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 20.0f, "%.1f"))
                    {
                        material.SetEmmisiveIntensity(intensity);
                    }

                    ImGui::Unindent();
                }


                if (ImGui::CollapsingHeader("Sampler"))
                {
                    ImGui::Indent();
                    const char* minFilterTypes[] = { "Nearest", "Linear", "Nearest Mipmap", "Linear Mipmap"};
                    const char* magFilterTypes[] = { "Nearest", "Linear" };
                    const char* wrapTypes[] = { "Repeat", "Mirror Repeat", "Clamp to edge","Clamp to border", "Mirror clamp to edge"};
                    const char* anisoLevels[] = { "1x", "2x", "4x", "8x", "16x" };

                    SamplerDesc desc = material.GetSampler().GetDesc();
                    int currentMinFilter = static_cast<uint8_t>(desc.minFilter);
                    int currentMagFilter = static_cast<uint8_t>(desc.magFilter);
                    int currentWrap = static_cast<uint8_t>(desc.wrap);
                    int currentAniso = static_cast<uint8_t>(desc.anisotropy);

                    int currentAnisoIdx = 0;
                    if (desc.anisotropy > 8.0f) currentAnisoIdx = 4;      // 16x
                    else if (desc.anisotropy > 4.0f) currentAnisoIdx = 3; // 8x
                    else if (desc.anisotropy > 2.0f) currentAnisoIdx = 2; // 4x
                    else if (desc.anisotropy > 1.0f) currentAnisoIdx = 1; // 2x

                    bool isDirty = false;

                    if (ImGui::Combo("Min Filter", &currentMinFilter, minFilterTypes, 4))
                    {
                        desc.minFilter = static_cast<MinFilter>(currentMinFilter);
                        isDirty = true;
                    }
                    if (ImGui::Combo("Mag Filter", &currentMagFilter, magFilterTypes, 2))
                    {
                        desc.magFilter = static_cast<MagFilter>(currentMagFilter);
                        isDirty = true;

                    }
                    if (ImGui::Combo("Wrap", &currentWrap, wrapTypes, 5))
                    {
                        desc.wrap = static_cast<Wrap>(currentWrap);
                        isDirty = true;
                    }
                    if (ImGui::Combo("Anisotropy", &currentAnisoIdx, anisoLevels, 5)) {
                        float values[] = { 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
                        desc.anisotropy = values[currentAnisoIdx];
                        isDirty = true;
                    }
                    if (isDirty)
                        material.GetSampler().Init(desc);
                  
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
                        scene.RemoveParent(selectedEntity);
                    }

                    registry.view<SceneTreeComponent, TagComponent>().each([&](entt::entity entity, SceneTreeComponent&, TagComponent& tag)
                        {
                            if (entity != selectedEntity)
                            {
                                std::string displayName = tag.tag + "##" + std::to_string((uint32_t)entity);
                                if (ImGui::Selectable(displayName.c_str(), selectedNode.parent == entity))
                                {
                                    scene.SetParent(selectedEntity, entity);
                                }
                            }
                        });

                    ImGui::EndCombo();
                }
            }
        }

        if (registry.any_of<ScriptComponent>(selectedEntity))
        {
            bool open = ImGui::CollapsingHeader("Script");

            if (ImGui::BeginPopupContextItem("ScriptComponentContext")) {
                if (ImGui::MenuItem("Remove Component")) {
                    registry.remove<ScriptComponent>(selectedEntity);
                }
                ImGui::EndPopup();
            }

            if (open)
            {
                auto& scriptComp = registry.get<ScriptComponent>(selectedEntity);

                // Lua script path text box
                char buffer[512];
                std::strncpy(buffer, scriptComp.luaFile.c_str(), sizeof(buffer));
                if (ImGui::InputText("Path", buffer, sizeof(buffer)))
                {
                    scriptComp.luaFile = buffer;
                }

                // Drag & Drop target
                ImGui::Button("Drop Lua Script Here", ImVec2(200, 20));
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                    {
                        const char* path = (const char*)payload->Data;
                        std::string pathStr(path);
                        if (pathStr.ends_with(".lua"))
                        {
                            scriptComp.luaFile = pathStr;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            }
        }

        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            if (!registry.any_of<ScriptComponent>(selectedEntity))
            {
                if (ImGui::MenuItem("Script"))
                {
                    ScriptComponent& comp = registry.emplace<ScriptComponent>(selectedEntity);
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!registry.any_of<MaterialComponent>(selectedEntity))
            {
                if (ImGui::MenuItem("Material"))
                {
                    AssetUUID defaultMaterialId = AssetUUID::FromString("ee3dde12-6263-4f11-bb1d-812b3e196ab7");
                    MaterialComponent& comp = registry.emplace<MaterialComponent>(selectedEntity, defaultMaterialId);
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!registry.any_of<MeshComponent>(selectedEntity))
            {
                if (ImGui::MenuItem("Mesh"))
                {
                    MeshComponent& comp = registry.emplace<MeshComponent>(selectedEntity);
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!registry.any_of<MeshRendererComponent>(selectedEntity))
            {
                if (ImGui::MenuItem("Mesh renderer"))
                {
                    MeshRendererComponent& comp = registry.emplace<MeshRendererComponent>(selectedEntity);
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!registry.any_of<PointLightComponent>(selectedEntity))
            {
                if (ImGui::MenuItem("Point light"))
                {
                    PointLightComponent& comp = registry.emplace<PointLightComponent>(selectedEntity);
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!registry.any_of<DirectionalLightComponent>(selectedEntity))
            {
                if (ImGui::MenuItem("Directional light"))
                {
                    DirectionalLightComponent& comp = registry.emplace<DirectionalLightComponent>(selectedEntity);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (!registry.any_of<RigidbodyComponent>(selectedEntity))
            {
                if (ImGui::MenuItem("Rigidbody"))
                {
                    RigidbodyComponent& comp = registry.emplace<RigidbodyComponent>(selectedEntity);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }


    }
    else {
        ImGui::Text("No entity selected.");
    }
    ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Environment"))
    {
        auto& env = engine->GetActiveScene().GetEnvironment();

        if (ImGui::CollapsingHeader("Lighting"))
        {
            ImGui::Button("Skybox HDR", ImVec2(200, 20));
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    const char* path = (const char*)payload->Data;
                    std::string pathStr(path);

                    if (pathStr.ends_with(".hdr"))
                    {
                        Path path = Path::FromAbsolute(pathStr);
                        env.GetSkybox().InitHDR(path);
                    }
                }
                ImGui::EndDragDropTarget();
            }
            bool* useIBL = &env.Lighting_IBL;
            ImGui::Checkbox("Use IBL", useIBL);
            
            if (*useIBL)
            {
                ImGui::SliderFloat("IBL Intensity", &env.Lighting_IBLIntensity, 0.0f, 5.0f, "%.1f");
            }
            else
            {
                ImGui::ColorEdit3("Ambient Color", glm::value_ptr(env.Lighting_AmbientColor));
                ImGui::SliderFloat("Ambient Intensity", &env.Lighting_AmbientIntensity, 0.0f, 5.0f, "%.1f");
            }
            
            
            
            

        }

        if (ImGui::CollapsingHeader("SSAO"))
        {
            ImGui::Checkbox("Use SSAO", &env.SSAO);
            ImGui::SliderFloat("Intensity", &env.SSAO_Intensity, 0.0f, 10.0f);
            ImGui::SliderFloat("Radius", &env.SSAO_Radius, 0.0f, 10.0f);
            ImGui::SliderFloat("Bias", &env.SSAO_Bias, 0.0f, 0.1f);

        }

        ImGui::Checkbox("SSR", &env.SSR);

        if (ImGui::CollapsingHeader("Bloom"))
        {
            ImGui::Checkbox("Use Bloom", &env.Bloom);
            ImGui::SliderFloat("Intensity", &env.Bloom_Intensity,0.0f,1.0f);
            ImGui::SliderFloat("Treshold", &env.Bloom_Treshold,0.0f,1.0f);
            ImGui::SliderFloat("Knee", &env.Bloom_Knee,0.0f,1.0f);
        }
       
        if (ImGui::CollapsingHeader("Post Process"))
        {
            ImGui::SliderFloat("Exposure", &env.PostProcess_Exposure, 0.0f, 10.0f);
            ImGui::SliderFloat("Chromatic Aberration", &env.PostProcess_ChromaticStrength, 0.001f, 0.1f);
            ImGui::SliderFloat("Vignette Intensity", &env.PostProcess_VignetteIntensity, 0.0f, 1.0f);
            ImGui::SliderFloat("Vignette Smoothness", &env.PostProcess_VignetteSmoothness, 0.1f, 1.0f);

        }

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    }
    ImGui::End();
}

