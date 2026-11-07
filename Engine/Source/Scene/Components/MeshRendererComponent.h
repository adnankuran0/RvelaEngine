#pragma once
#include "Renderer/GPUMesh.h"
#include <Renderer/AABB.h>
#include "Asset/Types/MeshAsset.h"
#include "Core/Ref.h"
#include <memory>
#include <nlohmann/json.hpp>

namespace rv {

using json = nlohmann::json;

struct alignas(16) MeshRendererComponent 
{
public:
    AABB localAABB;
    AABB worldAABB;
    unsigned int indexCount = 0;
    std::shared_ptr<GPUMesh> gpuMesh = nullptr;

    bool enableAutoLOD = true;
    float lodBias = 1.0f;
    uint32_t currentLOD = 0; // LOD choosen by camera

    MeshRendererComponent() = default;
    MeshRendererComponent(const MeshRendererComponent&) = default;
    MeshRendererComponent& operator=(const MeshRendererComponent&) = default;
    MeshRendererComponent(MeshRendererComponent&&) = default;
    MeshRendererComponent& operator=(MeshRendererComponent&&) = default;

    MeshRendererComponent(Ref<MeshAsset> mesh)
    {
        if (mesh)
            RecreateFromMesh(mesh);
    }

    void RecreateFromMesh(Ref<MeshAsset> mesh);

    inline void Destroy() noexcept
    {
        gpuMesh = nullptr;
        indexCount = 0;
        currentLOD = 0;
    }

    inline void Bind() const noexcept
    {
        if (gpuMesh && gpuMesh->VAO.getID() != 0)
            gpuMesh->Bind();
    }

    inline bool IsCastShadow() const noexcept { return castShadow; }
    inline void SetCastShadow(bool isCastsShadow) noexcept { castShadow = isCastsShadow; }

    inline bool IsAutoLODEnabled() const noexcept { return enableAutoLOD; }
    inline void SetAutoLODEnabled(bool enable) noexcept { enableAutoLOD = enable; }

    inline float GetLODBias() const noexcept { return lodBias; }
    inline void SetLODBias(float bias) noexcept { lodBias = bias; }

    inline uint32_t GetCurrentLOD() const noexcept { return currentLOD; }

    json Serialize() const;
    void Deserialize(const json& j);

private:
    bool castShadow = true;
};

}
