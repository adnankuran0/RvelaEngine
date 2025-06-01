#pragma once
#include "Scene/Component.h"
#include "Core/Utils/FileUtils.h"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

struct MeshComponent : public Component {
    Path modelPath;
    uint32_t meshIndex;

    MeshComponent() = delete;
    MeshComponent(const Path modelPath, uint16_t meshIndex) : modelPath(modelPath), meshIndex(meshIndex) {}

    std::string Serialize() const override {
        json j;
        j["modelPath"] = modelPath.GetVirtualStr();
        j["meshIndex"] = meshIndex;
        return j.dump(4);
    }

    void Deserialize(const std::string& str) override {
        json j = json::parse(str);
        std::string modelPathData = j["modelPath"];
        modelPath = TO_ABSOLUTE_PATH(modelPathData);
        meshIndex = j["meshIndex"];
    }
};