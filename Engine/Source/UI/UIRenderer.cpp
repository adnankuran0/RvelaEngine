#include "rvelapch.h"
#include "UIRenderer.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Core/Log.h"

namespace rv {

uint32_t UIRenderer::s_VAO = 0;
uint32_t UIRenderer::s_VBO = 0;
uint32_t UIRenderer::s_EBO = 0;

std::shared_ptr<Shader> UIRenderer::s_UIShader = nullptr;
std::shared_ptr<Shader> UIRenderer::s_CurrentShader = nullptr;

UIVertex* UIRenderer::s_BufferBase = nullptr;
UIVertex* UIRenderer::s_BufferPtr = nullptr;
uint32_t UIRenderer::s_IndexCount = 0;
uint32_t UIRenderer::s_ActiveTextureID = 0;

float UIRenderer::s_ViewportWidth = 1920.0f;
float UIRenderer::s_ViewportHeight = 1080.0f;
glm::mat4 UIRenderer::s_ProjectionMatrix = glm::mat4(1.0f);

static uint32_t CreateGLTexture(uint32_t width, uint32_t height, const void* data)
{
    uint32_t texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

uint32_t UIRenderer::s_WhiteTextureID = 0;

void UIRenderer::Init()
{
    if (s_VAO != 0) return;

    s_UIShader = std::make_shared<Shader>("UI", ENGINE_PATH("Shaders\\ui.glsl"));

    uint32_t whitePixel = 0xFFFFFFFF;
    s_WhiteTextureID = CreateGLTexture(1, 1, &whitePixel);

    s_BufferBase = new UIVertex[MaxVertices];

    glGenVertexArrays(1, &s_VAO);
    glGenBuffers(1, &s_VBO);
    glGenBuffers(1, &s_EBO);

    glBindVertexArray(s_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferData(GL_ARRAY_BUFFER, MaxVertices * sizeof(UIVertex), nullptr, GL_DYNAMIC_DRAW);

    uint32_t* indices = new uint32_t[MaxIndices];
    uint32_t offset = 0;
    for (uint32_t i = 0; i < MaxIndices; i += 6) {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;
        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;
        offset += 4;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MaxIndices * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    delete[] indices;

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (const void*)offsetof(UIVertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (const void*)offsetof(UIVertex, color));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (const void*)offsetof(UIVertex, texCoord));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (const void*)offsetof(UIVertex, useTexture));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (const void*)offsetof(UIVertex, quadSize));

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (const void*)offsetof(UIVertex, localPos));

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (const void*)offsetof(UIVertex, cornerRadius));

    glBindVertexArray(0);
}

void UIRenderer::Shutdown()
{
    if (s_BufferBase) {
        delete[] s_BufferBase;
        s_BufferBase = nullptr;
    }
    if (s_VAO) {
        glDeleteVertexArrays(1, &s_VAO);
        glDeleteBuffers(1, &s_VBO);
        glDeleteBuffers(1, &s_EBO);
        s_VAO = s_VBO = s_EBO = 0;
    }
    if (s_WhiteTextureID) {
        glDeleteTextures(1, &s_WhiteTextureID);
        s_WhiteTextureID = 0;
    }
    s_UIShader.reset();
}

void UIRenderer::Begin(float viewportWidth, float viewportHeight, const glm::mat4& viewProjMatrix, std::shared_ptr<Shader> customShader)
{
    if (s_VAO == 0 || s_BufferBase == nullptr) {
        Init();
    }

    s_ViewportWidth = viewportWidth;
    s_ViewportHeight = viewportHeight;

    if (viewProjMatrix == glm::mat4(1.0f)) {
        s_ProjectionMatrix = glm::ortho(0.0f, viewportWidth, 0.0f, viewportHeight, -100.0f, 100.0f);
    } else {
        s_ProjectionMatrix = viewProjMatrix;
    }

    s_CurrentShader = customShader ? customShader : s_UIShader;
    s_BufferPtr = s_BufferBase;
    s_IndexCount = 0;
    s_ActiveTextureID = s_WhiteTextureID;
}

