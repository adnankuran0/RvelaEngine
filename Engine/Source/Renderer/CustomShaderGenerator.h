#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "Asset/AssetUUID.h"
#include "Renderer/RenderTypes.h"
#include "Renderer/Sampler.h"

namespace rv {

enum class ShaderUniformType {
    Float = 0,
    Int,
    Bool,
    Vec2,
    Vec3,
    Vec4,
    Color3,
    Color4,
    Texture2D
};

enum class EngineTextureType {
    None = 0,
    Screen,
    Depth,
    Normal
};

struct ShaderRenderModes {
    bool hasCullMode = false;
    CullMode cullMode = CullMode::Back;

    bool hasBlendMode = false;
    BlendMode blendMode = BlendMode::Mix;

    bool depthTestDisabled = false;
    bool depthDrawDisabled = false;
    bool depthDrawAlways = false;

    bool wireframe = false;
};

struct ShaderUniformDef {
    std::string name;
    std::string displayName;
    ShaderUniformType type = ShaderUniformType::Float;
    EngineTextureType engineTexture = EngineTextureType::None;
    float floatVal = 0.0f;
    int intVal = 0;
    bool boolVal = false;
    glm::vec2 vec2Val{ 0.0f };
    glm::vec3 vec3Val{ 0.0f };
    glm::vec4 vec4Val{ 0.0f };
    AssetUUID textureUUID = AssetUUID::Invalid();
    bool hasRange = false;
    float minVal = 0.0f;
    float maxVal = 1.0f;

    // Sampler filter and repeat hints
    bool hasCustomSampler = false;
    SamplerDesc samplerDesc{};
};

struct CustomUniformValue {
    ShaderUniformType type = ShaderUniformType::Float;
    float floatVal = 0.0f;
    int intVal = 0;
    bool boolVal = false;
    glm::vec2 vec2Val{ 0.0f };
    glm::vec3 vec3Val{ 0.0f };
    glm::vec4 vec4Val{ 0.0f };
    AssetUUID textureUUID = AssetUUID::Invalid();
};

class CustomShaderGenerator
{
public:
    static bool GenerateGLSL(
        const std::string& rshaderSource,
        bool& outIsLit,
        ShaderRenderModes& outRenderModes,
        std::vector<ShaderUniformDef>& outUniforms,
        std::string& outGLSLSource,
        std::string& outError
    );

private:
    static void ParseUniforms(
        const std::string& source,
        std::vector<ShaderUniformDef>& outUniforms,
        std::string& outCleanGlobals
    );
};

}
