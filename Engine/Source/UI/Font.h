#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include "Asset/AssetUUID.h"
#include <ImGui/imstb_truetype.h>

namespace rv {

struct SDFGlyph
{
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    float xoff = 0.0f, yoff = 0.0f;
    float width = 0.0f, height = 0.0f;
    float xadvance = 0.0f;
};

struct GlyphQuad
{
    glm::vec4 positionRect; // minX, minY, maxX, maxY
    glm::vec4 uvRect; // u0, v0, u1, v1
    float xAdvance;
};

class Font
{
public:
    Font() = default;
    ~Font();

    static std::shared_ptr<Font> GetDefault();
    static std::shared_ptr<Font> LoadFromFile(const std::string& filepath, float fontSize = 48.0f);

    bool Load(const std::string& filepath, float fontSize = 48.0f);

    uint32_t GetTextureID() const { return m_TextureID; }
    float GetFontSize() const { return m_FontSize; }
    bool IsSDF() const { return m_IsSDF; }
    float GetAscent() const { return m_Ascent; }
    float GetDescent() const { return m_Descent; }

    const SDFGlyph& GetSDFGlyph(uint32_t codepoint) const;

private:
    uint32_t m_TextureID = 0;
    std::unordered_map<uint32_t, SDFGlyph> m_SDFGlyphs;
    float m_FontSize = 48.0f;
    float m_Ascent = 38.0f;
    float m_Descent = -10.0f;
    bool m_IsSDF = true;

    static std::shared_ptr<Font> s_DefaultFont;
};

}