void UIRenderer::End()
{
    Flush();
}

void UIRenderer::Flush()
{
    if (s_IndexCount == 0) return;

    GLsizeiptr size = (uint8_t*)s_BufferPtr - (uint8_t*)s_BufferBase;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    auto activeShader = s_CurrentShader ? s_CurrentShader : s_UIShader;
    activeShader->use();
    activeShader->setMat4("u_Projection", s_ProjectionMatrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_ActiveTextureID);
    s_UIShader->setInt("u_Texture", 0);

    glBindVertexArray(s_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, s_BufferBase);

    glDrawElements(GL_TRIANGLES, s_IndexCount, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);

    s_BufferPtr = s_BufferBase;
    s_IndexCount = 0;
}

void UIRenderer::DrawQuad(const glm::vec4& rect, const glm::vec4& color, float cornerRadius)
{
    DrawQuad(rect, color, s_WhiteTextureID, cornerRadius);
}

void UIRenderer::DrawQuad(const glm::vec4& rect, const glm::vec4& color, uint32_t textureID, float cornerRadius)
{
    DrawQuadRotated(rect, 0.0f, color, textureID, cornerRadius);
}

void UIRenderer::DrawQuadRotated(const glm::vec4& rect, float rotationDegrees, const glm::vec4& color, uint32_t textureID, float cornerRadius, const glm::vec2& customCenter)
{
    uint32_t texId = (textureID != 0) ? textureID : s_WhiteTextureID;

    if (s_IndexCount >= MaxIndices || (s_ActiveTextureID != texId && s_IndexCount > 0)) {
        Flush();
    }
    s_ActiveTextureID = texId;

    float w = rect.z - rect.x;
    float h = rect.w - rect.y;
    float hw = w * 0.5f;
    float hh = h * 0.5f;
    glm::vec2 center = (customCenter != glm::vec2(0.0f)) ? customCenter : glm::vec2((rect.x + rect.z) * 0.5f, (rect.y + rect.w) * 0.5f);

    glm::vec2 p0, p1, p2, p3;

    if (rotationDegrees == 0.0f) {
        p0 = { rect.x, rect.y };
        p1 = { rect.z, rect.y };
        p2 = { rect.z, rect.w };
        p3 = { rect.x, rect.w };
    } else {
        float rad = glm::radians(rotationDegrees);
        float cosR = cos(rad);
        float sinR = sin(rad);

        auto RotatePoint = [&](float x, float y) -> glm::vec2 {
            float dx = x - center.x;
            float dy = y - center.y;
            return glm::vec2(center.x + dx * cosR - dy * sinR, center.y + dx * sinR + dy * cosR);
        };

        p0 = RotatePoint(rect.x, rect.y);
        p1 = RotatePoint(rect.z, rect.y);
        p2 = RotatePoint(rect.z, rect.w);
        p3 = RotatePoint(rect.x, rect.w);
    }

    float useTex = (textureID != 0) ? 1.0f : 0.0f;
    glm::vec2 qSize = { w, h };

    s_BufferPtr->position = { p0.x, p0.y, 0.0f };
    s_BufferPtr->color = color;
    s_BufferPtr->texCoord = { 0.0f, 0.0f };
    s_BufferPtr->useTexture = useTex;
    s_BufferPtr->quadSize = qSize;
    s_BufferPtr->localPos = { -hw, -hh };
    s_BufferPtr->cornerRadius = cornerRadius;
    s_BufferPtr++;

    s_BufferPtr->position = { p1.x, p1.y, 0.0f };
    s_BufferPtr->color = color;
    s_BufferPtr->texCoord = { 1.0f, 0.0f };
    s_BufferPtr->useTexture = useTex;
    s_BufferPtr->quadSize = qSize;
    s_BufferPtr->localPos = { hw, -hh };
    s_BufferPtr->cornerRadius = cornerRadius;
    s_BufferPtr++;

    s_BufferPtr->position = { p2.x, p2.y, 0.0f };
    s_BufferPtr->color = color;
    s_BufferPtr->texCoord = { 1.0f, 1.0f };
    s_BufferPtr->useTexture = useTex;
    s_BufferPtr->quadSize = qSize;
    s_BufferPtr->localPos = { hw, hh };
    s_BufferPtr->cornerRadius = cornerRadius;
    s_BufferPtr++;

    s_BufferPtr->position = { p3.x, p3.y, 0.0f };
    s_BufferPtr->color = color;
    s_BufferPtr->texCoord = { 0.0f, 1.0f };
    s_BufferPtr->useTexture = useTex;
    s_BufferPtr->quadSize = qSize;
    s_BufferPtr->localPos = { -hw, hh };
    s_BufferPtr->cornerRadius = cornerRadius;
    s_BufferPtr++;

    s_IndexCount += 6;
}

