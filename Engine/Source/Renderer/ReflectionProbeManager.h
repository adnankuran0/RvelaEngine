#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "GLAD/gl.h"
#include "Scene/Components/ReflectionProbeComponent.h"
#include <entt/entt.h>

namespace rv {

class Scene;

struct GPUReflectionProbeData
{
    glm::vec4 position;     // xyz: center, w: intensity
    glm::vec4 boxExtents;   // xyz: extents, w: blendDistance
    glm::vec4 originOffset; // xyz: originOffset, w: priority
    glm::vec4 params;       // x: boxProjection, y: interior, z: cubeIndex
};

constexpr uint32_t MAX_GPU_REFLECTION_PROBES = 8;

struct alignas(16) GPUReflectionProbeUBO
{
    GPUReflectionProbeData probes[MAX_GPU_REFLECTION_PROBES];
    int probeCount = 0;
    int pad0 = 0;
    int pad1 = 0;
    int pad2 = 0;
};

class ReflectionProbeManager
{
public:
    static ReflectionProbeManager& Get()
    {
        static ReflectionProbeManager instance;
        return instance;
    }

    void Init();
    void Shutdown();

    void BakeProbe(Scene& scene, entt::entity entity, ReflectionProbeComponent& probe);
    void BakeAllDirtyProbes(Scene& scene);
    void PrepareProbes(Scene& scene, GPUReflectionProbeUBO& outUbo);

    [[nodiscard]] GLuint GetCubeArrayTexture() const noexcept { return m_CubeArrayTexture; }

private:
    ReflectionProbeManager() = default;
    ~ReflectionProbeManager() = default;

    void RenderUnitCube();

    GLuint m_CubeArrayTexture = 0;
    GLuint m_CaptureCubeMap = 0;
    GLuint m_CaptureFBO = 0;
    GLuint m_CaptureRBO = 0;

    GLuint m_UnitCubeVAO = 0;
    GLuint m_UnitCubeVBO = 0;

    uint32_t m_Resolution = 256;
    uint32_t m_MipLevels = 8;
    uint32_t m_MaxProbes = MAX_GPU_REFLECTION_PROBES;
    bool m_Initialized = false;
};

}
