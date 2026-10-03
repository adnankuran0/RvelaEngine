#include "rvelapch.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "Font.h"
#include <glad/gl.h>
#include <fstream>
#include "Utils/FileUtils.h"
#include "Core/Log.h"

namespace rv {

std::shared_ptr<Font> Font::s_DefaultFont = nullptr;

Font::~Font()
{
    if (m_TextureID != 0) {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }
}

std::shared_ptr<Font> Font::GetDefault()
{
    if (!s_DefaultFont)
    {
        s_DefaultFont = std::make_shared<Font>();
        std::string fontPath = ENGINE_PATH("Fonts\\roboto.ttf").GetAbsoluteStr();
        if (!s_DefaultFont->Load(fontPath, 48.0f))
        {
            std::string editorFontPath = EDITOR_PATH("Fonts\\roboto.ttf").GetAbsoluteStr();
            s_DefaultFont->Load(editorFontPath, 48.0f);
        }
    }
    return s_DefaultFont;
}

std::shared_ptr<Font> Font::LoadFromFile(const std::string& filepath, float fontSize)
{
    auto font = std::make_shared<Font>();
    if (font->Load(filepath, fontSize))
    {
        return font;
    }
    return GetDefault();
}

bool Font::Load(const std::string& filepath, float fontSize)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LOG_WARN("Failed to open font file: {}", filepath);
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> fontBuffer(size);
    if (!file.read((char*)fontBuffer.data(), size))
    {
        LOG_WARN("Failed to read font file data: {}", filepath);
        return false;
    }

    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, fontBuffer.data(), 0))
    {
        LOG_WARN("Failed to init stb_truetype font: {}", filepath);
        return false;
    }

    float baseSize = fontSize > 0.0f ? fontSize : 48.0f;
    m_FontSize = baseSize;
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, baseSize);

    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    if (ascent != 0 || descent != 0) {
        m_Ascent = (float)ascent * scale;
        m_Descent = (float)descent * scale;
    } else {
        m_Ascent = baseSize * 0.8f;
        m_Descent = -baseSize * 0.2f;
    }

    const int atlasW = 1024;
    const int atlasH = 1024;
    std::vector<unsigned char> alphaBitmap(atlasW * atlasH, 0);

    int padding = 6;
    unsigned char onedge_value = 128;
    float pixel_dist_scale = 128.0f / (float)padding;

    int currentX = 1;
    int currentY = 1;
    int maxHeightInRow = 0;

    std::vector<uint32_t> codepointsToLoad;
    for (uint32_t cp = 32; cp <= 126; ++cp) codepointsToLoad.push_back(cp);
    // Turkish support
    for (uint32_t cp = 0x00C0; cp <= 0x017F; ++cp) codepointsToLoad.push_back(cp);

    m_SDFGlyphs.clear();

    for (uint32_t cp : codepointsToLoad)
    {
        int gWidth = 0, gHeight = 0, xoff = 0, yoff = 0;
        unsigned char* sdf = stbtt_GetCodepointSDF(&fontInfo, scale, cp, padding, onedge_value, pixel_dist_scale, &gWidth, &gHeight, &xoff, &yoff);

        int advanceWidth = 0, leftSideBearing = 0;
        stbtt_GetCodepointHMetrics(&fontInfo, cp, &advanceWidth, &leftSideBearing);

        if (currentX + gWidth + 2 >= atlasW)
        {
            currentX = 1;
            currentY += maxHeightInRow + 2;
            maxHeightInRow = 0;
        }

        if (sdf && gWidth > 0 && gHeight > 0)
        {
            for (int row = 0; row < gHeight; ++row)
            {
                for (int col = 0; col < gWidth; ++col)
                {
                    int dstIndex = (currentY + row) * atlasW + (currentX + col);
                    alphaBitmap[dstIndex] = sdf[row * gWidth + col];
                }
            }
        }

        if (sdf)
        {
            stbtt_FreeSDF(sdf, nullptr);
        }

        SDFGlyph glyph{};
        glyph.u0 = (float)currentX / (float)atlasW;
        glyph.v0 = (float)currentY / (float)atlasH;
        glyph.u1 = (float)(currentX + gWidth) / (float)atlasW;
        glyph.v1 = (float)(currentY + gHeight) / (float)atlasH;
        glyph.xoff = (float)xoff;
        glyph.yoff = (float)yoff;
        glyph.width = (float)gWidth;
        glyph.height = (float)gHeight;
        glyph.xadvance = (float)advanceWidth * scale;

        m_SDFGlyphs[cp] = glyph;

        currentX += gWidth + 2;
        if (gHeight > maxHeightInRow) maxHeightInRow = gHeight;
    }

    m_FontSize = baseSize;
    m_IsSDF = true;

    // Convert 1 channel SDF to RGBA
    std::vector<uint32_t> rgbaPixels(atlasW * atlasH);
    for (int i = 0; i < atlasW * atlasH; ++i)
    {
        uint8_t dist = alphaBitmap[i];
        rgbaPixels[i] = (dist << 24) | (dist << 16) | (dist << 8) | dist;
    }

    if (m_TextureID != 0)
    {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }

    glGenTextures(1, &m_TextureID);
    glBindTexture(GL_TEXTURE_2D, m_TextureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasW, atlasH, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

const SDFGlyph& Font::GetSDFGlyph(uint32_t codepoint) const
{
    auto it = m_SDFGlyphs.find(codepoint);
    if (it != m_SDFGlyphs.end())
    {
        return it->second;
    }
    // Fallback to ?
    auto fallbackIt = m_SDFGlyphs.find(63);
    if (fallbackIt != m_SDFGlyphs.end())
    {
        return fallbackIt->second;
    }
    static SDFGlyph emptyGlyph;
    return emptyGlyph;
}

}