void UIRenderer::DrawNineSlice(const glm::vec4& rect, const glm::vec4& color, float borderSize, uint32_t textureID)
{
    float minX = rect.x;
    float minY = rect.y;
    float maxX = rect.z;
    float maxY = rect.w;

    float bX = std::min(borderSize, (maxX - minX) * 0.5f);
    float bY = std::min(borderSize, (maxY - minY) * 0.5f);

    // Center fill
    DrawQuad(glm::vec4(minX + bX, minY + bY, maxX - bX, maxY - bY), color, textureID);
    // Borders
    DrawQuad(glm::vec4(minX, minY + bY, minX + bX, maxY - bY), color, textureID);
    DrawQuad(glm::vec4(maxX - bX, minY + bY, maxX, maxY - bY), color, textureID);
    DrawQuad(glm::vec4(minX + bX, minY, maxX - bX, minY + bY), color, textureID);
    DrawQuad(glm::vec4(minX + bX, maxY - bY, maxX - bX, maxY), color, textureID);
    // Corners
    DrawQuad(glm::vec4(minX, minY, minX + bX, minY + bY), color, textureID);
    DrawQuad(glm::vec4(maxX - bX, minY, maxX, minY + bY), color, textureID);
    DrawQuad(glm::vec4(minX, maxY - bY, minX + bX, maxY), color, textureID);
    DrawQuad(glm::vec4(maxX - bX, maxY - bY, maxX, maxY), color, textureID);
}

static uint32_t DecodeNextUTF8Char(const char*& str)
{
    uint32_t c = (unsigned char)*str;
    if (c == 0) return 0;
    str++;

    if (c < 0x80)
    {
        return c;
    }
    else if ((c & 0xE0) == 0xC0)
    {
        uint32_t c2 = (unsigned char)*str++;
        return ((c & 0x1F) << 6) | (c2 & 0x3F);
    }
    else if ((c & 0xF0) == 0xE0)
    {
        uint32_t c2 = (unsigned char)*str++;
        uint32_t c3 = (unsigned char)*str++;
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }
    else if ((c & 0xF8) == 0xF0)
    {
        uint32_t c2 = (unsigned char)*str++;
        uint32_t c3 = (unsigned char)*str++;
        uint32_t c4 = (unsigned char)*str++;
        return ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
    }
    return c;
}

static void AppendUTF8Char(std::string& str, uint32_t cp)
{
    if (cp < 0x80) {
        str += (char)cp;
    } else if (cp < 0x800) {
        str += (char)(0xC0 | (cp >> 6));
        str += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        str += (char)(0xE0 | (cp >> 12));
        str += (char)(0x80 | ((cp >> 6) & 0x3F));
        str += (char)(0x80 | (cp & 0x3F));
    } else {
        str += (char)(0xF0 | (cp >> 18));
        str += (char)(0x80 | ((cp >> 12) & 0x3F));
        str += (char)(0x80 | ((cp >> 6) & 0x3F));
        str += (char)(0x80 | (cp & 0x3F));
    }
}

