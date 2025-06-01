#pragma once
#include "Scene/Component.h"
#include "Core/Utils/FileUtils.h"
#include "Core/Utils/MaterialManager.h"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

class MaterialComponent : public Component {
public:

    MaterialComponent() = delete;

    MaterialComponent(const Path materialPath)
        : materialPath(materialPath)
    {
        material = MaterialManager::LoadOrGetMaterial(materialPath);
    }

    MaterialComponent(const MaterialComponent& other)
        : materialPath(other.materialPath)
    {
    }

    MaterialComponent(MaterialComponent&& other) noexcept
        : materialPath(std::move(other.materialPath))
    {
    }

    ~MaterialComponent()
    {
    }

    Path& GetMaterialPath()
    {
        return materialPath;
    }

    void SetMaterialPath(const Path materialPath)
    {
        if (this->materialPath == materialPath) return;
        MaterialManager::UnloadMaterial(materialPath);
        MaterialManager::ClearExpiredMaterials();
        this->materialPath = materialPath;
        material = MaterialManager::LoadOrGetMaterial(materialPath);
    }



    std::string Serialize() const override
    {
        json j;
        j["materialPath"] = materialPath.GetVirtualStr();

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        std::string materialPathData = j["materialPath"];
        materialPath = TO_ABSOLUTE_PATH(materialPathData);

    }

    std::shared_ptr<Material> material;


private:
    Path materialPath;
};
