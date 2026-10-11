#include "rvelapch.h"
#include "CustomShaderGenerator.h"
#include <regex>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include "Utils/FileUtils.h"

namespace rv {

static std::string ExtractFunctionBody(const std::string& source, const std::string& funcName, size_t& outStart, size_t& outEnd)
{
    outStart = std::string::npos;
    outEnd = std::string::npos;

    std::regex funcRegex(R"(\bvoid\s+)" + funcName + R"(\s*\(\s*\))");
    std::smatch match;
    if (!std::regex_search(source, match, funcRegex))
        return "";

    size_t matchPos = match.position();
    size_t openBrace = source.find('{', matchPos);
    if (openBrace == std::string::npos)
        return "";

    int depth = 1;
    size_t i = openBrace + 1;
    for (; i < source.size(); ++i)
    {
        if (source[i] == '{') depth++;
        else if (source[i] == '}')
        {
            depth--;
            if (depth == 0)
                break;
        }
    }

    if (depth != 0)
        return "";

    outStart = matchPos;
    outEnd = i + 1; // inclusive of closing brace
    return source.substr(openBrace + 1, i - openBrace - 1);
}

static std::string CleanDisplayName(const std::string& name)
{
    std::string s = name;
    if (s.rfind("u_", 0) == 0)
        s = s.substr(2);
    else if (s.rfind("m_", 0) == 0)
        s = s.substr(2);

    for (char& c : s)
    {
        if (c == '_') c = ' ';
    }
    if (!s.empty())
        s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

void CustomShaderGenerator::ParseUniforms(
    const std::string& source,
    std::vector<ShaderUniformDef>& outUniforms,
    std::string& outCleanGlobals
)
{
    std::istringstream stream(source);
    std::stringstream cleanStream;
    std::string line;

    // Matches: uniform <type> <name> [: <hint>] [= <default>];
    std::regex uniformRegex(R"(^\s*uniform\s+([a-zA-Z0-9_]+)\s+([a-zA-Z0-9_]+)(?:\s*:\s*([^=;]+?))?(?:\s*=\s*([^;]+))?\s*;)");

    while (std::getline(stream, line))
    {
        // Strip carriage return
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // Skip shader_type directive
        if (line.find("shader_type") != std::string::npos)
            continue;

        // Skip render_mode directive
        if (line.find("render_mode") != std::string::npos)
            continue;

        std::smatch match;
        if (std::regex_search(line, match, uniformRegex))
        {
            std::string typeStr = match[1].str();
            std::string nameStr = match[2].str();
            std::string hintStr = match[3].matched ? match[3].str() : "";
            std::string defaultStr = match[4].matched ? match[4].str() : "";

            ShaderUniformDef uDef;
            uDef.name = nameStr;
            uDef.displayName = CleanDisplayName(nameStr);

            if (typeStr == "float") uDef.type = ShaderUniformType::Float;
            else if (typeStr == "int") uDef.type = ShaderUniformType::Int;
            else if (typeStr == "bool") uDef.type = ShaderUniformType::Bool;
            else if (typeStr == "vec2") uDef.type = ShaderUniformType::Vec2;
            else if (typeStr == "vec3") uDef.type = ShaderUniformType::Vec3;
            else if (typeStr == "vec4") uDef.type = ShaderUniformType::Vec4;
            else if (typeStr == "sampler2D") uDef.type = ShaderUniformType::Texture2D;
            else continue; // Unknown uniform type

            // Check hints
            if (hintStr.find("source_color") != std::string::npos || hintStr.find("hint_color") != std::string::npos)
            {
                if (uDef.type == ShaderUniformType::Vec3) uDef.type = ShaderUniformType::Color3;
                else if (uDef.type == ShaderUniformType::Vec4) uDef.type = ShaderUniformType::Color4;
            }
            else if (nameStr.find("color") != std::string::npos || nameStr.find("Color") != std::string::npos)
            {
                if (uDef.type == ShaderUniformType::Vec3) uDef.type = ShaderUniformType::Color3;
                else if (uDef.type == ShaderUniformType::Vec4) uDef.type = ShaderUniformType::Color4;
            }

            if (uDef.type == ShaderUniformType::Texture2D)
            {
                if (hintStr.find("hint_screen_texture") != std::string::npos)
                {
                    uDef.engineTexture = EngineTextureType::Screen;
                }
                else if (hintStr.find("hint_depth_texture") != std::string::npos)
                {
                    uDef.engineTexture = EngineTextureType::Depth;
                }
                else if (hintStr.find("hint_normal_roughness_texture") != std::string::npos ||
                         hintStr.find("hint_normal_texture") != std::string::npos)
                {
                    uDef.engineTexture = EngineTextureType::Normal;
                }

                // Sampler filter hints
                if (hintStr.find("filter_nearest_mipmap") != std::string::npos)
                {
                    uDef.hasCustomSampler = true;
                    uDef.samplerDesc.minFilter = MinFilter::NEAREST_MIPMAP_LINEAR;
                    uDef.samplerDesc.magFilter = MagFilter::NEAREST;
                }
                else if (hintStr.find("filter_nearest") != std::string::npos)
                {
                    uDef.hasCustomSampler = true;
                    uDef.samplerDesc.minFilter = MinFilter::NEAREST;
                    uDef.samplerDesc.magFilter = MagFilter::NEAREST;
                }
                else if (hintStr.find("filter_linear_mipmap") != std::string::npos || hintStr.find("filter_mipmap") != std::string::npos)
                {
                    uDef.hasCustomSampler = true;
                    uDef.samplerDesc.minFilter = MinFilter::LINEAR_MIPMAP_LINEAR;
                    uDef.samplerDesc.magFilter = MagFilter::LINEAR;
                }
                else if (hintStr.find("filter_linear") != std::string::npos)
                {
                    uDef.hasCustomSampler = true;
                    uDef.samplerDesc.minFilter = MinFilter::LINEAR;
                    uDef.samplerDesc.magFilter = MagFilter::LINEAR;
                }

                // Sampler repeat and wrap hints
                if (hintStr.find("repeat_disable") != std::string::npos || hintStr.find("clamp_to_edge") != std::string::npos)
                {
                    uDef.hasCustomSampler = true;
                    uDef.samplerDesc.wrap = Wrap::CLAMP_TO_EDGE;
                }
                else if (hintStr.find("repeat_enable") != std::string::npos)
                {
                    uDef.hasCustomSampler = true;
                    uDef.samplerDesc.wrap = Wrap::REPEAT;
                }
            }

            if (hintStr.find("hint_range") != std::string::npos)
            {
                std::regex rangeRegex(R"(hint_range\s*\(\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+))");
                std::smatch rMatch;
                if (std::regex_search(hintStr, rMatch, rangeRegex))
                {
                    uDef.hasRange = true;
                    uDef.minVal = std::stof(rMatch[1].str());
                    uDef.maxVal = std::stof(rMatch[2].str());
                }
            }

            // Parse default values
            if (!defaultStr.empty())
            {
                try {
                    if (uDef.type == ShaderUniformType::Float)
                    {
                        uDef.floatVal = std::stof(defaultStr);
                    }
                    else if (uDef.type == ShaderUniformType::Int)
                    {
                        uDef.intVal = std::stoi(defaultStr);
                    }
                    else if (uDef.type == ShaderUniformType::Bool)
                    {
                        uDef.boolVal = (defaultStr.find("true") != std::string::npos || defaultStr.find("1") != std::string::npos);
                    }
                    else if (uDef.type == ShaderUniformType::Vec2)
                    {
                        std::regex v2Regex(R"(vec2\s*\(\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*\))");
                        std::smatch vMatch;
                        if (std::regex_search(defaultStr, vMatch, v2Regex))
                        {
                            uDef.vec2Val = glm::vec2(std::stof(vMatch[1].str()), std::stof(vMatch[2].str()));
                        }
                    }
                    else if (uDef.type == ShaderUniformType::Vec3 || uDef.type == ShaderUniformType::Color3)
                    {
                        std::regex v3Regex(R"(vec3\s*\(\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*\))");
                        std::smatch vMatch;
                        if (std::regex_search(defaultStr, vMatch, v3Regex))
                        {
                            uDef.vec3Val = glm::vec3(std::stof(vMatch[1].str()), std::stof(vMatch[2].str()), std::stof(vMatch[3].str()));
                        }
                        else
                        {
                            std::regex v1Regex(R"(vec3\s*\(\s*([-+]?[0-9]*\.?[0-9]+)\s*\))");
                            if (std::regex_search(defaultStr, vMatch, v1Regex))
                            {
                                float val = std::stof(vMatch[1].str());
                                uDef.vec3Val = glm::vec3(val);
                            }
                        }
                    }
                    else if (uDef.type == ShaderUniformType::Vec4 || uDef.type == ShaderUniformType::Color4)
                    {
                        std::regex v4Regex(R"(vec4\s*\(\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*\))");
                        std::smatch vMatch;
                        if (std::regex_search(defaultStr, vMatch, v4Regex))
                        {
                            uDef.vec4Val = glm::vec4(std::stof(vMatch[1].str()), std::stof(vMatch[2].str()), std::stof(vMatch[3].str()), std::stof(vMatch[4].str()));
                        }
                        else
                        {
                            std::regex v1Regex(R"(vec4\s*\(\s*([-+]?[0-9]*\.?[0-9]+)\s*\))");
                            if (std::regex_search(defaultStr, vMatch, v1Regex))
                            {
                                float val = std::stof(vMatch[1].str());
                                uDef.vec4Val = glm::vec4(val);
                            }
                        }
                    }
                }
                catch (...) {
                }
            }

            outUniforms.push_back(uDef);

            // Output clean standard GLSL declaration or engine texture alias
            if (uDef.engineTexture == EngineTextureType::Screen)
            {
                cleanStream << "#define " << nameStr << " u_EngineScreenTexture\n";
            }
            else if (uDef.engineTexture == EngineTextureType::Depth)
            {
                cleanStream << "#define " << nameStr << " u_EngineDepthTexture\n";
            }
            else if (uDef.engineTexture == EngineTextureType::Normal)
            {
                cleanStream << "#define " << nameStr << " u_EngineNormalTexture\n";
            }
            else
            {
                cleanStream << "uniform " << typeStr << " " << nameStr << ";\n";
            }
        }
        else
        {
            cleanStream << line << "\n";
        }
    }

    outCleanGlobals = cleanStream.str();
}

bool CustomShaderGenerator::GenerateGLSL(
    const std::string& rshaderSource,
    bool& outIsLit,
    ShaderRenderModes& outRenderModes,
    std::vector<ShaderUniformDef>& outUniforms,
    std::string& outGLSLSource,
    std::string& outError
)
{
    outUniforms.clear();
    outError.clear();
    outRenderModes = ShaderRenderModes{};
    outIsLit = true;

    std::regex renderModeRegex(R"(\brender_mode\s+([^;]+);)");
    auto rmBegin = std::sregex_iterator(rshaderSource.begin(), rshaderSource.end(), renderModeRegex);
    auto rmEnd = std::sregex_iterator();

    for (std::sregex_iterator i = rmBegin; i != rmEnd; ++i)
    {
        std::smatch match = *i;
        std::string modes = match[1].str();
        std::istringstream modeStream(modes);
        std::string modeToken;
        while (std::getline(modeStream, modeToken, ','))
        {
            size_t first = modeToken.find_first_not_of(" \t\r\n");
            size_t last = modeToken.find_last_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
            std::string token = modeToken.substr(first, last - first + 1);

            if (token == "unshaded" || token == "unlit")
            {
                outIsLit = false;
            }
            else if (token == "cull_disabled")
            {
                outRenderModes.hasCullMode = true;
                outRenderModes.cullMode = CullMode::Disabled;
            }
            else if (token == "cull_front")
            {
                outRenderModes.hasCullMode = true;
                outRenderModes.cullMode = CullMode::Front;
            }
            else if (token == "cull_back")
            {
                outRenderModes.hasCullMode = true;
                outRenderModes.cullMode = CullMode::Back;
            }
            else if (token == "depth_test_disabled")
            {
                outRenderModes.depthTestDisabled = true;
            }
            else if (token == "depth_draw_disabled")
            {
                outRenderModes.depthDrawDisabled = true;
            }
            else if (token == "depth_draw_opaque" || token == "depth_draw_always")
            {
                outRenderModes.depthDrawAlways = true;
            }
            else if (token == "blend_mix")
            {
                outRenderModes.hasBlendMode = true;
                outRenderModes.blendMode = BlendMode::Mix;
            }
            else if (token == "blend_add")
            {
                outRenderModes.hasBlendMode = true;
                outRenderModes.blendMode = BlendMode::Add;
            }
            else if (token == "blend_sub")
            {
                outRenderModes.hasBlendMode = true;
                outRenderModes.blendMode = BlendMode::Subtract;
            }
            else if (token == "blend_mul")
            {
                outRenderModes.hasBlendMode = true;
                outRenderModes.blendMode = BlendMode::Multiply;
            }
            else if (token == "wireframe")
            {
                outRenderModes.wireframe = true;
            }
        }
    }

    // Extract vertex() and fragment()
    std::string remainingSource = rshaderSource;

    size_t vertStart = 0, vertEnd = 0;
    std::string vertBody = ExtractFunctionBody(remainingSource, "vertex", vertStart, vertEnd);
    if (vertStart != std::string::npos)
    {
        remainingSource.erase(vertStart, vertEnd - vertStart);
    }

    size_t fragStart = 0, fragEnd = 0;
    std::string fragBody = ExtractFunctionBody(remainingSource, "fragment", fragStart, fragEnd);
    if (fragStart != std::string::npos)
    {
        remainingSource.erase(fragStart, fragEnd - fragStart);
    }

    std::string cleanGlobals;
    ParseUniforms(remainingSource, outUniforms, cleanGlobals);

    std::string templatePath = ENGINE_PATH("Shaders\\CustomShaderBase.glsl").GetAbsoluteStr();
    std::ifstream templateFile(templatePath);
    if (!templateFile.is_open())
    {
        outError = "Failed to load base custom shader template at: " + templatePath;
        return false;
    }

    std::stringstream templateStream;
    templateStream << templateFile.rdbuf();
    std::string glsl = templateStream.str();

    if (!outIsLit)
    {
        std::string fragHeader = "#shader fragment\n#version 460 core";
        size_t fragPos = glsl.find(fragHeader);
        if (fragPos != std::string::npos)
        {
            glsl.insert(fragPos + fragHeader.length(), "\n#define RVELA_UNLIT\n");
        }
    }

    auto ReplaceAll = [](std::string& str, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos)
        {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    ReplaceAll(glsl, "// === USER_GLOBALS_VERTEX ===", cleanGlobals);
    ReplaceAll(glsl, "// === USER_GLOBALS_FRAGMENT ===", cleanGlobals);
    ReplaceAll(glsl, "// === USER_VERTEX_CODE ===", vertBody);
    ReplaceAll(glsl, "// === USER_FRAGMENT_CODE ===", fragBody);

    outGLSLSource = std::move(glsl);
    return true;
}

}