void UIRenderer::DrawTextString(const std::string& text, const glm::vec2& position, float fontSize, const glm::vec4& color, TextAlignment align, float rotationDegrees, const glm::vec2& pivotPoint, std::shared_ptr<Font> font, bool isBold, bool isOutline, float outlineSize, const glm::vec4& outlineColor, float maxWidth, float lineSpacing, float characterSpacing)
{
    if (text.empty()) return;

    if (!font) {
        font = Font::GetDefault();
    }

    if (!font || font->GetTextureID() == 0) return;

    uint32_t fontTexID = font->GetTextureID();
    float fontNativeSize = font->GetFontSize();
    float scale = (fontNativeSize > 0.0f) ? (fontSize / fontNativeSize) : 1.0f;

    struct TextLine {
        std::string lineText;
        float width = 0.0f;
    };

    std::vector<TextLine> lines;
    std::string currentLine;
    float currentLineWidth = 0.0f;
    std::string currentWord;
    float currentWordWidth = 0.0f;

    const char* strPtr = text.c_str();
    while (*strPtr)
    {
        uint32_t cp = DecodeNextUTF8Char(strPtr);
        if (cp == 0) break;
        if (cp == '\r') continue;

        if (cp == '\n')
        {
            currentLine += currentWord;
            currentLineWidth += currentWordWidth;
            lines.push_back({ currentLine, currentLineWidth });
            currentLine.clear();
            currentLineWidth = 0.0f;
            currentWord.clear();
            currentWordWidth = 0.0f;
            continue;
        }

        float charWidth = (font->GetSDFGlyph(cp).xadvance + characterSpacing) * scale;

        if (cp == ' ' || cp == '\t')
        {
            currentLine += currentWord;
            currentLineWidth += currentWordWidth;
            currentWord.clear();
            currentWordWidth = 0.0f;

            if (maxWidth > 0.0f && currentLineWidth + charWidth > maxWidth && !currentLine.empty())
            {
                lines.push_back({ currentLine, currentLineWidth });
                currentLine.clear();
                currentLineWidth = 0.0f;
            }
            else
            {
                AppendUTF8Char(currentLine, cp);
                currentLineWidth += charWidth;
            }
        }
        else
        {
            if (maxWidth > 0.0f && currentLineWidth + currentWordWidth + charWidth > maxWidth && (!currentLine.empty() || !currentWord.empty()))
            {
                if (!currentLine.empty())
                {
                    lines.push_back({ currentLine, currentLineWidth });
                    currentLine.clear();
                    currentLineWidth = 0.0f;
                }
                else
                {
                    lines.push_back({ currentWord, currentWordWidth });
                    currentWord.clear();
                    currentWordWidth = 0.0f;
                }
            }
            AppendUTF8Char(currentWord, cp);
            currentWordWidth += charWidth;
        }
    }

    currentLine += currentWord;
    currentLineWidth += currentWordWidth;
    if (!currentLine.empty() || lines.empty())
    {
        lines.push_back({ currentLine, currentLineWidth });
    }

    float rad = glm::radians(rotationDegrees);
    float cosR = cos(rad);
    float sinR = sin(rad);

    auto RotatePoint = [&](float x, float y) -> glm::vec2 {
        if (rotationDegrees == 0.0f) return glm::vec2(x, y);
        float dx = x - pivotPoint.x;
        float dy = y - pivotPoint.y;
        return glm::vec2(pivotPoint.x + dx * cosR - dy * sinR, pivotPoint.y + dx * sinR + dy * cosR);
    };

    auto DrawSinglePass = [&](const glm::vec2& basePos, const glm::vec4& passColor) {
        float fontScale = (fontNativeSize > 0.0f) ? (fontSize / fontNativeSize) : 1.0f;
        float asc = font->GetAscent() * fontScale;
        float dsc = font->GetDescent() * fontScale;

        float lineOffsetStep = fontSize * lineSpacing;
        float boldOffset = isBold ? 0.08f : 0.0f;

        float lineCenterAboveBaseline = (asc + dsc) * 0.5f;
        float firstLineY = basePos.y + (((float)lines.size() - 1.0f) * lineOffsetStep * 0.5f) - lineCenterAboveBaseline;

        for (size_t lineIdx = 0; lineIdx < lines.size(); ++lineIdx)
        {
            const auto& line = lines[lineIdx];
            float startX = basePos.x;
            if (align == TextAlignment::Center) {
                startX -= line.width * 0.5f;
            } else if (align == TextAlignment::Right) {
                startX -= line.width;
            }

            float lineY = firstLineY - (float)lineIdx * lineOffsetStep;
            float curX = startX;
            float curY = lineY;

            const char* linePtr = line.lineText.c_str();
            while (*linePtr) {
                uint32_t cp = DecodeNextUTF8Char(linePtr);
                if (cp == 0) break;

                const auto& g = font->GetSDFGlyph(cp);

                float x0 = curX + g.xoff * scale;
                float x1 = x0 + g.width * scale;

                float y1 = curY - g.yoff * scale;
                float y0 = y1 - g.height * scale;

                if (s_IndexCount + 6 >= MaxIndices || (s_ActiveTextureID != fontTexID && s_IndexCount > 0)) {
                    Flush();
                }
                s_ActiveTextureID = fontTexID;

                glm::vec2 p0 = RotatePoint(x0, y0);
                glm::vec2 p1 = RotatePoint(x1, y0);
                glm::vec2 p2 = RotatePoint(x1, y1);
                glm::vec2 p3 = RotatePoint(x0, y1);

                float useTex = 2.0f; // 2.0 = SDF Font

                s_BufferPtr->position = { p0.x, p0.y, 0.0f };
                s_BufferPtr->color = passColor;
                s_BufferPtr->texCoord = { g.u0, g.v1 };
                s_BufferPtr->useTexture = useTex;
                s_BufferPtr->quadSize = { 0.0f, 0.0f };
                s_BufferPtr->localPos = { 0.0f, 0.0f };
                s_BufferPtr->cornerRadius = boldOffset;
                s_BufferPtr++;

                s_BufferPtr->position = { p1.x, p1.y, 0.0f };
                s_BufferPtr->color = passColor;
                s_BufferPtr->texCoord = { g.u1, g.v1 };
                s_BufferPtr->useTexture = useTex;
                s_BufferPtr->quadSize = { 0.0f, 0.0f };
                s_BufferPtr->localPos = { 0.0f, 0.0f };
                s_BufferPtr->cornerRadius = boldOffset;
                s_BufferPtr++;

                s_BufferPtr->position = { p2.x, p2.y, 0.0f };
                s_BufferPtr->color = passColor;
                s_BufferPtr->texCoord = { g.u1, g.v0 };
                s_BufferPtr->useTexture = useTex;
                s_BufferPtr->quadSize = { 0.0f, 0.0f };
                s_BufferPtr->localPos = { 0.0f, 0.0f };
                s_BufferPtr->cornerRadius = boldOffset;
                s_BufferPtr++;

                s_BufferPtr->position = { p3.x, p3.y, 0.0f };
                s_BufferPtr->color = passColor;
                s_BufferPtr->texCoord = { g.u0, g.v0 };
                s_BufferPtr->useTexture = useTex;
                s_BufferPtr->quadSize = { 0.0f, 0.0f };
                s_BufferPtr->localPos = { 0.0f, 0.0f };
                s_BufferPtr->cornerRadius = boldOffset;
                s_BufferPtr++;

                s_IndexCount += 6;

                curX += (g.xadvance + characterSpacing) * scale;
            }
        }
    };

    // outline
    if (isOutline && outlineSize > 0.0f && outlineColor.a > 0.0f) {
        static const glm::vec2 outlineDirs[8] = {
            { -1.0f,  0.0f },
            {  1.0f,  0.0f },
            {  0.0f, -1.0f },
            {  0.0f,  1.0f },
            { -0.707f, -0.707f },
            {  0.707f, -0.707f },
            { -0.707f,  0.707f },
            {  0.707f,  0.707f }
        };

        for (int i = 0; i < 8; ++i) {
            glm::vec2 outlinePos = position + outlineDirs[i] * outlineSize;
            DrawSinglePass(outlinePos, outlineColor);
        }
    }

    DrawSinglePass(position, color);
}

void UIRenderer::SetScissor(int x, int y, int width, int height)
{
    Flush();
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
}

void UIRenderer::ResetScissor()
{
    Flush();
    glDisable(GL_SCISSOR_TEST);
}

}
