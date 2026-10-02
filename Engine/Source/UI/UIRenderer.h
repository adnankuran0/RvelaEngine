#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"
#include "Scene/Components/UIComponents.h"
#include "Font.h"

namespace rv {

struct UIVertex
{
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
    float useTexture;
    glm::vec2 quadSize;
    glm::vec2 localPos;
    float cornerRadius;
};

class UIRenderer
{
public:
    static void Init();
    static void Shutdown();

    static void Begin(float viewportWidth, float viewportHeight, const glm::mat4& viewProjMatrix = glm::mat4(1.0f), std::shared_ptr<Shader> customShader = nullptr);
    static void End();

    static void DrawQuad(const glm::vec4& rect, const glm::vec4& color, float cornerRadius = 0.0f);
    static void DrawQuad(const glm::vec4& rect, const glm::vec4& color, uint32_t textureID, float cornerRadius = 0.0f);
    static void DrawQuadRotated(const glm::vec4& rect, float rotationDegrees, const glm::vec4& color, uint32_t textureID = 0, float cornerRadius = 0.0f, const glm::vec2& customCenter = glm::vec2(0.0f));
    static void DrawNineSlice(const glm::vec4& rect, const glm::vec4& color, float borderSize, uint32_t textureID = 0);
    
    static void DrawTextString(const std::string& text, const glm::vec2& position, float fontSize, const glm::vec4& color, TextAlignment align = TextAlignment::Left, float rotationDegrees = 0.0f, const glm::vec2& pivotPoint = glm::vec2(0.0f), std::shared_ptr<Font> font = nullptr, bool isBold = false, bool isOutline = false, float outlineSize = 1.0f, const glm::vec4& outlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), float maxWidth = 0.0f, float lineSpacing = 1.2f, float characterSpacing = 0.0f);

    static void SetScissor(int x, int y, int width, int height);
    static void ResetScissor();

private:
    static void Flush();

    static uint32_t s_VAO;
    static uint32_t s_VBO;
    static uint32_t s_EBO;

    static std::shared_ptr<Shader> s_UIShader;
    static std::shared_ptr<Shader> s_CurrentShader;
    static uint32_t s_WhiteTextureID;

    static const uint32_t MaxQuads = 4000;
    static const uint32_t MaxVertices = MaxQuads * 4;
    static const uint32_t MaxIndices = MaxQuads * 6;

    static UIVertex* s_BufferBase;
    static UIVertex* s_BufferPtr;
    static uint32_t s_IndexCount;
    static uint32_t s_ActiveTextureID;

    static float s_ViewportWidth;
    static float s_ViewportHeight;
    static glm::mat4 s_ProjectionMatrix;
};

}
