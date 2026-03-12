#pragma once
#include <memory>
#include "Rendering/VertexArray.h"
#include "Rendering/VertexBuffer.h"

namespace rv {

struct DebugVertex
{
    glm::vec3 position;
    glm::vec4 color;
};

class DebugRenderer
{
public:
    DebugRenderer(const DebugRenderer&) = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;
    DebugRenderer(DebugRenderer&&) = delete;
    DebugRenderer& operator=(DebugRenderer&&) = delete;

    void Init();

    void BeginFrame();
    void EndFrame(const glm::mat4& mvp);

    void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color);

    static DebugRenderer& Get()
    {
        static DebugRenderer s_Instance;
        return s_Instance;
    }

private:
    std::vector<DebugVertex> m_LineVertices;
    std::vector<DebugVertex> m_TriangleVertices;

    VertexArray m_LineVAO;
    VertexBuffer m_LineVBO;
    DebugRenderer() = default;
};

}
